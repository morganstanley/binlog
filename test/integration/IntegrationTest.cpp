#include <boost/test/unit_test.hpp>

#include <mserialize/string_view.hpp>

#include <algorithm> // replace
#include <cctype> // isspace
#include <cstdio> // remove, popen, pclose, fread
#include <fstream>
#include <sstream>
#include <string>

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

std::string executePipeline(std::string cmd)
{
  #ifdef _WIN32
    #define popen(command, mode) _popen(command, mode)
    #define pclose(stream) _pclose(stream)

    std::replace(cmd.begin(), cmd.end(), '/', '\\');
  #endif

  BOOST_TEST_MESSAGE("Cmd: " + cmd);

  FILE* f = popen(cmd.data(), "r");
  BOOST_TEST_REQUIRE(f != nullptr);

  std::string result;

  char buf[4096];
  while (std::size_t rs = std::fread(buf, 1, sizeof(buf), f))
  {
    result.append(buf, rs);
  }

  BOOST_TEST(pclose(f) != -1);
  return result;
}

bool fileReadable(const std::string& path)
{
  std::ifstream i(path);
  return bool(i);
}

std::string expectedDataFromSource(const std::string& name)
{
  std::ostringstream result;

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
      result << view << "\n";
    }
  }

  if (!input && !input.eof())
  {
    throw std::runtime_error("Failed to read " + path);
  }

  return result.str();
}

void runReadDiff(const std::string& name, const std::string& format)
{
  std::ostringstream cmd;
  cmd << g_inttest_dir << name << extension()
      << " | " << g_bread_path << " -f \"" << format << "\" - ";
  const std::string actual = executePipeline(cmd.str());
  const std::string expected = expectedDataFromSource(name);
  BOOST_TEST(expected == actual);
}

BOOST_AUTO_TEST_SUITE(RunReadDiff)

BOOST_AUTO_TEST_CASE(Logging)               { runReadDiff("Logging", "%S %m"); }
BOOST_AUTO_TEST_CASE(LoggingFundamentals)   { runReadDiff("LoggingFundamentals", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingContainers)     { runReadDiff("LoggingContainers", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingStrings)        { runReadDiff("LoggingStrings", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingCStrings)       { runReadDiff("LoggingCStrings", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingPointers)       { runReadDiff("LoggingPointers", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingTuples)         { runReadDiff("LoggingTuples", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingEnums)          { runReadDiff("LoggingEnums", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingAdaptedStructs) { runReadDiff("LoggingAdaptedStructs", "%m"); }
BOOST_AUTO_TEST_CASE(LoggingTimePoint)      { runReadDiff("LoggingTimePoint", "%m"); }
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
  std::ostringstream cmd;
  cmd << g_bread_path << " -f \"%u %m\" -d \"%Y-%m-%dT%H:%M:%S.%NZ\" " << g_src_dir << "data/dateformat.blog";
  const std::string actual = executePipeline(cmd.str());
  const std::string expected = "2019-12-02T13:38:33.602967233Z Hello\n";
  BOOST_TEST(expected == actual);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Brecovery)

BOOST_AUTO_TEST_CASE(RecoverMetadataAndData)
{
  // check gdb
  const std::string gdbpath = "/usr/bin/gdb";
  if (! fileReadable(gdbpath))
  {
    BOOST_TEST_MESSAGE("gdb not found, skip this test");
    return;
  }

  // run shell in gdb, create a corefile
  const std::string corepath = "shell.core";

  std::ostringstream gdbcmd;
  gdbcmd << gdbpath << " -q -iex 'set auto-load python-scripts off' -nx -batch"
    " -ex 'print \"THIS IS A TEST. CRASH THE TEST PROGRAM\"'" // make the test output less scary
    " -ex run -ex 'gcore " << corepath << "' --args '" << g_inttest_dir << "Shell" << extension() << "'"
    " 'log w1 hello w1'"
    " 'log w2 hello w2'"
    " 'log w1 bye w1'"
    " 'log w2 bye w2'"
    " terminate";
  BOOST_TEST_MESSAGE("Run GDB: " + gdbcmd.str());
  const int gdbretval = std::system(gdbcmd.str().data());
  (void) gdbretval; // return value is implementation specified

  // check the core
  if (! fileReadable(corepath))
  {
    BOOST_TEST_MESSAGE("Could not generate corefile, skip this test");
    return;
  }

  // recover data from the core
  std::ostringstream reccmd;
  reccmd << g_inttest_dir + "brecovery" + extension() << " " << corepath
         << " | " << g_bread_path << " -f %m";
  const std::string recovered = executePipeline(reccmd.str());

  // order of recovered buffers is unspecified
  const std::string expected1 = "hello w1\nbye w1\nhello w2\nbye w2\n";
  const std::string expected2 = "hello w2\nbye w2\nhello w1\nbye w1\n";
  if (recovered == expected1)
  {
    BOOST_TEST(recovered == expected1);
  }
  else
  {
    BOOST_TEST(recovered == expected2);
  }

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
