#pragma once
#include <string>

struct Data {
  // Target (both kept; pricePerM2 is the primary training target)
  double propertyValue;
  double pricePerM2; // propertyValue / usable area — primary training target
  double totalArea;  // usable surface (carrez if available, else realBuiltArea)

  // Temporal
  int year;
  int month;

  // Geographic
  std::string deptCode;
  std::string cityCode;
  std::string section;
  std::string streetCode;

  // Building features
  int localTypeCode;
  double totalCarrezArea;
  double realBuiltArea;
  int mainRooms;
  int lotCount;

  // Land features
  double landArea;
  std::string landType;
  std::string landTypeSpecial;

  // Macro features
  double macroPrixM2Moyen;
  int macroNbMutations;
  double macroMedianIncome; // from INSEE

  // Building morphology & state
  int bdnbBuildYear;         // annee_construction
  std::string bdnbDpeStatus; // dpe_classe_bilan_gse
  double bdnbHeight;         // hauteur_max
  int bdnbUnitCount;         // nb_logements

  // Environment
  int riskInundation; // Score of flood risk
  int riskClay;       // Score of clay risk

  // Detailed Socio-Eco (INSEE)
  double socioPovertyRate;
  double socioDensity;

  // OSM POIs (Distance in meters)
  double distSchool;
  double distHospital;
  double distSupermarket;
  double distPublicTransport;

  // Market Trends (Indicators)
  double marketPriceM2;
  int marketVolume;

  // Rolling historical price: median price/m2 of same section over past 18
  // months
  // Detailed market dynamics
  double sectionPriceM2;
  double buildingPriceM2;
  double cityPriceM2;        // Option B: Filet de sécurité spatial niveau ville
  double priceMomentum;      // Ratio 6m / 18m
  double typeRollingPriceM2; // Rolling price for same category (House/Apt)

  // Vente précédente du même bien (Option C)
  double lastSalePriceM2;
  int monthsSinceLastSale;

  // Neighborhood DNA
  double wealthGapIndex;
  double lat, lon;
  double greenScore;
  double noiseIndex;
  double serviceDensity;
};
