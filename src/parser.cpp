#include "../include/parser.hpp"
#include <algorithm>
#include <fstream>

std::vector<Data> Parser::parse(const std::string &filePath) {
  std::vector<Data> records;
  std::ifstream file(filePath);
  std::string line;

  std::getline(file, line); // remove the header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, '|');
    if (txt.size() < 43 || txt[9] != "Vente" || txt[10].empty())
      continue;
    try {
      Data d;
      d.propertyValue = parseDouble(txt[10]);

      // Drop obvious outliers before doing anything else.
      // Transactions below 10k seem symbolic (e.g.donations, family
      // transfers).
      // Above 2M we have too few samples and too much variance.
      if (d.propertyValue < 10000.0 || d.propertyValue > 2000000.0)
        continue;

      d.month = std::stoi(txt[8].substr(3, 2));
      d.year = std::stoi(txt[8].substr(6, 4));

      d.deptCode = txt[18];
      d.cityCode = txt[19];
      d.section = txt[21];
      d.streetCode = txt[14];

      d.localTypeCode = txt[35].empty() ? 0 : std::stoi(txt[35]);
      d.realBuiltArea = txt[38].empty() ? 0.0 : parseDouble(txt[38]);
      d.mainRooms = txt[39].empty() ? 0 : std::stoi(txt[39]);
      d.lotCount = txt[34].empty() ? 0 : std::stoi(txt[34]);

      d.totalCarrezArea = 0.0;
      const int carrezIdx[] = {25, 27, 29, 31, 33};
      for (int idx : carrezIdx) {
        if (!txt[idx].empty())
          d.totalCarrezArea += parseDouble(txt[idx]);
      }

      // Skip built properties that have no usable surface area at all.
      // They would just add noise, the model can't learn anything from them.
      if (d.localTypeCode > 0 && d.totalCarrezArea == 0.0 &&
          d.realBuiltArea == 0.0)
        continue;

      d.landArea = txt[42].empty() ? 0.0 : parseDouble(txt[42]);
      d.landType = txt[40];
      d.landTypeSpecial = txt[41];

      records.push_back(d);
    } catch (...) {
      continue;
    }
  }
  return records;
}

std::vector<std::string> Parser::split(const std::string &s, char c) {
  std::vector<std::string> tokens;
  size_t start = 0, end = s.find(c);
  while (end != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + 1;
    end = s.find(c, start);
  }
  tokens.push_back(s.substr(start));
  return tokens;
}

double Parser::parseDouble(std::string s) {
  std::replace(s.begin(), s.end(), ',', '.');
  return std::stod(s);
}
