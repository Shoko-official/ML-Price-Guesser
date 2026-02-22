#include "../include/encoder.hpp"

int LabelEncode::encode(const std::string &label) {
  if (label.empty()) {
    return -1;
  }
  auto iteration = mapping.find(label);
  if (iteration == mapping.end()) {
    return mapping[label] = nextId++;
  }
  return iteration->second;
}

int LabelEncode::size() const { return mapping.size(); }

void LabelEncode::reset() {
  mapping.clear();
  nextId = 0;
}

std::vector<EncodeData> Encoder::transform(const std::vector<Data> &rawData) {
  std::vector<EncodeData> processed;
  processed.reserve(rawData.size());

  LabelEncode deptEnc, cityEnc, sectEnc, streetEnc, landEnc, specLandEnc;

  for (const auto &d : rawData) {
    EncodeData ed;
    ed.target = d.propertyValue;

    // Numeric feats
    ed.feats.push_back(static_cast<double>(d.year));
    ed.feats.push_back(static_cast<double>(d.month));
    ed.feats.push_back(static_cast<double>(d.localTypeCode));
    ed.feats.push_back(d.totalCarrezArea);
    ed.feats.push_back(d.realBuiltArea);
    ed.feats.push_back(static_cast<double>(d.mainRooms));
    ed.feats.push_back(static_cast<double>(d.lotCount));
    ed.feats.push_back(d.landArea);

    // Categorical feats
    ed.feats.push_back(static_cast<double>(deptEnc.encode(d.deptCode)));
    ed.feats.push_back(static_cast<double>(cityEnc.encode(d.cityCode)));
    ed.feats.push_back(static_cast<double>(sectEnc.encode(d.section)));
    ed.feats.push_back(static_cast<double>(streetEnc.encode(d.streetCode)));
    ed.feats.push_back(static_cast<double>(landEnc.encode(d.landType)));
    ed.feats.push_back(
        static_cast<double>(specLandEnc.encode(d.landTypeSpecial)));

    processed.push_back(ed);
  }
  return processed;
}
