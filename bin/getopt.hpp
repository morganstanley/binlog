#ifndef BINLOG_BIN_GETOPT_HPP
#define BINLOG_BIN_GETOPT_HPP

#ifdef _WIN32

extern char* optarg;
extern int optind;

/** Implements a subset of POSIX getopt, only what bread needs */
int getopt(int argc, /*const*/ char* argv[], const char* optstring);

#else // assume POSIX

#include <getopt.h>

#endif

#endif // BINLOG_BIN_GETOPT_HPP
