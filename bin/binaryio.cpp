#ifdef _WIN32

#include <io.h>    // _setmode
#include <fcntl.h> // _O_BINARY
#include <stdio.h> // _fileno

namespace {

// By default, stdin is opened in text mode,
// and reading stops at 1A. Change the mode
// to binary:
const int g_stdin_is_binary = _setmode(_fileno(stdin), _O_BINARY);

// By default, stdout is opened in text mode,
// and 0A is replaced with 0D 0A. Change
// the mode to binary:
const int g_stdout_is_binary = _setmode(_fileno(stdout), _O_BINARY);

} // namespace

#endif // _WIN32