#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

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
  #ifdef _MSC_VER
    #define popen(command, mode) _popen(command, mode)
    #define pclose(stream) _pclose(stream)

    std::replace(cmd.begin(), cmd.end(), '/', '\\');
  #endif

  INFO("Cmd: ", cmd);

  FILE* f = popen(cmd.data(), "r");
  REQUIRE(f != nullptr);

  std::string result;

  char buf[4096];
  while (std::size_t rs = std::fread(buf, 1, sizeof(buf), f))
  {
    result.append(buf, rs);
  }

  CHECK(pclose(f) != -1);
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
  CHECK(expected == actual);
}

TEST_CASE("Logging")               { runReadDiff("Logging", "%S %m"); }
TEST_CASE("LoggingFundamentals")   { runReadDiff("LoggingFundamentals", "%m"); }
TEST_CASE("LoggingContainers")     { runReadDiff("LoggingContainers", "%m"); }
TEST_CASE("LoggingStrings")        { runReadDiff("LoggingStrings", "%m"); }
TEST_CASE("LoggingCStrings")       { runReadDiff("LoggingCStrings", "%m"); }
TEST_CASE("LoggingPointers")       { runReadDiff("LoggingPointers", "%m"); }
TEST_CASE("LoggingTuples")         { runReadDiff("LoggingTuples", "%m"); }
TEST_CASE("LoggingEnums")          { runReadDiff("LoggingEnums", "%m"); }
TEST_CASE("LoggingAdaptedStructs") { runReadDiff("LoggingAdaptedStructs", "%m"); }
TEST_CASE("LoggingTimePoint")      { runReadDiff("LoggingTimePoint", "%m"); }
TEST_CASE("LoggingDuration")       { runReadDiff("LoggingDuration", "%m"); }
TEST_CASE("LoggingErrorCode")      { runReadDiff("LoggingErrorCode", "%m"); }
TEST_CASE("NamedWriters")          { runReadDiff("NamedWriters", "%n %m"); }
TEST_CASE("SeverityControl")       { runReadDiff("SeverityControl", "%S %m"); }
TEST_CASE("Categories")            { runReadDiff("Categories", "%C %n %m"); }

#if __cplusplus >= 201703L

TEST_CASE("LoggingOptionals")      { runReadDiff("LoggingOptionals", "%m"); }
TEST_CASE("LoggingFilesystem")     { runReadDiff("LoggingFilesystem", "%m"); }
TEST_CASE("LoggingVariants")       { runReadDiff("LoggingVariants", "%m"); }

#endif

#if __cplusplus >= 202002L

TEST_CASE("LoggingAdaptedConcepts") { runReadDiff("LoggingAdaptedConcepts", "%m"); }

#endif

#ifdef BINLOG_HAS_BOOST

TEST_CASE("LoggingBoostTypes")     { runReadDiff("LoggingBoostTypes", "%m"); }

#endif

TEST_CASE("DateFormat")
{
  // read dateformat.blog, convert to text, check result
  std::ostringstream cmd;
  cmd << g_bread_path << " -f \"%u %m\" -d \"%Y-%m-%dT%H:%M:%S.%NZ\" " << g_src_dir << "data/dateformat.blog";
  const std::string actual = executePipeline(cmd.str());
  const std::string expected = "2019-12-02T13:38:33.602967233Z Hello\n";
  CHECK(expected == actual);
}

TEST_CASE("RecoverMetadataAndData")
{
  // check gdb
  const std::string gdbpath = "/usr/bin/gdb";
  if (! fileReadable(gdbpath))
  {
    MESSAGE("gdb not found, skip this test");
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
  MESSAGE("Run GDB: ", gdbcmd.str());
  const int gdbretval = std::system(gdbcmd.str().data());
  (void) gdbretval; // return value is implementation specified

  // check the core
  if (! fileReadable(corepath))
  {
    MESSAGE("Could not generate corefile, skip this test");
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
    CHECK(recovered == expected1);
  }
  else
  {
    CHECK(recovered == expected2);
  }

  std::remove(corepath.data());
}

void initGlobals(int argc, const char* argv[])
{
  g_bread_path = (argc > 1) ? argv[1] : "./bread" + extension();
  g_inttest_dir = (argc > 2) ? argv[2] + std::string("/") : "./";
  g_src_dir = (argc > 3) ? argv[3] + std::string("/test/integration/") : "../test/integration/";
}

int main(int argc, const char* argv[])
{
  int testargc = argc;
  for (int i = 0; i < argc; ++i)
  {
    if (argv[i] == std::string("--"))
    {
      testargc = i;
      break;
    }
  }

  initGlobals(argc - testargc, argv + testargc);

  doctest::Context context(testargc, argv);

  return context.run();
}
