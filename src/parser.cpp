#include "../include/parser.hpp"
#include <algorithm>
#include <fstream>

std::unordered_map<std::string, MacroData>
Parser::parseIndicateurs(const std::string &filePath) {
  std::unordered_map<std::string, MacroData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    // Real Etalab CSV:
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 10)
      continue;
    auto unquote = [](std::string s) -> std::string {
      if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
      return s;
    };
    try {
      std::string codeCommune = unquote(txt[1]); // idx 0 is row number
      std::string year = unquote(txt[2]);
      if (codeCommune.empty() || year.empty())
        continue;
      std::string key = codeCommune + "_" + year;
      MacroData md;
      std::string nbMut = unquote(txt[3]);
      std::string prixM2 = unquote(txt[9]);
      md.nbMutations = nbMut.empty() ? 0 : std::stoi(nbMut);
      md.prixM2Moyen = prixM2.empty() ? 0.0 : parseDouble(prixM2);
      map[key] = md;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, double>
Parser::parseInseeRevenus(const std::string &filePath) {
  std::unordered_map<std::string, double> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    // 1: Code géographique, 7: [DISP] Médiane (€)
    std::vector<std::string> txt = split(line, ';');
    if (txt.size() < 8)
      continue;
    try {
      std::string codeCommune = txt[1];
      if (!txt[7].empty()) {
        map[codeCommune] = parseDouble(txt[7]);
      }
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::MarketData>
Parser::parseMarket(const std::string &filePath) {
  std::unordered_map<std::string, MarketData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line);

  while (std::getline(file, line)) {
    // market_indicators.csv uses semicolon and has NO leading empty column
    std::vector<std::string> txt = split(line, ';');
    if (txt.size() < 9)
      continue;
    try {
      std::string codeCommune = txt[0];
      std::string year = txt[1];
      if (codeCommune.empty() || year.empty())
        continue;
      std::string key = codeCommune + "_" + year;

      MarketData md;
      md.priceM2 = txt[8].empty() ? 0.0 : parseDouble(txt[8]);
      md.volume = txt[2].empty() ? 0 : std::stoi(txt[2]);
      map[key] = md;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::BdnbData>
Parser::parseBdnb(const std::string &filePath) {
  std::unordered_map<std::string, BdnbData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    // Expected:
    // code_commune,section,annee_construction,dpe,hauteur,nb_logements
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 6)
      continue;
    try {
      std::string codeCommune = txt[0];
      std::string section = txt[1];
      if (section.size() == 1)
        section = "0" + section;

      std::string key = codeCommune + "_" + section;
      BdnbData bd;
      bd.buildYear = txt[2].empty() ? 0 : std::stoi(txt[2]);
      bd.dpe = txt[3];
      bd.height = txt[4].empty() ? 0.0 : parseDouble(txt[4]);
      bd.unitCount = txt[5].empty() ? 0 : std::stoi(txt[5]);
      map[key] = bd;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::RiskData>
Parser::parseGeorisques(const std::string &filePath) {
  std::unordered_map<std::string, RiskData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 3)
      continue;
    try {
      std::string codeCommune = txt[0];
      RiskData rd;
      rd.inundation = txt[1].empty() ? 0 : std::stoi(txt[1]);
      rd.clay = txt[2].empty() ? 0 : std::stoi(txt[2]);
      map[codeCommune] = rd;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::SocioData>
Parser::parseSocio(const std::string &filePath) {
  std::unordered_map<std::string, SocioData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 3)
      continue;
    try {
      std::string codeCommune = txt[0];
      SocioData sd;
      sd.povertyRate = txt[1].empty() ? 0.0 : parseDouble(txt[1]);
      sd.density = txt[2].empty() ? 0.0 : parseDouble(txt[2]);
      map[codeCommune] = sd;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::PoiData>
Parser::parsePoi(const std::string &filePath) {
  std::unordered_map<std::string, PoiData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 5)
      continue;
    try {
      std::string codeCommune = txt[0];
      PoiData pd;
      pd.distSchool = txt[1].empty() ? 0.0 : parseDouble(txt[1]);
      pd.distHospital = txt[2].empty() ? 0.0 : parseDouble(txt[2]);
      pd.distSupermarket = txt[3].empty() ? 0.0 : parseDouble(txt[3]);
      pd.distPublicTransport = txt[4].empty() ? 0.0 : parseDouble(txt[4]);
      map[codeCommune] = pd;
    } catch (...) {
      continue;
    }
  }
  return map;
}

std::unordered_map<std::string, Parser::HyperLocalData>
Parser::parseHyperLocal(const std::string &filePath) {
  std::unordered_map<std::string, HyperLocalData> map;
  std::ifstream file(filePath);
  if (!file.is_open())
    return map;

  std::string line;
  std::getline(file, line); // header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, ',');
    if (txt.size() < 8)
      continue;
    try {
      std::string codeCommune = txt[0];
      std::string section = txt[1];
      if (section.size() == 1)
        section = "0" + section;

      std::string key = codeCommune + "_" + section;
      HyperLocalData hd;
      hd.wealthGapIndex = parseDouble(txt[2]);
      hd.lat = parseDouble(txt[3]);
      hd.lon = parseDouble(txt[4]);
      hd.greenScore = parseDouble(txt[5]);
      hd.noiseIndex = parseDouble(txt[6]);
      hd.serviceDensity = parseDouble(txt[7]);
      map[key] = hd;
    } catch (...) {
      continue;
    }
  }
  return map;
}
Parser::RollingPrices
Parser::computeRollingPrices(const std::vector<std::string> &filePaths,
                             int windowMonths) {
  // Light sale struct for building temporal price histories
  struct LightSale {
    std::string sectionKey;  // cityCode_section
    std::string buildingKey; // cityCode_street_parcel
    std::string typeKey;     // cityCode_section_localType
    std::string cityKey;     // cityCode (Option B)
    std::string exactKey; // cityCode_street_section_parcel_type_area (Option C)
    int timeIndex;        // months since Jan 2014
    double pricePerM2;
  };
  std::vector<LightSale> sales;
  sales.reserve(5000000);

  for (const auto &path : filePaths) {
    std::ifstream file(path);
    if (!file.is_open())
      continue;
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
      auto txt = split(line, '|');
      if (txt.size() < 43 || txt[9] != "Vente" || txt[10].empty())
        continue;
      try {
        double propValue = parseDouble(txt[10]);
        if (propValue < 25000.0 || propValue > 1200000.0)
          continue;

        std::string deptCode = txt[18];
        std::string cityCode = deptCode + txt[19];
        std::string dateStr = txt[8];
        int month = std::stoi(dateStr.substr(3, 2));
        int year = std::stoi(dateStr.substr(6, 4));

        std::string section = txt[21];
        if (section.size() == 1)
          section = "0" + section;

        double area = 0.0;
        const int carrezIdx[] = {25, 27, 29, 31, 33};
        for (int idx : carrezIdx)
          if (!txt[idx].empty())
            area += parseDouble(txt[idx]);
        if (area <= 0.0 && !txt[38].empty())
          area = parseDouble(txt[38]);
        if (area < 9.0)
          continue;

        double pricePerM2 = propValue / area;
        if (pricePerM2 < 500.0 || pricePerM2 > 20000.0)
          continue;

        std::string sectionKey = cityCode + "_" + section;
        std::string buildingKey = cityCode + "_" + txt[14] + "_" + txt[12];
        std::string typeKey = cityCode + "_" + section + "_" + txt[35];
        std::string cityKey = cityCode;

        // Clé ultra-précise : Ville + Rue + Section + Parcelle + TypeLocal +
        // Surface
        std::string surfaceStr = txt[38].empty() ? "0" : txt[38];
        std::string exactKey = cityCode + "_" + txt[14] + "_" + section + "_" +
                               txt[12] + "_" + txt[35] + "_" + surfaceStr;

        int timeIndex = (year - 2014) * 12 + (month - 1);

        sales.push_back({std::move(sectionKey), std::move(buildingKey),
                         std::move(typeKey), std::move(cityKey),
                         std::move(exactKey), timeIndex, pricePerM2});
      } catch (...) {
        continue;
      }
    }
  }

  auto buildHistory = [&](auto getKey) {
    std::unordered_map<std::string, std::vector<std::pair<int, double>>> hist;
    for (const auto &s : sales) {
      hist[getKey(s)].emplace_back(s.timeIndex, s.pricePerM2);
    }
    for (auto &e : hist)
      std::sort(e.second.begin(), e.second.end());
    return hist;
  };

  auto computeIndexedMap =
      [&](const std::unordered_map<std::string,
                                   std::vector<std::pair<int, double>>> &hist,
          int window) {
        std::unordered_map<std::string, double> res;
        for (const auto &entry : hist) {
          const auto &vec = entry.second;
          for (const auto &pt : vec) {
            int ti = pt.first;
            std::string lookupKey = entry.first + "_" + std::to_string(ti);
            if (res.count(lookupKey))
              continue;
            auto lo = std::lower_bound(vec.begin(), vec.end(),
                                       std::make_pair(ti - window, -1e18));
            auto hi = std::lower_bound(vec.begin(), vec.end(),
                                       std::make_pair(ti, -1e18));
            double sum = 0.0;
            int cnt = 0;
            for (auto it = lo; it != hi; ++it) {
              sum += it->second;
              ++cnt;
            }
            res[lookupKey] = (cnt > 0) ? (sum / cnt) : 0.0;
          }
        }
        return res;
      };

  auto sectHist = buildHistory([](const LightSale &s) { return s.sectionKey; });
  auto buildHist =
      buildHistory([](const LightSale &s) { return s.buildingKey; });
  auto typeHist = buildHistory([](const LightSale &s) { return s.typeKey; });
  auto cityHist = buildHistory([](const LightSale &s) { return s.cityKey; });
  auto exactHist = buildHistory([](const LightSale &s) { return s.exactKey; });

  RollingPrices rp;
  rp.sectionPrices = computeIndexedMap(sectHist, 18);
  rp.sectionPrices6m = computeIndexedMap(sectHist, 6);
  rp.buildingPrices = computeIndexedMap(buildHist, 18);
  rp.typeSectionPrices = computeIndexedMap(typeHist, 18);
  rp.cityPrices = computeIndexedMap(cityHist, 18);
  rp.exactSaleHistory = std::move(exactHist);
  return rp;
}

std::vector<Data> Parser::parse(
    const std::string &filePath,
    const std::unordered_map<std::string, MacroData> &macroMap,
    const std::unordered_map<std::string, double> &inseeMap,
    const std::unordered_map<std::string, BdnbData> &bdnbMap,
    const std::unordered_map<std::string, RiskData> &riskMap,
    const std::unordered_map<std::string, SocioData> &socioMap,
    const std::unordered_map<std::string, PoiData> &poiMap,
    const std::unordered_map<std::string, MarketData> &marketMap,
    const std::unordered_map<std::string, HyperLocalData> &hyperLocalMap,
    const RollingPrices &rollingPrices) {

  std::unordered_map<std::string, Data> transactionGroups;

  std::ifstream file(filePath);
  if (!file.is_open())
    return {};

  std::string line;
  std::getline(file, line); // remove the header

  while (std::getline(file, line)) {
    std::vector<std::string> txt = split(line, '|');
    if (txt.size() < 43 || txt[9] != "Vente" || txt[10].empty())
      continue;
    try {
      double propValue = parseDouble(txt[10]);
      if (propValue < 25000.0 || propValue > 1200000.0)
        continue;

      std::string deptCode = txt[18];
      std::string cityCode = deptCode + txt[19];
      std::string dateStr = txt[8];

      std::string txKey = cityCode + "_" + dateStr + "_" + txt[10];

      Data d;
      d.propertyValue = propValue;
      d.month = std::stoi(dateStr.substr(3, 2));
      d.year = std::stoi(dateStr.substr(6, 4));
      d.deptCode = deptCode;
      d.cityCode = cityCode;
      d.section = txt[21];
      if (d.section.size() == 1)
        d.section = "0" + d.section;
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
      // Compute usable area and price/m² right here
      d.totalArea =
          (d.totalCarrezArea > 0) ? d.totalCarrezArea : d.realBuiltArea;
      d.pricePerM2 = (d.totalArea >= 9.0) ? (propValue / d.totalArea) : 0.0;

      if (transactionGroups.count(txKey)) {
        Data &existing = transactionGroups[txKey];
        existing.realBuiltArea += d.realBuiltArea;
        existing.totalCarrezArea += d.totalCarrezArea;
        existing.mainRooms += d.mainRooms;
        existing.lotCount += d.lotCount;
        existing.localTypeCode =
            std::max(existing.localTypeCode, d.localTypeCode);
        continue;
      }
      d.landArea = txt[42].empty() ? 0.0 : parseDouble(txt[42]);
      d.landType = txt[40];
      d.landTypeSpecial = txt[41];

      std::string macroKey = d.cityCode + "_" + std::to_string(d.year);
      auto itMacro = macroMap.find(macroKey);
      if (itMacro != macroMap.end()) {
        d.macroPrixM2Moyen = itMacro->second.prixM2Moyen;
        d.macroNbMutations = itMacro->second.nbMutations;
      } else {
        d.macroPrixM2Moyen = 0.0;
        d.macroNbMutations = 0;
      }

      auto itInsee = inseeMap.find(d.cityCode);
      d.macroMedianIncome = (itInsee != inseeMap.end()) ? itInsee->second : 0.0;

      std::string bdnbKey = d.cityCode + "_" + d.section;
      auto itBdnb = bdnbMap.find(bdnbKey);
      if (itBdnb != bdnbMap.end()) {
        d.bdnbBuildYear = itBdnb->second.buildYear;
        d.bdnbDpeStatus = itBdnb->second.dpe;
        d.bdnbHeight = itBdnb->second.height;
        d.bdnbUnitCount = itBdnb->second.unitCount;
      } else {
        d.bdnbBuildYear = 0;
        d.bdnbDpeStatus = "Unknown";
        d.bdnbHeight = 0.0;
        d.bdnbUnitCount = 0;
      }

      auto itRisk = riskMap.find(d.cityCode);
      if (itRisk != riskMap.end()) {
        d.riskInundation = itRisk->second.inundation;
        d.riskClay = itRisk->second.clay;
      } else {
        d.riskInundation = 0;
        d.riskClay = 0;
      }

      auto itSocio = socioMap.find(d.cityCode);
      if (itSocio != socioMap.end()) {
        d.socioPovertyRate = itSocio->second.povertyRate;
        d.socioDensity = itSocio->second.density;
      } else {
        d.socioPovertyRate = 0.0;
        d.socioDensity = 0.0;
      }

      auto itPoi = poiMap.find(d.cityCode);
      if (itPoi != poiMap.end()) {
        d.distSchool = itPoi->second.distSchool;
        d.distHospital = itPoi->second.distHospital;
        d.distSupermarket = itPoi->second.distSupermarket;
        d.distPublicTransport = itPoi->second.distPublicTransport;
      } else {
        d.distSchool = 0.0;
        d.distHospital = 0.0;
        d.distSupermarket = 0.0;
        d.distPublicTransport = 0.0;
      }

      auto itMarket = marketMap.find(macroKey);
      if (itMarket != marketMap.end()) {
        d.marketPriceM2 = itMarket->second.priceM2;
        d.marketVolume = itMarket->second.volume;
      } else {
        d.marketPriceM2 = 0.0;
        d.marketVolume = 0;
      }

      // Rolling section price/m2 (past 18/6 months, same section)
      // KEY FIX: lookup by sectionKey + "_" + timeIndex (not txKey)
      std::string sectionKey = d.cityCode + "_" + d.section;
      std::string buildingKey = d.cityCode + "_" + d.streetCode + "_" + txt[12];
      std::string typeKey = d.cityCode + "_" + d.section + "_" + txt[35];
      std::string cityKey = d.cityCode;

      std::string surfaceStr = txt[38].empty() ? "0" : txt[38];
      std::string exactKey = d.cityCode + "_" + txt[14] + "_" + d.section +
                             "_" + txt[12] + "_" + txt[35] + "_" + surfaceStr;

      int timeIndex = (d.year - 2014) * 12 + (d.month - 1);
      std::string tiStr = std::to_string(timeIndex);

      auto rollingLookup =
          [&](const std::unordered_map<std::string, double> &map,
              const std::string &key) -> double {
        auto it = map.find(key + "_" + tiStr);
        return (it != map.end()) ? it->second : 0.0;
      };

      d.sectionPriceM2 = rollingLookup(rollingPrices.sectionPrices, sectionKey);
      d.buildingPriceM2 =
          rollingLookup(rollingPrices.buildingPrices, buildingKey);
      d.typeRollingPriceM2 =
          rollingLookup(rollingPrices.typeSectionPrices, typeKey);
      d.cityPriceM2 = rollingLookup(rollingPrices.cityPrices, cityKey);

      double p6m = rollingLookup(rollingPrices.sectionPrices6m, sectionKey);
      if (p6m == 0.0)
        p6m = d.sectionPriceM2;
      d.priceMomentum = (d.sectionPriceM2 > 0) ? (p6m / d.sectionPriceM2) : 1.0;

      // Option C : Extract last exact sale
      d.lastSalePriceM2 = 0.0;
      d.monthsSinceLastSale = -1;
      auto itHist = rollingPrices.exactSaleHistory.find(exactKey);
      if (itHist != rollingPrices.exactSaleHistory.end()) {
        const auto &vec = itHist->second;
        // Find the first sale whose timeIndex is >= current timeIndex
        auto it = std::lower_bound(vec.begin(), vec.end(),
                                   std::make_pair(timeIndex, -1e18));
        // If there is a sale strictly before the current one
        if (it != vec.begin()) {
          --it;
          d.lastSalePriceM2 = it->second;
          d.monthsSinceLastSale = timeIndex - it->first;
        }
      }

      // Fallbacks en cascade : macro <- city <- section <- building/type
      if (d.cityPriceM2 == 0.0)
        d.cityPriceM2 = d.macroPrixM2Moyen;
      if (d.sectionPriceM2 == 0.0)
        d.sectionPriceM2 = d.cityPriceM2;
      if (d.buildingPriceM2 == 0.0)
        d.buildingPriceM2 = d.sectionPriceM2;
      if (d.typeRollingPriceM2 == 0.0)
        d.typeRollingPriceM2 = d.sectionPriceM2;

      std::string hyperKey = d.cityCode + "_" + d.section;
      auto itHyper = hyperLocalMap.find(hyperKey);
      if (itHyper != hyperLocalMap.end()) {
        d.wealthGapIndex = itHyper->second.wealthGapIndex;
        d.lat = itHyper->second.lat;
        d.lon = itHyper->second.lon;
        d.greenScore = itHyper->second.greenScore;
        d.noiseIndex = itHyper->second.noiseIndex;
        d.serviceDensity = itHyper->second.serviceDensity;
      } else {
        d.wealthGapIndex = 1.0;
        d.lat = 0.0;
        d.lon = 0.0;
        d.greenScore = 0.0;
        d.noiseIndex = 0.0;
        d.serviceDensity = 0.0;
      }

      transactionGroups[txKey] = d;
    } catch (...) {
      continue;
    }
  }

  std::vector<Data> records;
  records.reserve(transactionGroups.size());
  for (auto const &pair : transactionGroups) {
    const auto &data = pair.second;
    if (data.localTypeCode > 0 && data.totalCarrezArea == 0.0 &&
        data.realBuiltArea == 0.0)
      continue;
    records.push_back(data);
  }
  return records;
}

std::vector<Data> Parser::parseAll(
    const std::vector<std::string> &filePaths,
    const std::unordered_map<std::string, MacroData> &macroMap,
    const std::unordered_map<std::string, double> &inseeMap,
    const std::unordered_map<std::string, BdnbData> &bdnbMap,
    const std::unordered_map<std::string, RiskData> &riskMap,
    const std::unordered_map<std::string, SocioData> &socioMap,
    const std::unordered_map<std::string, PoiData> &poiMap,
    const std::unordered_map<std::string, MarketData> &marketMap,
    const std::unordered_map<std::string, HyperLocalData> &hyperLocalMap,
    const RollingPrices &rollingPrices) {
  std::vector<Data> allRecords;
  for (const auto &path : filePaths) {
    auto records = parse(path, macroMap, inseeMap, bdnbMap, riskMap, socioMap,
                         poiMap, marketMap, hyperLocalMap, rollingPrices);
    allRecords.insert(allRecords.end(), records.begin(), records.end());
  }
  return allRecords;
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
