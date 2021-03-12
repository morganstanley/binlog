#ifndef TEST_UNIT_MSERIALIZE_TEST_STREAMS_HPP
#define TEST_UNIT_MSERIALIZE_TEST_STREAMS_HPP

#include <ios> // streamsize
#include <sstream>

// In tests, do not use std streams directly,
// to make sure tested code only accesses
// members of the specific concept.

struct OutputStream
{
  std::stringstream& stream;

  OutputStream& write(const char* buf, std::streamsize size)
  {
    stream.write(buf, size);
    return *this;
  }
};

struct InputStream
{
  std::stringstream& stream;

  InputStream& read(char* buf, std::streamsize size)
  {
    stream.read(buf, size);
    return *this;
  }
};

#endif // TEST_UNIT_MSERIALIZE_TEST_STREAMS_HPP
