#include "../include/encoder.hpp"
#include <cmath>

void TargetEncoder::fit(const std::vector<std::string> &labels,
                        const std::vector<double> &targets) {
  if (labels.empty())
    return;

  std::unordered_map<std::string, std::pair<double, int>> stats;
  double sum = 0.0;
  for (size_t i = 0; i < labels.size(); ++i) {
    auto &s = stats[labels[i]];
    s.first += targets[i];
    s.second++;
    sum += targets[i];
  }

  globalMean = sum / static_cast<double>(labels.size());

  for (auto const &entry : stats) {
    const std::pair<double, int> &stat = entry.second;
    double mean = stat.first / static_cast<double>(stat.second);
    int count = stat.second;
    // Smoothed target encoding: (mean * count + globalMean * smoothing) /
    // (count + smoothing)
    mapping[entry.first] =
        (mean * count + globalMean * smoothing) / (count + smoothing);
  }
}

double TargetEncoder::transform(const std::string &label) const {
  auto it = mapping.find(label);
  if (it != mapping.end()) {
    return it->second;
  }
  return globalMean;
}

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

int LabelEncode::lookup(const std::string &label) const {
  auto it = mapping.find(label);
  if (it != mapping.end()) {
    return it->second;
  }
  return -1;
}

int LabelEncode::size() const { return static_cast<int>(mapping.size()); }

void LabelEncode::reset() {
  mapping.clear();
  nextId = 0;
}

void Encoder::fit(const std::vector<Data> &rawData) {
  std::vector<double> targets;
  targets.reserve(rawData.size());
  std::vector<std::string> depts, cities, sections, streets, deptTypes;
  std::vector<std::string> lands, specLands, dpes;
  depts.reserve(rawData.size());
  cities.reserve(rawData.size());
  sections.reserve(rawData.size());
  streets.reserve(rawData.size());
  deptTypes.reserve(rawData.size());
  lands.reserve(rawData.size());
  specLands.reserve(rawData.size());
  dpes.reserve(rawData.size());

  for (const auto &d : rawData) {
    targets.push_back(d.propertyValue);
    depts.push_back(d.deptCode);
    cities.push_back(d.cityCode);
    sections.push_back(d.cityCode + "_" + d.section);
    streets.push_back(d.cityCode + "_" + d.streetCode);
    deptTypes.push_back(d.deptCode + "_" + std::to_string(d.localTypeCode));

    lands.push_back(d.landType);
    specLands.push_back(d.landTypeSpecial);
    dpes.push_back(d.bdnbDpeStatus);
  }

  deptEnc.fit(depts, targets);
  cityEnc.fit(cities, targets);
  sectEnc.fit(sections, targets);
  streetEnc.fit(streets, targets);
  deptTypeEnc.fit(deptTypes, targets);

  landEnc.fit(lands, targets);
  specLandEnc.fit(specLands, targets);
  dpeEnc.fit(dpes, targets);
}

std::vector<EncodeData>
Encoder::transform(const std::vector<Data> &rawData) const {
  std::vector<EncodeData> processed;
  processed.reserve(rawData.size());

  for (const auto &d : rawData) {
    EncodeData ed;
    ed.target = d.pricePerM2; // log(pricePerM2) applied in main.cpp

    // Temporal weight: Scale: 2020 -> 0.4, 2021 -> 0.6, 2022 -> 0.8, 2023
    // -> 1.0.. so on and so forth
    double wScale = 0.4 + (std::max(0, d.year - 2020)) * 0.2;
    ed.weight = std::min(1.0, wScale);

    // Numeric feats
    ed.feats.push_back(static_cast<double>(d.year));
    ed.feats.push_back(static_cast<double>(d.month));
    ed.feats.push_back(static_cast<double>(d.localTypeCode));
    ed.feats.push_back(d.totalCarrezArea);
    ed.feats.push_back(d.realBuiltArea);
    ed.feats.push_back(static_cast<double>(d.mainRooms));
    ed.feats.push_back(static_cast<double>(d.lotCount));
    ed.feats.push_back(d.landArea);

    // Advanced features
    double surfacePerRoom =
        (d.mainRooms > 0) ? (d.realBuiltArea / d.mainRooms) : d.realBuiltArea;
    ed.feats.push_back(surfacePerRoom);

    double timeIndex =
        static_cast<double>((d.year - 2020) * 12 + (d.month - 1));
    ed.feats.push_back(timeIndex);

    double relativeDensity =
        (d.landArea > 0) ? (d.realBuiltArea / d.landArea) : 0.0;
    ed.feats.push_back(relativeDensity);

    // Geographic feats (Target Encoded)
    ed.feats.push_back(deptEnc.transform(d.deptCode));
    ed.feats.push_back(cityEnc.transform(d.cityCode));
    ed.feats.push_back(sectEnc.transform(d.cityCode + "_" + d.section));
    ed.feats.push_back(streetEnc.transform(d.cityCode + "_" + d.streetCode));

    std::string deptType = d.deptCode + "_" + std::to_string(d.localTypeCode);
    ed.feats.push_back(deptTypeEnc.transform(deptType));

    // Other categorical feats (Label Encoded)
    ed.feats.push_back(landEnc.transform(d.landType));
    ed.feats.push_back(specLandEnc.transform(d.landTypeSpecial));

    // Macro indicators
    ed.feats.push_back(d.macroPrixM2Moyen);
    ed.feats.push_back(static_cast<double>(d.macroNbMutations));
    ed.feats.push_back(d.macroMedianIncome);

    // BDNB Features
    ed.feats.push_back(static_cast<double>(d.bdnbBuildYear));
    ed.feats.push_back(dpeEnc.transform(d.bdnbDpeStatus));
    ed.feats.push_back(d.bdnbHeight);
    ed.feats.push_back(static_cast<double>(d.bdnbUnitCount));

    // Georisques Features
    ed.feats.push_back(static_cast<double>(d.riskInundation));
    ed.feats.push_back(static_cast<double>(d.riskClay));

    // Socio-Eco Features
    ed.feats.push_back(d.socioPovertyRate);
    ed.feats.push_back(d.socioDensity);

    // OSM POI Features
    ed.feats.push_back(d.distSchool);
    ed.feats.push_back(d.distHospital);
    ed.feats.push_back(d.distSupermarket);
    ed.feats.push_back(d.distPublicTransport);

    // Market Trend Features
    ed.feats.push_back(d.marketPriceM2);
    ed.feats.push_back(static_cast<double>(d.marketVolume));

    // Rolling section/building price/m2 (past 18 months) — KEY features
    ed.feats.push_back(d.sectionPriceM2);
    ed.feats.push_back(d.buildingPriceM2);
    ed.feats.push_back(d.priceMomentum);
    ed.feats.push_back(d.typeRollingPriceM2);

    // Hyper-Local Neighborhood Features
    ed.feats.push_back(d.wealthGapIndex);
    ed.feats.push_back(d.lat);
    ed.feats.push_back(d.lon);
    ed.feats.push_back(d.greenScore);
    ed.feats.push_back(d.noiseIndex);
    ed.feats.push_back(d.serviceDensity);

    // Feature interactions: Estimated Base prices
    double area = d.totalArea > 0 ? d.totalArea
                                  : (d.totalCarrezArea > 0 ? d.totalCarrezArea
                                                           : d.realBuiltArea);
    double expectedPrice = d.macroPrixM2Moyen * area;
    ed.feats.push_back(expectedPrice);
    // Rolling expected prices are highly predictive
    double rollingExpectedSect = d.sectionPriceM2 * area;
    double rollingExpectedBuild = d.buildingPriceM2 * area;
    ed.feats.push_back(rollingExpectedSect);
    ed.feats.push_back(rollingExpectedBuild);

    // Ratio: how does the local rolling price compare to the macro market?
    // This is VERY informative for the residual the model needs to predict.
    double relToMacro = (d.macroPrixM2Moyen > 0)
                            ? (d.sectionPriceM2 / d.macroPrixM2Moyen)
                            : 1.0;
    ed.feats.push_back(relToMacro);

    // Carrez / real built area ratio — proxy for apartment vs house /
    // efficiency
    double carrezRatio = (d.realBuiltArea > 0 && d.totalCarrezArea > 0)
                             ? (d.totalCarrezArea / d.realBuiltArea)
                             : 1.0;
    ed.feats.push_back(carrezRatio);

    // Log-transformed features: these are linearly correlated with
    // log(price/m²) and dramatically reduce the work needed by the trees.
    auto safeLog = [](double x) { return (x > 0.0) ? std::log(x) : 0.0; };
    ed.feats.push_back(safeLog(d.sectionPriceM2));     // log €/m² section
    ed.feats.push_back(safeLog(d.buildingPriceM2));    // log €/m² building
    ed.feats.push_back(safeLog(d.typeRollingPriceM2)); // log €/m² type
    ed.feats.push_back(safeLog(d.cityPriceM2)); // log €/m² city (Option B)

    // Option C features (Exact Sale History)
    bool hasLastSale = (d.lastSalePriceM2 > 0.0);
    ed.feats.push_back(hasLastSale ? 1.0 : 0.0); // Flag: "Has been sold before"
    ed.feats.push_back(
        hasLastSale
            ? safeLog(d.lastSalePriceM2)
            : safeLog(d.sectionPriceM2)); // The best predictor available
    ed.feats.push_back(hasLastSale ? static_cast<double>(d.monthsSinceLastSale)
                                   : -1.0); // Time elapsed since last sale

    ed.feats.push_back(safeLog(rollingExpectedSect));  // log expected € (sect)
    ed.feats.push_back(safeLog(rollingExpectedBuild)); // log expected € (build)
    ed.feats.push_back(safeLog(d.macroPrixM2Moyen));   // log macro €/m²
    ed.feats.push_back(safeLog(expectedPrice));        // log macro expected €
    ed.feats.push_back(safeLog(d.macroMedianIncome));  // log income
    ed.feats.push_back(safeLog(area));                 // log surface
    ed.feats.push_back(safeLog(relToMacro));           // log ratio to macro

    processed.push_back(ed);
  }
  return processed;
}
