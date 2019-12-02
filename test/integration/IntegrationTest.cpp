#include <boost/test/unit_test.hpp>

#include <boost/process/io.hpp>
#include <boost/process/system.hpp>

#include <mserialize/string_view.hpp>

#include <cctype> // isspace
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector<std::string>)

std::string g_bread_path;
std::string g_inttest_dir;
std::string g_src_dir;

std::vector<std::string> expectedDataFromSource(const std::string& name)
{
  std::vector<std::string> result;

  const std::string path = g_src_dir + name + ".cpp";
  std::ifstream input(path);
  const std::string marker("// Outputs: ");

  std::string line;
  while (std::getline(input, line))
  {
    mserialize::string_view view(line);
    while (! view.empty() && std::isspace(static_cast<unsigned char>(view.front())) != 0)
    {
      view.remove_prefix(1);
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

void runReadDiff(const std::string& name, const std::string& format)
{
  namespace bp = boost::process;

  bp::pipe p;
  bp::ipstream text;
  bp::child inttest(g_inttest_dir + name, bp::std_out > p);
  bp::child bread(g_bread_path, "-", "-f", format, bp::std_in < p, bp::std_out > text);

  std::vector<std::string> actual;
  for (std::string line; std::getline(text, line);)
  {
    actual.push_back(std::move(line));
  }

  bread.wait();
  inttest.wait();

  const std::vector<std::string> expected = expectedDataFromSource(name);

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

BOOST_AUTO_TEST_SUITE_END()

bool initMasterSuite()
{
  boost::unit_test::framework::master_test_suite().p_name.value = "Binlog Integration Test";

  // Boost.Test only allows custom args after the -- separator
  int argc = boost::unit_test::framework::master_test_suite().argc;
  char** argv = boost::unit_test::framework::master_test_suite().argv;

  g_bread_path = (argc > 1) ? argv[1] : "./bread";
  g_inttest_dir = (argc > 2) ? argv[2] + std::string("/") : "./";
  g_src_dir = (argc > 3) ? argv[3] + std::string("/test/integration/") : "../test/integration/";

  return true;
}

int main(int argc, /*const*/ char* argv[])
{
  return boost::unit_test::unit_test_main(&initMasterSuite, argc, argv);
}
