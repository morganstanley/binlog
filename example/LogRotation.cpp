#include <binlog/binlog.hpp>

#include <cstdio> // rename
#include <fstream>
#include <iostream>

int main()
{
  // log to rotate.blog
  BINLOG_INFO("Hello, first file");
  std::ofstream logfile("rotate.blog", std::ofstream::out|std::ofstream::binary);
  binlog::consume(logfile);

  if (! logfile)
  {
    std::cerr << "Failed to write rotate.blog\n";
    return 1;
  }

  //[rotate
  if (std::rename("rotate.blog", "rotate.1.blog") != 0)
  {
    std::cerr << "Failed to rename rotate.blog to rotate.1.blog\n";
    return 2;
  }

  logfile.close();
  logfile.open("rotate.blog", std::ofstream::out|std::ofstream::binary);

  // Metadata is now moved to rotate.1.blog, rotate.blog is empty.
  // To make rotate.blog stand alone, re-add metadata:
  binlog::default_session().reconsumeMetadata(logfile);
  //]

  // log to rotate.blog - a new file with the old name
  BINLOG_INFO("Hello, second file");
  binlog::consume(logfile);

  if (! logfile)
  {
    std::cerr << "Failed to write rotate.blog after rotation\n";
    return 3;
  }

  std::cout << "Binary log written to rotate.blog and rotate.1.blog\n";

  return 0;
}
