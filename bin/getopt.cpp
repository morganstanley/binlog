#include "getopt.hpp"

#include <cstring> // strchr
#include <iostream>

char* optarg = nullptr;
int optind = 1;

int getopt(int argc, /*const*/ char* argv[], const char* optstring)
{
  if (optind >= argc || argv[optind][0] != '-')
  {
    return -1; // no more options
  }

  const char opt = argv[optind][1];

  if (opt == '\0' || opt == '-') // opt is - or --, stop processing
  {
    ++optind;
    return -1;
  }

  const char* s = strchr(optstring, opt);
  if (s == nullptr)
  {
    std::cerr << argv[0] << ": invalid option -- '" << opt << "'\n";
    return '?';
  }

  if (s[1] == ':') // opt takes an argument
  {
    ++optind;
    if (optind >= argc)
    {
      std::cerr << argv[0] << ": option requires an argument -- '" << opt << "'\n";
      return '?';
    }

    optarg = argv[optind++];
  }

  return opt;
}
