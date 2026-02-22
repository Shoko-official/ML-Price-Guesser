#include "../include/encoder.hpp"
#include "../include/model.hpp"
#include "../include/normalizer.hpp"
#include "../include/parser.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// Split a dataset into a training set and a test set.
// I'm shuffling before splitting to avoid any ordering bias in the raw file
// (e.g. all Paris entries grouped together at the end).
void trainTestSplit(std::vector<EncodeData> &allData,
                    std::vector<EncodeData> &trainSet,
                    std::vector<EncodeData> &testSet, double testRatio = 0.2,
                    unsigned int seed = 42) {
  std::mt19937 rng(seed);
  std::shuffle(allData.begin(), allData.end(), rng);

  size_t testSize = static_cast<size_t>(allData.size() * testRatio);
  size_t trainSize = allData.size() - testSize;

  trainSet.assign(allData.begin(), allData.begin() + trainSize);
  testSet.assign(allData.begin() + trainSize, allData.end());
}

int main() {
  const std::string dataPath = "data/ValeursFoncieres-2025-S1.txt";

  std::cout << "Parsing data...\n";
  Parser parser;
  std::vector<Data> rawRecords = parser.parse(dataPath);
  std::cout << "  Loaded " << rawRecords.size() << " valid records.\n";

  if (rawRecords.empty()) {
    std::cerr << "Error: no records were loaded. Check the file path.\n";
    return 1;
  }

  std::cout << "Encoding features...\n";
  Encoder encoder;
  std::vector<EncodeData> encodedData = encoder.transform(rawRecords);

  // Important: fit the normalizer only on the training set, not evaluation set
  std::cout << "Splitting into train / test sets (80 / 20)...\n";
  std::vector<EncodeData> trainSet, testSet;
  trainTestSplit(encodedData, trainSet, testSet);
  std::cout << "  Train: " << trainSet.size() << " samples\n";
  std::cout << "  Test:  " << testSet.size() << " samples\n";

  // Fit on train only, then apply the same scale to the test set.
  std::cout << "Normalizing features...\n";
  Normalizer normalizer;
  normalizer.fit(trainSet);
  normalizer.transform(trainSet);
  normalizer.transform(testSet);

  std::cout << "Training linear regression...\n";
  LinearRegression model(0.01,              // learning rate
                         500,               // number of epochs
                         0.001,             // L2 regularization strength
                         WeightInit::Xavier // weight initialization strategy
  );
  model.train(trainSet);

  double trainRmse = model.rmse(trainSet);
  double testRmse = model.rmse(testSet);
  std::cout << "\nResults:\n";
  std::cout << "  Train RMSE: " << trainRmse << " EUR\n";
  std::cout << "  Test  RMSE: " << testRmse << " EUR\n";

  return 0;
}
