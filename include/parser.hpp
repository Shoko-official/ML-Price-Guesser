#pragma once
#include "data.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct MacroData {
  double prixM2Moyen;
  int nbMutations;
};

class Parser {
public:
  std::unordered_map<std::string, MacroData>
  parseIndicateurs(const std::string &filePath);
  std::unordered_map<std::string, double>
  parseInseeRevenus(const std::string &filePath);

  struct BdnbData {
    int buildYear;
    std::string dpe;
    double height;
    int unitCount;
  };
  std::unordered_map<std::string, BdnbData>
  parseBdnb(const std::string &filePath);

  struct RiskData {
    int inundation;
    int clay;
  };
  std::unordered_map<std::string, RiskData>
  parseGeorisques(const std::string &filePath);

  struct SocioData {
    double povertyRate;
    double density;
  };
  std::unordered_map<std::string, SocioData>
  parseSocio(const std::string &filePath);

  struct PoiData {
    double distSchool;
    double distHospital;
    double distSupermarket;
    double distPublicTransport;
  };
  std::unordered_map<std::string, PoiData>
  parsePoi(const std::string &filePath);

  struct MarketData {
    double priceM2;
    int volume;
  };
  std::unordered_map<std::string, MarketData>
  parseMarket(const std::string &filePath);

  struct HyperLocalData {
    double wealthGapIndex;
    double lat, lon;
    double greenScore;
    double noiseIndex;
    double serviceDensity;
  };
  std::unordered_map<std::string, HyperLocalData>
  parseHyperLocal(const std::string &filePath);

  struct RollingPrices {
    std::unordered_map<std::string, double> sectionPrices;
    std::unordered_map<std::string, double> buildingPrices;
    size_t size() const { return sectionPrices.size(); }
  };

  // Rolling section/building price: for each transaction, the mean price/m2
  // over the previous 18 months.
  RollingPrices computeRollingPrices(const std::vector<std::string> &filePaths,
                                     int windowMonths = 18);

  std::vector<Data>
  parse(const std::string &filePath,
        const std::unordered_map<std::string, MacroData> &macroMap,
        const std::unordered_map<std::string, double> &inseeMap,
        const std::unordered_map<std::string, BdnbData> &bdnbMap,
        const std::unordered_map<std::string, RiskData> &riskMap,
        const std::unordered_map<std::string, SocioData> &socioMap,
        const std::unordered_map<std::string, PoiData> &poiMap,
        const std::unordered_map<std::string, MarketData> &marketMap,
        const std::unordered_map<std::string, HyperLocalData> &hyperLocalMap,
        const RollingPrices &rollingPrices);

  std::vector<Data>
  parseAll(const std::vector<std::string> &filePaths,
           const std::unordered_map<std::string, MacroData> &macroMap,
           const std::unordered_map<std::string, double> &inseeMap,
           const std::unordered_map<std::string, BdnbData> &bdnbMap,
           const std::unordered_map<std::string, RiskData> &riskMap,
           const std::unordered_map<std::string, SocioData> &socioMap,
           const std::unordered_map<std::string, PoiData> &poiMap,
           const std::unordered_map<std::string, MarketData> &marketMap,
           const std::unordered_map<std::string, HyperLocalData> &hyperLocalMap,
           const RollingPrices &rollingPrices);

private:
  std::vector<std::string> split(const std::string &s, char c);
  double parseDouble(std::string s);
};
