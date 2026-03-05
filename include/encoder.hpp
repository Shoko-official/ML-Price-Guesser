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
  int lookup(const std::string &label) const;
  int size() const;
  void reset();
};

class TargetEncoder {
private:
  std::unordered_map<std::string, double> mapping;
  double globalMean = 0.0;
  double smoothing;

public:
  TargetEncoder(double smoothing = 10.0) : smoothing(smoothing) {}
  void fit(const std::vector<std::string> &labels,
           const std::vector<double> &targets);
  double transform(const std::string &label) const;
};

struct EncodeData {
  double target;             // value
  double weight;             // sample weight
  std::vector<double> feats; // categorical vars in number
};

class Encoder {
private:
  TargetEncoder landEnc, specLandEnc, dpeEnc;
  TargetEncoder deptEnc, cityEnc, sectEnc, streetEnc, deptTypeEnc;

public:
  void fit(const std::vector<Data> &rawData);
  std::vector<EncodeData> transform(const std::vector<Data> &rawData) const;
};
