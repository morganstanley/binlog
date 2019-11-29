#include <binlog/binlog.hpp>

#include <iostream>

int main()
{
  BINLOG_INFO_C(my_category, "This is a categorized event");
  // Outputs: my_category  This is a categorized event

  binlog::SessionWriter writer(binlog::default_session());
  writer.setName("W1");

  BINLOG_INFO_WC(writer, my_category, "My writer, my category");
  // Outputs: my_category W1 My writer, my category

  binlog::consume(std::cout);
  return 0;
}
