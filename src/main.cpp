#include "../include/encoder.hpp"
#include "../include/model.hpp"
#include "../include/parser.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

void temporalSplit(const std::vector<Data> &allData,
                   std::vector<Data> &trainSet, std::vector<Data> &valSet,
                   std::vector<Data> &testSet) {
  for (const auto &d : allData) {
    if (d.year <= 2023) {
      trainSet.push_back(d);
    } else if (d.year == 2024) {
      valSet.push_back(d);
    } else {
      testSet.push_back(d);
    }
  }
}

int main() {
  try {
    const std::vector<std::string> dataFiles = {
        "data/ValeursFoncieres-2020-S2.txt",
        "data/ValeursFoncieres-2021.txt",
        "data/ValeursFoncieres-2022.txt",
        "data/ValeursFoncieres-2023.txt",
        "data/ValeursFoncieres-2024.txt",
        "data/ValeursFoncieres-2025-S1.txt"};

    std::cout << "Loading market indicators...\n";
    Parser parser;
    auto macroMap = parser.parseIndicateurs("data/indicateurs_marche_real.csv");
    std::cout << "  Loaded " << macroMap.size() << " macro keys.\n";

    std::cout << "Loading INSEE income data...\n";
    auto inseeMap = parser.parseInseeRevenus("data/insee_revenus.csv");
    std::cout << "  Loaded " << inseeMap.size() << " INSEE keys.\n";

    std::cout << "Loading BDNB indicators...\n";
    auto bdnbMap = parser.parseBdnb("data/bdnb_commune.csv");
    std::cout << "  Loaded " << bdnbMap.size() << " BDNB keys.\n";

    std::cout << "Loading Georisques indicators...\n";
    auto riskMap = parser.parseGeorisques("data/georisques_commune.csv");
    std::cout << "  Loaded " << riskMap.size() << " Georisques keys.\n";

    std::cout << "Loading Socio-Eco indicators...\n";
    auto socioMap = parser.parseSocio("data/socio_commune.csv");
    std::cout << "  Loaded " << socioMap.size() << " Socio-Eco keys.\n";

    std::cout << "Loading OSM POIs...\n";
    auto poiMap = parser.parsePoi("data/osm_pois.csv");
    std::cout << "  Loaded " << poiMap.size() << " POI keys.\n";

    auto marketMap = parser.parseMarket("data/market_indicators.csv");
    std::cout << "  Loaded " << marketMap.size() << " market keys.\n";

    auto hyperLocalMap =
        parser.parseHyperLocal("data/hyper_local_features.csv");
    std::cout << "  Loaded " << hyperLocalMap.size() << " hyper-local keys.\n";

    Parser::RollingPrices rollingPrices =
        parser.computeRollingPrices(dataFiles);
    std::cout << "  Computed " << rollingPrices.size()
              << " rolling price entries.\n";

    std::cout << "Parsing data from " << dataFiles.size() << " files...\n";
    std::vector<Data> rawRecords = parser.parseAll(
        dataFiles, macroMap, inseeMap, bdnbMap, riskMap, socioMap, poiMap,
        marketMap, hyperLocalMap, rollingPrices);
    std::cout << "  Loaded " << rawRecords.size() << " valid records.\n";

    if (rawRecords.empty()) {
      std::cerr << "Error: no records were loaded. Check the file path.\n";
      return 1;
    }

    std::cout << "Splitting dataset (temporal & by type)...\n";
    std::vector<Data> trainHouseRaw, valHouseRaw, testHouseRaw;
    std::vector<Data> trainAptRaw, valAptRaw, testAptRaw;

    for (const auto &d : rawRecords) {
      if (d.localTypeCode == 1) { // Maison
        if (d.year <= 2023)
          trainHouseRaw.push_back(d);
        else if (d.year == 2024)
          valHouseRaw.push_back(d);
        else
          testHouseRaw.push_back(d);
      } else if (d.localTypeCode == 2) { // Appartement
        if (d.year <= 2023)
          trainAptRaw.push_back(d);
        else if (d.year == 2024)
          valAptRaw.push_back(d);
        else
          testAptRaw.push_back(d);
      }
    }

    std::cout << "Houses:\n";
    std::cout << "  Train (2014-2023): " << trainHouseRaw.size()
              << " samples\n";
    std::cout << "  Val   (2024):      " << valHouseRaw.size() << " samples\n";
    std::cout << "  Test  (2025 S1):   " << testHouseRaw.size() << " samples\n";

    std::cout << "Apartments:\n";
    std::cout << "  Train (2014-2023): " << trainAptRaw.size() << " samples\n";
    std::cout << "  Val   (2024):      " << valAptRaw.size() << " samples\n";
    std::cout << "  Test  (2025 S1):   " << testAptRaw.size() << " samples\n";

    rawRecords.clear();

    auto applyLogPricePerM2 = [](std::vector<Data> &set) {
      set.erase(
          std::remove_if(set.begin(), set.end(),
                         [](const Data &d) { return d.pricePerM2 <= 0.0; }),
          set.end());
      for (auto &sample : set)
        sample.pricePerM2 = std::log(sample.pricePerM2);
    };

    applyLogPricePerM2(trainHouseRaw);
    applyLogPricePerM2(valHouseRaw);
    applyLogPricePerM2(testHouseRaw);

    applyLogPricePerM2(trainAptRaw);
    applyLogPricePerM2(valAptRaw);
    applyLogPricePerM2(testAptRaw);

    auto applyTargetEncoding = [](std::vector<Data> &trainRaw,
                                  std::vector<Data> &valRaw,
                                  std::vector<Data> &testRaw) {
      std::vector<EncodeData> finalTrain;
      finalTrain.reserve(trainRaw.size());

      int nFolds = 5;
      size_t foldSize = trainRaw.size() / nFolds;
      for (int i = 0; i < nFolds; ++i) {
        size_t start = i * foldSize;
        size_t end = (i == nFolds - 1) ? trainRaw.size() : start + foldSize;

        std::vector<Data> trainFold;
        trainFold.reserve(trainRaw.size() - (end - start));
        trainFold.insert(trainFold.end(), trainRaw.begin(),
                         trainRaw.begin() + start);
        trainFold.insert(trainFold.end(), trainRaw.begin() + end,
                         trainRaw.end());

        std::vector<Data> valFold(trainRaw.begin() + start,
                                  trainRaw.begin() + end);

        Encoder foldEncoder;
        foldEncoder.fit(trainFold);
        std::vector<EncodeData> encodedValFold = foldEncoder.transform(valFold);
        finalTrain.insert(finalTrain.end(), encodedValFold.begin(),
                          encodedValFold.end());
      }

      Encoder globalEncoder;
      globalEncoder.fit(trainRaw);
      std::vector<EncodeData> valSet = globalEncoder.transform(valRaw);
      std::vector<EncodeData> testSet = globalEncoder.transform(testRaw);

      trainRaw.clear();
      valRaw.clear();
      testRaw.clear();

      return std::make_tuple(std::move(finalTrain), std::move(valSet),
                             std::move(testSet));
    };

    std::cout << "Encoding House dataset...\n";
    auto [finalTrainHouse, valSetHouse, testSetHouse] =
        applyTargetEncoding(trainHouseRaw, valHouseRaw, testHouseRaw);

    std::cout << "Encoding Apartment dataset...\n";
    auto [finalTrainApt, valSetApt, testSetApt] =
        applyTargetEncoding(trainAptRaw, valAptRaw, testAptRaw);

    std::cout << "Training House GBDT...\n";
    GBDT modelHouse(8000, 0.03, 11, 250, 0.8, 15.0);
    modelHouse.fit(finalTrainHouse, valSetHouse, 150);
    std::cout << "Saving House model to model_house.bin...\n";
    modelHouse.save("model_house.bin");

    std::cout << "Training Apartment GBDT...\n";
    GBDT modelApt(8000, 0.03, 11, 250, 0.8, 15.0);
    modelApt.fit(finalTrainApt, valSetApt, 150);
    std::cout << "Saving Apartment model to model_apt.bin...\n";
    modelApt.save("model_apt.bin");

    auto reportMetrics = [&](const std::string &name,
                             const std::vector<EncodeData> &data,
                             const GBDT &model) {
      double sumSqErr = 0.0, totalAbsErrPct = 0.0;
      for (const auto &d : data) {
        double predLogPM2 = model.predict(d.feats);
        double actualPM2 = std::exp(d.target);
        double predPM2 = std::exp(predLogPM2);
        double err = predPM2 - actualPM2;
        sumSqErr += err * err;
        totalAbsErrPct += std::abs(err) / actualPM2;
      }
      double rmsePM2 = std::sqrt(sumSqErr / static_cast<double>(data.size()));
      double maePct =
          (totalAbsErrPct / static_cast<double>(data.size())) * 100.0;
      double rmseLog = model.rmse(data);

      std::cout << "  " << name << ":\n";
      std::cout << "    RMSE log: " << rmseLog << "\n";
      std::cout << "    RMSE EUR/m2: " << rmsePM2 << "\n";
      std::cout << "    MAE %: " << maePct << "%\n";
    };

    std::cout << "\n--- Final Validation Report: HOUSES ---\n";
    reportMetrics("House Train", finalTrainHouse, modelHouse);
    reportMetrics("House Val (2024)", valSetHouse, modelHouse);
    reportMetrics("House Test (2025)", testSetHouse, modelHouse);

    std::cout << "\n--- Final Validation Report: APARTMENTS ---\n";
    reportMetrics("Apt Train", finalTrainApt, modelApt);
    reportMetrics("Apt Val (2024)", valSetApt, modelApt);
    reportMetrics("Apt Test (2025)", testSetApt, modelApt);

  } catch (const std::exception &e) {
    std::cerr << "CRITICAL ERROR: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
