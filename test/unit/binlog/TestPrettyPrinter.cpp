#include <binlog/PrettyPrinter.hpp>

#include <binlog/Entries.hpp>
#include <binlog/Range.hpp>

#include <mserialize/serialize.hpp>

#include <boost/test/unit_test.hpp>

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
  binlog::Range args;
  binlog::Event event{&eventSource, 1569939329, {}};
  binlog::WriterProp writerProp{789, "writer", 0};
  binlog::ClockSync clockSync{0, 1, 0, 5400, "XYZ"};

  std::string print(binlog::PrettyPrinter& pp)
  {
    std::ostringstream str;
    pp.printEvent(str, event, writerProp, clockSync);
    return str.str();
  }
};

} // namespace

BOOST_AUTO_TEST_SUITE(PrettyPrinter)

BOOST_FIXTURE_TEST_CASE(empty_fmt, TestcaseBase)
{
  binlog::PrettyPrinter pp("", "");
  BOOST_TEST(print(pp) == "");
}

BOOST_FIXTURE_TEST_CASE(full_fmt, TestcaseBase)
{
  binlog::PrettyPrinter pp(
    "%I %S %C %M %F %G %L %P %T %n %t %d %u %r %m %% %x foo",
    "%Y %y-%m-%d %H:%M:%S.%N %z %Z"
  );

  BOOST_TEST(print(pp) ==
    "123 INFO cat func dir1/dir2/file file 456 a: {}, b: {}"
    " i[c writer 789"
    " 2019 19-10-01 15:45:29.000000000 +0130 XYZ"
    " 2019 19-10-01 14:15:29.000000000 +0000 UTC"
    " 1569939329 a: 111, b: foo % %x foo"
  );
}

BOOST_FIXTURE_TEST_CASE(reverse_full_fmt, TestcaseBase)
{
  // make sure PP assumes no particular order
  binlog::PrettyPrinter pp(
    "foo %x %% %m %r %u %d %t %n %T %P %L %G %F %M %C %S %C",
    "%Z %z %N.%S:%M:%H %d-%m-%y %Y"
  );

  BOOST_TEST(print(pp) ==
    "foo %x % a: 111, b: foo 1569939329"
    " UTC +0000 000000000.29:15:14 01-10-19 2019"
    " XYZ +0130 000000000.29:45:15 01-10-19 2019"
    " 789 writer i[c a: {}, b: {} 456 file dir1/dir2/file func cat INFO cat"
  );
}

BOOST_FIXTURE_TEST_CASE(empty_clock_sync, TestcaseBase)
{
  binlog::PrettyPrinter pp("%d %u %r", "");

  clockSync = binlog::ClockSync{};

  BOOST_TEST(print(pp) == "no_clock_sync? no_clock_sync? 1569939329");
}

BOOST_FIXTURE_TEST_CASE(negative_clock_sync_freq, TestcaseBase)
{
  binlog::PrettyPrinter pp("%d %u %r", "");

  clockSync = binlog::ClockSync{0, std::uint64_t(-1), 0, 0, {}};
  event = binlog::Event{&eventSource, 0x8000000000000000, event.arguments};

  BOOST_TEST(print(pp) == "no_clock_sync? no_clock_sync? 9223372036854775808");
}

BOOST_FIXTURE_TEST_CASE(filename, TestcaseBase)
{
  binlog::PrettyPrinter pp("%G", "");

  eventSource.file = "";
  BOOST_TEST(print(pp) == "");

  eventSource.file = "/";
  BOOST_TEST(print(pp) == "");

  eventSource.file = "/foo";
  BOOST_TEST(print(pp) == "foo");

  eventSource.file = "/a/b/c.cpp";
  BOOST_TEST(print(pp) == "c.cpp");

  eventSource.file = "bar";
  BOOST_TEST(print(pp) == "bar");

  eventSource.file = R"(\a\b\c.cpp)";
  BOOST_TEST(print(pp) == "c.cpp");
}

BOOST_FIXTURE_TEST_CASE(tzoffset, TestcaseBase)
{
  binlog::PrettyPrinter pp("%d", "%z");

  clockSync.tzOffset = 0;
  BOOST_TEST(print(pp) == "+0000");

  clockSync.tzOffset = -60 * 60 * 3;
  BOOST_TEST(print(pp) == "-0300");

  clockSync.tzOffset = 60 * 30 * 5;
  BOOST_TEST(print(pp) == "+0230");
}

BOOST_FIXTURE_TEST_CASE(corrupt_argument_tags, TestcaseBase)
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.argumentTags = "[c[c";
  BOOST_CHECK_THROW(print(pp), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(corrupt_argument_buffer, TestcaseBase)
{
  binlog::PrettyPrinter pp("%m", "");

  event.arguments.read<char>(); // drop one byte
  BOOST_CHECK_THROW(print(pp), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(corrupt_event_source_format, TestcaseBase)
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.formatString = "{}_{}_{}";

  BOOST_TEST(print(pp) == "111_foo_");
}

BOOST_FIXTURE_TEST_CASE(curlies_in_event_source_format, TestcaseBase)
{
  binlog::PrettyPrinter pp("%m", "");

  eventSource.formatString = "{ {}_{{}_ {";

  BOOST_TEST(print(pp) == "{ 111_{foo_ {");
}

BOOST_FIXTURE_TEST_CASE(closing_percentage, TestcaseBase)
{
  binlog::PrettyPrinter pp("%d %", "%Z %");

  BOOST_TEST(print(pp) == "XYZ % %");
}

BOOST_AUTO_TEST_SUITE_END()
