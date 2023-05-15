Title: User Guide
codelink_BINLOG_INFO: include/binlog/basic_macros.hpp

A quick tour to a high-throughput low-latency logging library.

[TOC]

# Hello World

    [catchfile example/HelloWorld.cpp hello]

Compile and run this program: it will produce a binary logfile, `hello.blog`.
The logfile can be converted to text using `bread`:

    $ bread hello.blog
    INFO [10/05 20:05:30.176362854] Hello World!

Compared to other log libraries, Binlog is very fast for two reasons:
First, Binlog is using asynchronous logging. The log events are first copied to
a fast-access, lock-free intermediate storage, before `consume` writes
them to the logfile.
Second, Binlog produces a structured, binary log.
This means, in the application, arguments and timestamps are not converted to text,
and are not substituted into a format string. These happen in a later stage offline,
while reading the logs (i.e: when running `bread`).
As a bonus, structured logfiles are smaller, more flexible (e.g: text representation is customizable),
and easier to data mine.

# Logging

Binlog provides a macro interface for logging.
The most basic log macros are: `BINLOG_TRACE`, `BINLOG_DEBUG`, `BINLOG_INFO`, `BINLOG_WARNING`, `BINLOG_ERROR` and `BINLOG_CRITICAL`,
one for each severity. Usage:

    [catchfile test/integration/Logging.cpp log]

Each basic log macro takes a format string, and zero or more arguments.
The format string can contain `{}` placeholders for the arguments.
The number of placeholders in the format string and the number of arguments must match,
and it is enforced by a compile time check.
Events are timestamped using `std::chrono::system_clock`.
The set of loggable argument types includes primitives, containers, pointers,
pairs and tuples, enums and adapted user defined types - as shown below.

## Logging Containers

Standard containers of loggable `value_type` are loggable by default:

    [catchfile test/integration/LoggingContainers.cpp sequence]
    [catchfile test/integration/LoggingContainers.cpp associative]

Aside the standard containers, any container-like type is loggable
that satisfies the following constraints:

 - It has a const qualified `begin()` and `end()`
 - The iterators returned by begin/end satisfy the forward iterator concept
 - The `value_type` of the iterator is loggable

C style arrays of loggable elements can be wrapped in a view if the array size is known,
to be logged as a container:

    [catchfile test/integration/LoggingContainers.cpp carray]

## Logging Strings

Containers of characters (e.g: `std::string` or `std::vector<char>`) are logged just
like any other container, but have a special text representation:

    [catchfile test/integration/LoggingStrings.cpp stdstr]

`const char*` arguments, because of established convention, are assumed to
point to null terminated strings, therefore logged and displayed accordingly.

Unfortunately, sometimes even `char*` is used to refer to a null terminated string
(e.g: [strerror][]). Binlog has no way to know whether the passed argument
is a string (therefore calling `strlen` on it is safe) or just a pointer to a single character.
To remain on the safe side, by default, Binlog assumes the latter.
A few ways to workaround this:

 - Use `const char*`, instead of `char*`. If `char*` is given, assign it to a `const char*` variable before logging
 - Make the intent explicit, and wrap `char*` by `string_view`
 - Trade safety for convenience, and make Binlog treat every `char*` as a string:

        [catchfile test/integration/LoggingCStrings.cpp cstr]

[strerror]: https://en.cppreference.com/w/cpp/string/byte/strerror

## Logging Pointers and Optionals

Raw and standard smart pointers pointing to a loggable `element_type`
are loggable by default:

    [catchfile test/integration/LoggingPointers.cpp stdptr]

If the pointer is valid, it gets dereferenced and the pointed object will be logged.
If the pointer is _empty_ (i.e: it points to no valid object),
no value is logged, and in the text log it will be shown as `{null}`.
Logging of `weak_ptr` is not supported, those must be `.lock()`-ed first.

To log the address pointed by the pointer (instead of the object it points to),
apply `binlog::address` or cast the pointer to a `void*`:

    [catchfile test/integration/LoggingPointers.cpp address]

Aside the standard pointers, any pointer or optional-like type is loggable
that satisfies the following constraints:

 - It is explicitly convertible to `bool`
 - Dereferencing it yields to a loggable type
 - It is declared as being an optional, e.g:

        [catchfile test/integration/LoggingBoostTypes.cpp optspec]

## Logging Pairs and Tuples

Standard pair and tuple with loggable elements are loggable by default:

    [catchfile test/integration/LoggingTuples.cpp stdtup]

Aside the standard pair and tuple, any tuple-like type is loggable
that satisfies the following constraints:

 - It's declaration matches `T<E...>`.
 - Each `E` of `E...` are loggable
 - Elements are accessible via unqualified call to `get<N>(t)`
 - It is declared as being a tuple, e.g:

        namespace mserialize { namespace detail {
          template <typename... T>
          struct is_tuple<boost::tuple<T...>> : std::true_type {};
        }}

## Logging Enums

Enums are loggable by default, serialized and shown as their underlying type:

    [catchfile test/integration/LoggingEnums.cpp basic]

To make the log easier to read, enums can be adapted:
adapted enums are still serialized as their underlying type,
but in the log, the enumerator name is shown:

    [catchfile test/integration/LoggingEnums.cpp adapted]

The `BINLOG_ADAPT_ENUM` must be called in global scope, outside of any namespace.
If an enumerator is omitted, the underlying value will be shown
instead of the omitted enumerator name.
Both scoped and unscoped enums can be adapted.
For scoped enums, the enumerators must be prefixed with the enum name, as usual:

    [catchfile test/integration/LoggingEnums.cpp scoped]

The maximum number of enumerators is limited to 100.

To adapt enum types shadowed by non-type declarations (e.g: a field),
the following workaround is required:

    [catchfile test/integration/LoggingEnums.cpp shadowed]

## Logging User Defined Structures

User defined types outside the categories above can be still logged,
if adapted:

    [catchfile test/integration/LoggingAdaptedStructs.cpp mixed]

`BINLOG_ADAPT_STRUCT` takes a typename, and a list of data members or getters.

Data members are:

 - non-static
 - non-reference
 - non-bitfield members, which has a
 - loggable type.

Getters are:

 - const qualified
 - nullary methods,
 - returning a loggable value, and
 - getters must not throw
 - must not log, and
 - must always return the same value during the creation of a single log event

The member list does not have to be exhaustive, e.g: mutex members can be omitted, those will not be logged.
The member list can be empty. The maximum number of members is limited to 100.
`BINLOG_ADAPT_STRUCT` must be called in global scope, outside of any namespace.
The type must not be recursive, e.g: `Foo` can't have a to be logged `Foo*` typed member.

User defined types that derive from base classes already adapted for logging
can be adapted, without enumerating the base members again:

    [catchfile test/integration/LoggingAdaptedStructs.cpp derived]

The second argument of `BINLOG_ADAPT_DERIVED` must be a list of adapted base classes, multiple inheritance is supported.

User defined templates can be adapted for logging in a similar way:

    [catchfile test/integration/LoggingAdaptedStructs.cpp adapt_template]

As of C++20, concepts can be also adapted for logging (namespaced concepts are also supported):

    [catchfile test/integration/LoggingAdaptedConcepts.cpp concept]

For more information see the Mserialize documentation on [Adapting custom types][mserialize-act] and
[Adapting user defined recursive types for visitation][mserialize-rec].

[mserialize-act]: Mserialize.html#adapting-custom-types
[mserialize-rec]: Mserialize.html#adapting-user-defined-recursive-types-for-visitation

## Logging Standard Types

### chrono

`std::chrono::system_clock::time_point` objects can be logged after adoption by including a header file:

    [catchfile test/integration/LoggingTimePoint.cpp timepoint]

The object is serialized as a single number (nanoseconds since epoch), and pretty printed as a human readable timestamp.
The pretty printed text format is affected by the `-d` flag when using [bread](#bread).
If `%d` (localtime) appears first in the `-f` (format) parameter, the time point is converted to producer-local timezone
(this is the default). If `%u` (UTC) appears first, the time point is shown in UTC.

Certain helper types of `std::chrono::duration` can be logged after adoption by including a header file:

    [catchfile test/integration/LoggingDuration.cpp duration]

### error_code

`std::error_code` objects can be logged after adoption by including a header file:

    [catchfile test/integration/LoggingErrorCode.cpp ec]

The error code is serialized in terms of the `message` member, therefore it may result in memory allocations.

### optional

If C++17 is available, the standard optional with a loggable `value_type` can be made loggable:

    [catchfile test/integration/LoggingOptionals.cpp stdopt]

### filesystem

If C++17 is available, standard filesystem types can be made loggable:

    [catchfile test/integration/LoggingFilesystem.cpp fs]

The supported filesystem types are: `path`, `directory_entry`, `file_type`, `perms`, `space_info`, `file_status`.
On platforms where `path::value_type` is `char` (e.g: POSIX platforms), `path` objects are serialized using the `native` member,
therefore no extra memory allocation is required.
On platforms where `path::value_type` is **not** `char` (e.g: on Windows), `path` objects are serialized using the `string` member,
which may result in extra memory allocations.

### variant

If C++17 is available, standard variant types can be made loggable:

    [catchfile test/integration/LoggingVariants.cpp variant]

To log a variant, every alternative must be loggable. The number of alternatives must be less than 256.
Monostate and valueless-by-exception variants are supported.

# Tools

## bread

The binary logfile produced by Binlog is not human-readable,
it has to be converted to text first. The `bread` program
reads the given binary logfile, converts it to text
and writes the text to the standard output.

    $ bread logfile.blog > logfile.txt

The format of the text representation can be customized using command line switches:

    $ bread -f "%S [%d] %n %m (%G:%L)" -d "%m/%d %H:%M:%S.%N" logfile.blog

The events of the logfile can be sorted by their timestamp using `-s`.
The complete input is consumed first, then sorted and printed in one go.

    $ bread -s logfile.blog

If no input file is specified, or it is `-`, `bread` reads from the standard input,
acting like a filter that converts a binary log stream to text. This allows reading
compressed logfiles:

    $ zcat logfile.blog.gz | bread

With piping, a live logfile that is being written by the application can be read,
even if log rotation is configured:

    $ tail -c +0 -F logfile.blog | bread

To customize the output and for further options, see the builtin help:

    $ bread -h

## brecovery

If the application crashes, because of the asynchronous logging employed by
Binlog, it is possible that the most interesting log events, those directly
preceding the crash, are still in the intermediate queues, waiting to be
consumed and flushed to the logfile. The `brecovery` program takes a coredump
(memory dump, crash dump, the memory image of the application at the time of the
crash), searches for metadata and log queues, and writes recovered content to the specified file:

    $ brecovery application.core recovered.blog

The recovered logfile is a binary logfile, that can be read using `bread`:

    $ bread recovered.blog

# A More Elaborate Greeting of the World

The first section, [Hello World](#hello-world) shows a very simple example,
where the data flow might not be clear, due to the hidden state,
which makes the example short. Consider the following, more explicit example:

    [catchfile example/DetailedHelloWorld.cpp]

`Session` is a log stream. `SessionWriter` can add log events to such a stream.
Multiple writers can write a single session concurrently,
as between the session and  each writer there's a queue of bytes.
In the first example, the default instances of these types were used implicitly,
provided by `default_session()` and `default_thread_local_writer()`.

`BINLOG_INFO_W` is the same as `BINLOG_INFO`, except that it takes
an additional writer as the first argument, which it will use
to add the event instead of the default writer.
The log event, created by this macro, is first serialized into the
queue of the `writer`, upon invocation. If this is the first
call to this macro, the metadata associated with this event
is also added to the session of the writer.
This macro is available for each severity, i.e:
`BINLOG_TRACE_W`, `BINLOG_DEBUG_W`, `BINLOG_INFO_W`, `BINLOG_WARNING_W`, `BINLOG_ERROR_W` and `BINLOG_CRITICAL_W`.
Serialization is done using the [Mserialize][] library.

When `session.consume` is called, first the available metadata is
consumed and written to the provided stream. Then each writer queue
is polled for data. Available data is written to the provided stream
in batches. At the end of the program, the health of the output stream
is checked, to make sure errors are not swallowed (e.g: disk full).

[Mserialize]: Mserialize.html

# Named Writers

To make the source of the events easier to identify, writers can be named.
Writer names appear in the converted text output.
This feature can be used to distinguish the output of different threads.

    [catchfile test/integration/NamedWriters.cpp setName]

# Severity Control

It might be desirable to change the verbosity of the logging runtime.
Setting the minimum severity of the session disables production
of events with lesser severities:

    [catchfile test/integration/SeverityControl.cpp setmin]

If an event of disabled severity is given to a writer,
it will be discarded without effect, and the log arguments will not be evaluated.

    [catchfile test/integration/SeverityControl.cpp noeval]

# Categories

To separate the log events coming from different components of the application,
a category can be attached to them:

    [catchfile test/integration/Categories.cpp c]

`BINLOG_INFO_C` is the same as `BINLOG_INFO`, except that it takes
an additional category name as the first argument, which will be the
category of the event. The category name can be any valid identifier,
that must be available compile time. The name of the default category is `main`.
This macro is available for each severity, i.e:
`BINLOG_TRACE_C`, `BINLOG_DEBUG_C`, `BINLOG_INFO_C`, `BINLOG_WARNING_C`, `BINLOG_ERROR_C` and `BINLOG_CRITICAL_C`.

The `BINLOG_<SEVERITY>_W` and `BINLOG_<SEVERITY>_C` macros can be combined:

    [catchfile test/integration/Categories.cpp wc]

As above, there's one for each severity, i.e:
`BINLOG_TRACE_WC`, `BINLOG_DEBUG_WC`, `BINLOG_INFO_WC`, `BINLOG_WARNING_WC`, `BINLOG_ERROR_WC` and `BINLOG_CRITICAL_WC`.

# Consume Logs

Regardless the exact log macro being used (`BINLOG_<SEVERITY>*`), when an event is created,
it is first put into the queue of the writer. When `session.consume(ostream)` is called,
these queues are polled and the acquired data is written to the given `ostream`.
If the writer is unable to write the queue, because it is full, it creates a new one,
and closes the old. Therefore, a balance of new event frequency, queue size and consume frequency
must be established. For applications built around a _main loop_, it might be suitable to
consume the logs at the end of each loop iteration, sizing the queue according to the
estimated amount of data one iteration produces:

    [catchfile example/ConsumeLoop.cpp loop]

For different kind of applications, calling `consume` periodically in a dedicated thread
or task can be an option.

# Log Rotation

[Log rotation][] can be achieved by simply changing the output stream passed to `Session::consume`.
`Session` does not know or care about the underlying device of the stream.
However, metadata is not added automatically to the new file (as `Session` does not know
that a rotation happened). To make the new file self contained (i.e: readable without the
metadata in the old file), old metadata has to be added via `Session::reconsumeMetadata`:

    [catchfile example/LogRotation.cpp rotate]

[Log rotation]: https://en.wikipedia.org/wiki/Log_rotation

# Text Output

The key feature of Binlog is producing structured, binary logfiles.
However, in some cases, writing text output directly might be still desirable.
This is easy to do using `TextOutputStream`:

    [catchfile example/TextOutput.cpp]

`TextOutputStream` requires the Binlog library to be linked to the application.

# Multiple Output

`Session::consume` takes a single target only, but it is easy to multiplex the log stream
to produce multiple output streams. With `EventFilter`, it is possible to route
different kinds of events to different outputs, efficiently. In this example,
the complete binary log is written to a logfile, but high severity events are also
written to the standard error as text:

    [catchfile example/MultiOutput.cpp ostream]

Usage is like before:

    [catchfile example/MultiOutput.cpp usage]

# Limitations

**Logging in global destructor context**:
The `BINLOG_<SEVERITY>` and `BINLOG_<SEVERITY>_C` macros that do not take a writer argument
access global state: a function local static writer and a session. Depending on the initialization order,
in a [global destructor context][cppref-Initialization], that state might be no longer valid,
yielding to undefined behavior. Possible resolutions include:

 - Do not log in the destructor of a global object
 - If logging in the destructor of a global object cannot be avoided,
   make sure that initialization of the global log state is sequenced before
   the initialization of the affected global objects
 - Use log macros that do not access global state, e.g: `BINLOG_<SEVERITY>_W`,
   and make sure the initialization of its writer argument is sequenced before
   the initialization of the affected global objects

[cppref-Initialization]: https://en.cppreference.com/w/cpp/language/initialization

**Timestamps**: By default, Binlog uses `std::chrono::system_clock` to create timestamps,
and assumes that this clock measures UTC time (without leap seconds). This is required as of C++20.
The Binlog test suite tests if this requirement holds on a given platform.

