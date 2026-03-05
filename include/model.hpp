#pragma once
#include "encoder.hpp"
#include "tree.hpp"
#include <vector>

// Gradient boosted decision trees, trained from scratch.
// Each round fits a shallow tree on the residuals of the current ensemble,
// then adds it with a shrinkage factor to slow down learning and reduce
// overfitting.
class GBDT {
public:
  GBDT(int numTrees = 100, double learningRate = 0.1, int maxDepth = 4,
       int minSamplesSplit = 10, double subsampleRatio = 0.5,
       double lambda = 1.0);

  void fit(const std::vector<EncodeData> &trainData,
           const std::vector<EncodeData> &valData = {}, int patience = 30);

  double predict(const std::vector<double> &feats) const;

  double rmse(const std::vector<EncodeData> &testData) const;

  void save(const std::string &path) const;
  void load(const std::string &path);

private:
  std::vector<std::vector<double>>
  computeBinThresholds(const std::vector<EncodeData> &data, int maxBins = 256);
  std::vector<uint8_t>
  binFeatures(const std::vector<EncodeData> &data,
              const std::vector<std::vector<double>> &thresholds);

  int numTrees;
  double learningRate;
  int maxDepth;
  int minSamplesSplit;
  double subsampleRatio;
  double lambda;
  double basePrediction;
  std::vector<std::vector<double>> binThresholds;
  std::vector<DecisionTree> trees;
};
