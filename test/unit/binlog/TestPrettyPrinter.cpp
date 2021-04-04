#include <binlog/PrettyPrinter.hpp>

#include <binlog/Entries.hpp>
#include <binlog/Range.hpp>

#include <mserialize/serialize.hpp>

#include <doctest/doctest.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct TestcaseBase
{
  TestcaseBase()
  {
    std::ostringstream argsBufferStream;
    mserialize::serialize(111, argsBufferStream);
    mserialize::serialize(std::string("foo"), argsBufferStream);
    argsBuffer = argsBufferStream.str();
    event.arguments = binlog::Range(argsBuffer.data(), argsBuffer.data() + argsBuffer.size());
  }

  binlog::EventSource eventSource{
    123, binlog::Severity::info,
    "cat", "func", "dir1/dir2/file", 456, "a: {}, b: {}", "i[c"
  };

  std::string argsBuffer;
  binlog::Event event{&eventSource, 1569939329, {}};
  binlog::WriterProp writerProp{789, "writer", 0};
  binlog::ClockSync clockSync{0, 1, 0, 5400, "XYZ"};

  std::string print(binlog::PrettyPrinter& pp) const
  {
    std::ostringstream str;
    pp.printEvent(str, event, writerProp, clockSync);
    return str.str();
  }
};

} // namespace

TEST_CASE_FIXTURE(TestcaseBase, "empty_fmt")
{
  binlog::PrettyPrinter pp("", "");
  CHECK(print(pp) == "");
}

TEST_CASE_FIXTURE(TestcaseBase, "full_fmt")
{
  binlog::PrettyPrinter pp(
    "%I %S %C %M %F %G %L %P %T %n %t %d %u %r %m %% %x foo",
    "%Y %y-%m-%d %H:%M:%S.%N %z %Z"
  );

  CHECK(print(pp) ==
    "123 INFO cat func dir1/dir2/file file 456 a: {}, b: {}"
    " i[c writer 789"
    " 2019 19-10-01 15:45:29.000000000 +0130 XYZ"
    " 2019 19-10-01 14:15:29.000000000 +0000 UTC"
    " 1569939329 a: 111, b: foo % %x foo"
  );
}

TEST_CASE_FIXTURE(TestcaseBase, "reverse_full_fmt")
{
  // make sure PP assumes no particular order
  binlog::PrettyPrinter pp(
    "foo %x %% %m %r %u %d %t %n %T %P %L %G %F %M %C %S %C",
    "%Z %z %N.%S:%M:%H %d-%m-%y %Y"
  );

  CHECK(print(pp) ==
    "foo %x % a: 111, b: foo 1569939329"
    " UTC +0000 000000000.29:15:14 01-10-19 2019"
    " XYZ +0130 000000000.29:45:15 01-10-19 2019"
    " 789 writer i[c a: {}, b: {} 456 file dir1/dir2/file func cat INFO cat"
  );
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_clock_sync")
{
  binlog::PrettyPrinter pp("%d %u %r", "");

  clockSync = binlog::ClockSync{};

  CHECK(print(pp) == "no_clock_sync? no_clock_sync? 1569939329");
}

TEST_CASE_FIXTURE(TestcaseBase, "negative_clock_sync_freq")
{
  binlog::PrettyPrinter pp("%d %u %r", "");

  clockSync = binlog::ClockSync{0, std::uint64_t(-1), 0, 0, {}};
  event = binlog::Event{&eventSource, 0x8000000000000000, event.arguments};

  CHECK(print(pp) == "no_clock_sync? no_clock_sync? 9223372036854775808");
}

TEST_CASE_FIXTURE(TestcaseBase, "filename")
{
  binlog::PrettyPrinter pp("%G", "");

  eventSource.file = "";
  CHECK(print(pp) == "");

  eventSource.file = "/";
  CHECK(print(pp) == "");

  eventSource.file = "/foo";
  CHECK(print(pp) == "foo");

  eventSource.file = "/a/b/c.cpp";
  CHECK(print(pp) == "c.cpp");

  eventSource.file = "bar";
  CHECK(print(pp) == "bar");

  eventSource.file = R"(\a\b\c.cpp)";
  CHECK(print(pp) == "c.cpp");
}

TEST_CASE_FIXTURE(TestcaseBase, "tzoffset")
{
  binlog::PrettyPrinter pp("%d", "%z");

  clockSync.tzOffset = 0;
  CHECK(print(pp) == "+0000");

  clockSync.tzOffset = -60 * 60 * 3;
  CHECK(print(pp) == "-0300");

  clockSync.tzOffset = 60 * 30 * 5;
  CHECK(print(pp) == "+0230");
}

TEST_CASE_FIXTURE(TestcaseBase, "corrupt_argument_tags")
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.argumentTags = "[c[c";
  CHECK_THROWS_AS(print(pp), std::runtime_error);
}

TEST_CASE_FIXTURE(TestcaseBase, "corrupt_argument_buffer")
{
  binlog::PrettyPrinter pp("%m", "");

  event.arguments.read<char>(); // drop one byte
  CHECK_THROWS_AS(print(pp), std::runtime_error);
}

TEST_CASE_FIXTURE(TestcaseBase, "corrupt_event_source_format")
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.formatString = "{}_{}_{}";

  CHECK(print(pp) == "111_foo_");
}

TEST_CASE_FIXTURE(TestcaseBase, "curlies_in_event_source_format")
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.formatString = "{ {}_{{}_ {";

  CHECK(print(pp) == "{ 111_{foo_ {");
}

TEST_CASE_FIXTURE(TestcaseBase, "closing_percentage")
{
  binlog::PrettyPrinter pp("%d %", "%Z %");

  CHECK(print(pp) == "XYZ % %");
}

TEST_CASE_FIXTURE(TestcaseBase, "inline_time_localtime_by_default")
{
  binlog::PrettyPrinter pp("%m", "%Y-%m-%d %H:%M:%S.%N %z %Z");
  eventSource.argumentTags = "{std::chrono::system_clock::time_point`ns'l}";

  std::ostringstream argsBufferStream;
  mserialize::serialize(std::uint64_t{123}, argsBufferStream);
  argsBuffer = argsBufferStream.str();
  event.arguments = binlog::Range(argsBuffer.data(), argsBuffer.data() + argsBuffer.size());
  eventSource.formatString = "{}";

  CHECK(print(pp) == "1970-01-01 01:30:00.000000123 +0130 XYZ");
}

TEST_CASE_FIXTURE(TestcaseBase, "inline_time_localtime_if_d")
{
  binlog::PrettyPrinter pp("%d %u %m", "%Y-%m-%d %H:%M:%S.%N %z %Z");
  eventSource.argumentTags = "{std::chrono::system_clock::time_point`ns'l}";

  std::ostringstream argsBufferStream;
  mserialize::serialize(std::uint64_t{456}, argsBufferStream);
  argsBuffer = argsBufferStream.str();
  event.arguments = binlog::Range(argsBuffer.data(), argsBuffer.data() + argsBuffer.size());
  eventSource.formatString = "{}";

  CHECK(print(pp) ==
    "2019-10-01 15:45:29.000000000 +0130 XYZ " // %d
    "2019-10-01 14:15:29.000000000 +0000 UTC " // %u
    "1970-01-01 01:30:00.000000456 +0130 XYZ"  // %m
  );
}

TEST_CASE_FIXTURE(TestcaseBase, "inline_time_utc_if_u")
{
  binlog::PrettyPrinter pp("%u %d %m", "%Y-%m-%d %H:%M:%S.%N %z %Z");
  eventSource.argumentTags = "{std::chrono::system_clock::time_point`ns'l}";

  std::ostringstream argsBufferStream;
  mserialize::serialize(std::uint64_t{789}, argsBufferStream);
  argsBuffer = argsBufferStream.str();
  event.arguments = binlog::Range(argsBuffer.data(), argsBuffer.data() + argsBuffer.size());
  eventSource.formatString = "{}";

  CHECK(print(pp) ==
    "2019-10-01 14:15:29.000000000 +0000 UTC " // %u
    "2019-10-01 15:45:29.000000000 +0130 XYZ " // %d
    "1970-01-01 00:00:00.000000789 +0000 UTC"  // %m
  );
}
