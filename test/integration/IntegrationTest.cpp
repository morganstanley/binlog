#include <boost/test/unit_test.hpp>

#include <boost/process/detail/traits/wchar_t.hpp>
#include <boost/process/env.hpp>
#include <boost/process/environment.hpp>
#include <boost/process/io.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>

#include <mserialize/string_view.hpp>

#include <algorithm> // sort
#include <cctype> // isspace
#include <cstdio> // remove
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector<std::string>)

std::string g_bread_path;
std::string g_inttest_dir;
std::string g_src_dir;

std::string extension()
{
  #ifdef _WIN32
    return ".exe";
  #else
    return {};
  #endif
}

std::vector<std::string> expectedDataFromSource(const std::string& name)
{
  std::vector<std::string> result;

  const std::string path = g_src_dir + name + ".cpp";
  std::ifstream input(path, std::ios_base::in|std::ios_base::binary);
  const std::string marker("// Outputs: ");

  std::string line;
  while (std::getline(input, line))
  {
    mserialize::string_view view(line);
    while (! view.empty() && std::isspace(static_cast<unsigned char>(view.front())) != 0)
    {
      view.remove_prefix(1);
    }

    while (! view.empty() && std::isspace(static_cast<unsigned char>(view.back())) != 0)
    {
      view.remove_suffix(1);
    }

    if (view.starts_with(marker))
    {
      view.remove_prefix(marker.size());
      result.push_back(view.to_string());
    }
  }

  if (!input && !input.eof())
  {
    throw std::runtime_error("Failed to read " + path);
  }

  return result;
}

std::vector<std::string> toVector(std::istream& input)
{
  std::vector<std::string> v;
  for (std::string line; std::getline(input, line);)
  {
    v.push_back(std::move(line));
  }

  return v;
}

void compareVectors(const std::vector<std::string>& expected, const std::vector<std::string>& actual)
{
  BOOST_TEST(actual == expected);
  if (actual.size() != expected.size())
  {
    // In this case, the test framework diagnostic is really lacking
    std::cerr << "***Expected***\n";
    for (auto&& s : expected) { std::cerr << s << "\n"; }
    std::cerr << "***Actual***\n";
    for (auto&& s : actual) { std::cerr << s << "\n"; }
  }
}

void runReadDiff(const std::string& name, const std::string& format)
{
  namespace bp = boost::process;

  bp::pipe p;
  bp::ipstream text;
  bp::child inttest(g_inttest_dir + name + extension(), bp::std_out > p);
  bp::child bread(g_bread_path, "-f", format, "-", bp::std_in < p, bp::std_out > text);

  const std::vector<std::string> actual = toVector(text);

  bread.wait();
  inttest.wait();

  const std::vector<std::string> expected = expectedDataFromSource(name);
  compareVectors(expected, actual);
}

BOOST_AUTO_TEST_SUITE(RunReadDiff)

BOOST_AUTO_TEST_CASE(Logging)               { runReadDiff("Logging", "%S %m"); }
BOOST_AUTO_TEST_CASE(LoggingFundamentals)   { runReadDiff("LoggingFundamentals", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingContainers)     { runReadDiff("LoggingContainers", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingStrings)        { runReadDiff("LoggingStrings", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingPointers)       { runReadDiff("LoggingPointers", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingTuples)         { runReadDiff("LoggingTuples", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingEnums)          { runReadDiff("LoggingEnums", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingAdaptedStructs) { runReadDiff("LoggingAdaptedStructs", "%m"); }
BOOST_AUTO_TEST_CASE(NamedWriters)          { runReadDiff("NamedWriters", "%n %m"); }
BOOST_AUTO_TEST_CASE(SeverityControl)       { runReadDiff("SeverityControl", "%S %m"); }
BOOST_AUTO_TEST_CASE(Categories)            { runReadDiff("Categories", "%C %n %m"); }

#if __cplusplus >= 201703L

BOOST_AUTO_TEST_CASE(LoggingOptionals)      { runReadDiff("LoggingOptionals", "%m"); }

#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Bread)

BOOST_AUTO_TEST_CASE(DateFormat)
{
  // read dateformat.blog, convert to text, check result
  namespace bp = boost::process;

  const std::string blogPath = g_src_dir + "data/dateformat.blog";
  bp::ipstream text;
  bp::child bread(g_bread_path, "-f", "%u %m", "-d", "%Y-%m-%dT%H:%M:%S.%NZ", blogPath, bp::std_out > text);

  const std::vector<std::string> actual = toVector(text);

  bread.wait();

  const std::vector<std::string> expected{"2019-12-02T13:37:38.262735011Z Hello"};

  compareVectors(expected, actual);
}

BOOST_AUTO_TEST_CASE(Pipe)
{
  // to make sure that piping works, used in scenarios like
  // `zcat log.blog.gz|bread` and `tail -c0 -f log.blog|bread`
  // pipe dateformat.blog byte by byte to bread, convert to text, check result
  namespace bp = boost::process;

  std::ifstream blog(g_src_dir + "data/dateformat.blog", std::ios_base::in|std::ios_base::binary);

  bp::opstream binary;
  bp::ipstream text;
  bp::child bread(g_bread_path, "-f", "%m", bp::std_in < binary, bp::std_out > text);

  // pipe input byte by byte to bread
  char byte = 0;
  while (blog.get(byte))
  {
    binary.put(byte);
    binary.flush(); // inefficient by design: allow bread to be scheduled
  }

  binary.pipe().close(); // so getline will not hang when the pipe is fully consumed

  // read bread output
  const std::vector<std::string> actual = toVector(text);

  bread.wait();

  compareVectors({"Hello"}, actual);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Brecovery)

BOOST_AUTO_TEST_CASE(RecoverMetadataAndData)
{
  namespace bp = boost::process;

  // find gdb
  const auto gdbpath = bp::search_path("gdb");
  if (gdbpath.empty())
  {
    BOOST_TEST_MESSAGE("gdb not found, skip this test");
    return;
  }

  // run shell in gdb, create a corefile
  const std::string shell = g_inttest_dir + "Shell" + extension();
  const std::string corepath = "shell." + std::to_string(boost::this_process::get_id()) + ".core";

  bp::child gdb(
    gdbpath,
    "-q", "-iex", "set auto-load python-scripts off",
    "-nx", "-batch", "-ex",
    "print \"THIS IS A TEST. CRASH THE TEST PROGRAM\"", // make the test output less scary
    "-ex", "run", "-ex", "gcore " + corepath, "--args", shell,
    "log w1 hello w1",
    "log w2 hello w2",
    "log w1 bye w1",
    "log w2 bye w2",
    "terminate",
    bp::std_in < bp::null
  );
  gdb.wait();

  // check the core
  std::ifstream core(corepath);
  if (! core)
  {
    BOOST_TEST_MESSAGE("Could not generate corefile, skip this test");
    return;
  }

  // recover data from the core
  bp::pipe binary;

  // dance around boost::process bug
  auto env = boost::this_process::environment();
  env.set("ASAN_OPTIONS", "allocator_may_return_null=1"); // we expect bad_alloc

  bp::child brecovery(
    g_inttest_dir + "brecovery" + extension(),
    corepath,
    bp::std_out > binary,
    env
  );

  bp::ipstream text;
  bp::child bread(g_bread_path, "-f", "%m", bp::std_in < binary, bp::std_out > text);

  std::vector<std::string> actual = toVector(text);
  std::sort(actual.begin(), actual.end()); // order of recovered events is unspecified

  brecovery.wait();
  binary.close();
  bread.wait();

  const std::vector<std::string> expected{"bye w1", "bye w2", "hello w1", "hello w2"};

  compareVectors(expected, actual);

  std::remove(corepath.data());
}

BOOST_AUTO_TEST_SUITE_END()

bool initMasterSuite()
{
  boost::unit_test::framework::master_test_suite().p_name.value = "Binlog Integration Test";

  // Boost.Test only allows custom args after the -- separator
  int argc = boost::unit_test::framework::master_test_suite().argc;
  char** argv = boost::unit_test::framework::master_test_suite().argv;

  g_bread_path = (argc > 1) ? argv[1] : "./bread" + extension();
  g_inttest_dir = (argc > 2) ? argv[2] + std::string("/") : "./";
  g_src_dir = (argc > 3) ? argv[3] + std::string("/test/integration/") : "../test/integration/";

  return true;
}

int main(int argc, /*const*/ char* argv[])
{
  return boost::unit_test::unit_test_main(&initMasterSuite, argc, argv);
}
