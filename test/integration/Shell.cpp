#include <binlog/binlog.hpp>

#include <mserialize/string_view.hpp>

#include <exception> // terminate
#include <iostream>
#include <string>

namespace {

binlog::SessionWriter w1(binlog::default_session());
binlog::SessionWriter w2(binlog::default_session());

bool prefixed(mserialize::string_view command, mserialize::string_view prefix, mserialize::string_view& suffix)
{
  if (! command.starts_with(prefix)) { return false; }

  suffix = command.substr(prefix.size());
  return true;
}

void log(binlog::SessionWriter& w, mserialize::string_view message)
{
  BINLOG_INFO_W(w, "{}", message);
}

void terminate()
{
  std::terminate();
}

void showHelp()
{
  std::cerr <<
    "Shell -- interactive test program using Binlog\n"
    "\n"
    "Reads commands from argv[1:] and standard input:\n"
    "\n"
    "  log w1 <msg>     Log <msg> using the first writer\n"
    "  log w2 <msg>     Log <msg> using the second writer\n"
    "  terminate        Forcefully terminate the application\n"
    "  help             Show this help\n"
    "\n"
    ;
}

void execute(mserialize::string_view command)
{
    mserialize::string_view args;

    if      (prefixed(command, "log w1 ", args)) { log(w1, args); }
    else if (prefixed(command, "log w2 ", args)) { log(w2, args); }
    else if (command == "terminate")             { terminate(); }
    else if (command == "help")                  { showHelp(); }
    else { std::cerr << "Unknown command " << command << "\n"; showHelp(); }
}

} // namespace

int main(int argc, const char* argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    execute(argv[i]);
  }

  std::cout << "> " << std::flush;

  std::string command;
  while (std::getline(std::cin, command))
  {
    // drop trailing \r on windows
    if (! command.empty() && command.back() == '\r') { command.pop_back(); }

    execute(command);
    std::cout << "> " << std::flush;
  }

  return 0;
}
