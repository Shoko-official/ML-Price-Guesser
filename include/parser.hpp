#pragma once
#include "data.hpp"
#include <string>
#include <vector>


class Parser {
public:
  std::vector<Data> parse(const std::string &filePath);

private:
  std::vector<std::string> split(const std::string &s, char c);
  double parseDouble(std::string s);
};
