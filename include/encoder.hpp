#pragma once
#include "data.hpp"
#include <string>
#include <unordered_map>
#include <vector>


class LabelEncode {
private:
  std::unordered_map<std::string, int> mapping;
  int nextId = 0;

public:
  int encode(const std::string &label);
  int size() const;
  void reset();
};

struct EncodeData {
  double target;             // value
  std::vector<double> feats; // categorical vars in number
};

class Encoder {
public:
  std::vector<EncodeData> transform(const std::vector<Data> &rawData);
};
