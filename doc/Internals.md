Title: Internals

Explaining how Binlog works, and why it is so fast.

[TOC]

# Introduction

The design goal of Binlog is correctness, ease of use and high performance.
Flexibility and ease of change is preferred over a large number of features.

Ease of use is achieved by supporting the logging of a [wide variety of types][logging] out of the box,
while also allowing simple adaption of custom [enums][adapt-enum] and [structures][adapt-struct].

Regular, text based logfiles are redundant, expensive to produce, not trivial to parse - but easy to use.
Binlog solves these issues by using binary logs, at the cost of adding an extra binary-to-text step
to the reading of the logs.

The binary log reduces redundancy by separating metadata (severity, category, format string, function, file, line)
from data (timestamp and arguments). The performance is improved by not converting binary values to text
in the application, only later, while reading the logs. Given the well defined format of the binary logfile,
it is more efficient and less error prone to parse programmatically.
The serialized format of the [adapted structures][adapt-struct] integrates into this format, further strengthening
the typed contract between the binary log writer and reader.

[logging]: UserGuide.html#logging
[adapt-enum]: UserGuide.html#logging-enums
[adapt-struct]: UserGuide.html#logging-user-defined-structures

# Glossary

  * **Entry**: A size prefixed binary blob. See Entries.hpp
  * **Event**: A kind of **Entry**, that corresponds to one log line in a text log
  * **EventSource**: A piece of code that produces **Event**s, can created by macros, e.g: `BINLOG_INFO`
  * **Session**: A concurrently writable and readable log stream
  * **SessionWriter**: A (non-concurrently) writable handle of a **Session** for adding **Event**s
  * **Producer**: An actor (thread, coroutine, fiber, etc.) that uses a **SessionWriter** to produce **Event**s
  * **OutputStream**: The destination of metadata and data consumed from a **Session**
  * **Consumer**: An actor that moves **Entries** from a **Session** to an **OutputStream**

# Architecture

    +---------------+  addEvent
    | SessionWriter |----------->( Channel )----+
    +---------------+                           |
                                                |
    +---------------+            ( Channel )    |   +---------+  consume  +--------------+
    | SessionWriter |----------->( Channel )----+-->| Session |---------->| OutputStream |
    +---------------+                           |   +---------+           +--------------+
           |                                    |        ^
           |                     ( Channel )----+        |
           |                                             |
           +---------------------------------------------+
                            addEventSource

The data flow of Binlog entries in a simple application is shown above.
Each Producer owns a SessionWriter. Through the SessionWriter, Producers
add EventSources (metadata) directly to the Session, and write Events (data) to a single-producer,
single-consumer queue, wrapped by a Channel. Every Channel is co-owned by a Session and a SessionWriter.
The Consumer reads the metadata from the Session, and the data from the Channels of the Session,
and writes them to the OutputStream. The Producers ands the Consumer can be any user defined actor
(thread, coroutine, fiber, etc.) - there's no hidden actor run by the Binlog library.

If a SessionWriter fills up the queue of its Channel, by default, it allocates a new Channel,
and closes the old one (see the second SessionWriter above), by dropping the owning reference to it.
The Consumer, after fully consuming the closed Channel, will deallocate it.

A Channel is said to be closed, if it is exclusively owned by a Session.
When a SessionWriter is destroyed, the owned Channel becomes closed.
Like before, the Consumer, after fully consuming the closed Channel, will deallocate it.
On the diagram above, such an orphan Channel can be seen, waiting to be fully consumed
and deallocated.

The asynchronous logging (i.e: SessionWriter does write OutputStream directly, but through the Channel)
saves cycles for the Producer (the hot path) to reduce latency. The cost of asynchronous logging is manifold:
First, Consumer task must be running periodically. Second, to restore the order of concurrently added
Events, [bread][] must do extra work. Third, if the application crashes, the remaining Events in the Channels
must be [recovered from the core dump][brecovery].

[bread]: UserGuide.html#bread
[brecovery]: UserGuide.html#brecovery

# Life of an Event

The life of an Event begins with the creation of an EventSource.
The EventSource describes the static properties of an Event,
e.g: severity, category, function, file, line, format string and [argument tags][mserialize-tag].
The newly created EventSource is added to the Session associated with the given SessionWriter.
The Session assigns a unique identifier to the source, that will be used by the events produced
by the source, and saves a copy of the EventSource. This happens only once for each EventSource.

After the source is created, space for an Event is allocated in the queue of the Channel
associated with the given SessionWriter. If the allocation fails, a new Channel is created,
and the old one becomes closed (as the writer drops its reference to it).
Now there is enough space, the Event fields are serialized into the queue:
size, EventSource identifier, clock value (the timestamp of the Event), and the log arguments
(the values the will be substitute the `{}` placeholders in the format string later).
The arguments are serialized by [Mserialize][], that means they are mostly copied directly
(no binary to text conversion), with very little decoration.
The number of arguments must match the number of placeholders in the format string,
and this is enforced compile time.

Following the serialization, the written data is committed to the queue.
The commit is atomic: the event will be either completely visible to the Consumer of the Session,
or it is not visible. Partial entries cannot be observed.
After the commit, the hot path of the Producer ends.

The periodically running Consumer first reads the metadata from the Session
(EventSources and ClockSync, if needed) and writes it to the OutputStream.
After the available metadata is fully consumed, it proceeds to poll each Channel.
Data (batch of Events) read from a Channel, preceded by a WriterProp kind of metadata, that
describes the SessionWriter is written to the OutputStream.

By design, the OutputStream is not owned by the Session, and only constrained by
a [simple concept][OutputStream], to make extensions simple: log rotation,
a syslog or custom monitoring backend, a network target are easy to add.

[Mserialize]: Mserialize.html
[mserialize-tag]: Mserialize.html#visiting-serialized-values
[OutputStream]: Mserialize.html#outputstream

# Binary log schema

The Binlog logfile is an append only stream of Binlog entries.
Entries can represent metadata (e.g: EventSource, WriterProp, ClockSync),
or data (i.e: log events). Metadata entries always precede data entries
that reference them.

    <BinlogStream> ::= <Entry>*
    <Entry>        ::= <EntrySize> <EntryPayload>
    <EntrySize>    ::= uint32
    <EntryPayload> ::= <EventSource> | <WriterProp> | <ClockSync> | <Event>

    <EventSource> ::= <EventSourceTag> <EventSourceId> <Severity> <Category> <Function> <File> <Line> <FormatString> <ArgumentTags>
    <EventSourceTag> ::= uint64(-1)
    <EventSourceId>  ::= uint64
    <Severity>       ::= uint16
    <Category>       ::= <String>
    <Function>       ::= <String>
    <File>           ::= <String>
    <Line>           ::= uint64
    <FormatString>   ::= <String>
    <ArgumentTags>   ::= <String>

    <WriterProp> ::= <WriterPropTag> <WriterPropId> <WriterPropName> <BatchSize>
    <WriterPropTag>  ::= uint64(-2)
    <WriterPropId>   ::= uint64
    <WriterPropName> ::= <String>
    <BatchSize>      ::= uint64

    <ClockSync> ::= <ClockSyncTag> <ClockValue> <ClockFrequency> <NsSinceEpoch> <TzOffset> <TzName>
    <ClockSyncTag>   ::= uint64(-3)
    <ClockValue>     ::= uint64
    <ClockFrequency> ::= uint64
    <NsSinceEpoch>   ::= uint64
    <TzOffset>       ::= int32
    <TzName>         ::= <String>

    <Event> ::= <EventSourceId> <ClockValue> <Arguments>
    <Arguments> ::= byte*   # serialized values according to the mserialize format

    <String> ::= <SequenceSize> char*

The semantic meaning of the fields is described in Entries.hpp.
Numbers are serialized according to the byte order of the producing host.
Tags with their most significant bit set indicate a metadata entry.
To ensure backward compatibility, a new field can be added only to the end
of any specific entry type. Adding a new metadata entry type is allowed,
as long as it is not required to parse the data.
To ensure forward compatibility, it is not an error if an entry has
extra payload after the last field (as long as the size field indicates that),
and unknown metadata entries are ignored.
