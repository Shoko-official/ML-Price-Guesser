#include "../include/model.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

GBDT::GBDT(int numTrees, double learningRate, int maxDepth, int minSamplesSplit,
           double subsampleRatio, double lambda)
    : numTrees(numTrees), learningRate(learningRate), maxDepth(maxDepth),
      minSamplesSplit(minSamplesSplit), subsampleRatio(subsampleRatio),
      lambda(lambda), basePrediction(0.0) {}

void GBDT::fit(const std::vector<EncodeData> &trainData,
               const std::vector<EncodeData> &valData, int patience) {
  if (trainData.empty())
    throw std::runtime_error("Training set is empty");

  size_t n = trainData.size();
  int numFeatures = (int)trainData[0].feats.size();

  double sum = 0.0;
  for (const auto &sample : trainData)
    sum += sample.target;
  basePrediction = sum / static_cast<double>(n);

  std::vector<double> predictions(n, basePrediction);

  std::cout << "Creating histogram bins for " << trainData.size()
            << " records...\n";
  binThresholds = computeBinThresholds(trainData);
  std::cout << "Applying binning...\n";
  std::vector<uint8_t> binnedFeats = binFeatures(trainData, binThresholds);

  std::mt19937 rng(42);
  size_t subsetSize =
      std::max<size_t>(1, static_cast<size_t>(n * subsampleRatio));
  std::vector<int> allIndices(n);
  std::iota(allIndices.begin(), allIndices.end(), 0);

  double bestValRmse = std::numeric_limits<double>::max();
  int roundsSinceImprovement = 0;
  trees.reserve(numTrees);
  std::vector<double> residuals(n);
  std::vector<double> weights(n);
  for (size_t i = 0; i < n; i++) {
    weights[i] = trainData[i].weight;
  }

  for (int t = 0; t < numTrees; t++) {
    std::shuffle(allIndices.begin(), allIndices.end(), rng);

    std::vector<int> subset(subsetSize);
    for (size_t k = 0; k < subsetSize; k++) {
      subset[k] = allIndices[k];
    }
    for (size_t i = 0; i < n; i++) {
      residuals[i] = trainData[i].target - predictions[i];
    }

    DecisionTree tree(maxDepth, minSamplesSplit, lambda);
    tree.fit(binnedFeats.data(), numFeatures, residuals, weights, subset,
             binThresholds);

    for (size_t i = 0; i < n; i++) {
      predictions[i] += learningRate * tree.predict(trainData[i].feats);
    }

    trees.push_back(std::move(tree));

    if ((t + 1) % 10 == 0 || t == numTrees - 1) {
      if (!valData.empty()) {
        double currentValRmse = rmse(valData);
        std::cout << "Tree " << t + 1 << "  Val RMSE: " << currentValRmse
                  << "\n";

        if (currentValRmse < bestValRmse) {
          bestValRmse = currentValRmse;
          roundsSinceImprovement = 0;
        } else {
          roundsSinceImprovement += 10;
          if (roundsSinceImprovement >= patience) {
            int toRemove = std::min(roundsSinceImprovement,
                                    static_cast<int>(trees.size()));
            std::cout << "Early stopping at tree " << t + 1
                      << " (Best Val: " << bestValRmse << ")\n";
            trees.resize(trees.size() - toRemove);
            break;
          }
        }
      }
    }
  }
}

std::vector<std::vector<double>>
GBDT::computeBinThresholds(const std::vector<EncodeData> &data, int maxBins) {
  int nFts = (int)data[0].feats.size();
  std::vector<std::vector<double>> thresholds(nFts);

  // Subsample for binning
  const size_t maxSamples = 100000;
  std::vector<size_t> indices(data.size());
  std::iota(indices.begin(), indices.end(), 0);
  if (data.size() > maxSamples) {
    std::mt19937 rng(42);
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(maxSamples);
  }

  for (int f = 0; f < nFts; f++) {
    std::vector<double> values;
    values.reserve(indices.size());
    for (size_t idx : indices)
      values.push_back(data[idx].feats[f]);
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());

    if ((int)values.size() <= maxBins) {
      thresholds[f] = values;
    } else {
      for (int i = 1; i <= maxBins; i++) {
        thresholds[f].push_back(values[i * (values.size() - 1) / maxBins]);
      }
    }
  }
  return thresholds;
}

std::vector<uint8_t>
GBDT::binFeatures(const std::vector<EncodeData> &data,
                  const std::vector<std::vector<double>> &thresholds) {
  int numFeatures = (int)thresholds.size();
  std::vector<uint8_t> binned(data.size() * numFeatures);
  for (size_t i = 0; i < data.size(); i++) {
    for (int f = 0; f < numFeatures; f++) {
      auto it = std::lower_bound(thresholds[f].begin(), thresholds[f].end(),
                                 data[i].feats[f]);
      int binIdx = (int)std::distance(thresholds[f].begin(), it);
      if (binIdx >= (int)thresholds[f].size())
        binIdx = (int)thresholds[f].size() - 1;
      binned[i * numFeatures + f] = (uint8_t)binIdx;
    }
  }
  return binned;
}

double GBDT::predict(const std::vector<double> &feats) const {
  double result = basePrediction;
  for (const auto &tree : trees)
    result += learningRate * tree.predict(feats);
  return result;
}

double GBDT::rmse(const std::vector<EncodeData> &testData) const {
  if (testData.empty())
    return 0.0;
  double sumSquaredErrors = 0.0;
  for (const auto &sample : testData) {
    double error = predict(sample.feats) - sample.target;
    sumSquaredErrors += error * error;
  }
  return std::sqrt(sumSquaredErrors / static_cast<double>(testData.size()));
}

void GBDT::save(const std::string &path) const {
  std::ofstream os(path, std::ios::binary);
  if (!os.is_open())
    throw std::runtime_error("Could not open file for saving: " + path);

  os.write(reinterpret_cast<const char *>(&numTrees), sizeof(numTrees));
  os.write(reinterpret_cast<const char *>(&learningRate), sizeof(learningRate));
  os.write(reinterpret_cast<const char *>(&maxDepth), sizeof(maxDepth));
  os.write(reinterpret_cast<const char *>(&minSamplesSplit),
           sizeof(minSamplesSplit));
  os.write(reinterpret_cast<const char *>(&subsampleRatio),
           sizeof(subsampleRatio));
  os.write(reinterpret_cast<const char *>(&lambda), sizeof(lambda));
  os.write(reinterpret_cast<const char *>(&basePrediction),
           sizeof(basePrediction));

  size_t numFeatures = binThresholds.size();
  os.write(reinterpret_cast<const char *>(&numFeatures), sizeof(numFeatures));
  for (const auto &featThr : binThresholds) {
    size_t numBins = featThr.size();
    os.write(reinterpret_cast<const char *>(&numBins), sizeof(numBins));
    os.write(reinterpret_cast<const char *>(featThr.data()),
             sizeof(double) * numBins);
  }

  size_t activeTrees = trees.size();
  os.write(reinterpret_cast<const char *>(&activeTrees), sizeof(activeTrees));
  for (const auto &tree : trees) {
    tree.save(os);
  }
}

void GBDT::load(const std::string &path) {
  std::ifstream is(path, std::ios::binary);
  if (!is.is_open())
    throw std::runtime_error("Could not open file for loading: " + path);

  is.read(reinterpret_cast<char *>(&numTrees), sizeof(numTrees));
  is.read(reinterpret_cast<char *>(&learningRate), sizeof(learningRate));
  is.read(reinterpret_cast<char *>(&maxDepth), sizeof(maxDepth));
  is.read(reinterpret_cast<char *>(&minSamplesSplit), sizeof(minSamplesSplit));
  is.read(reinterpret_cast<char *>(&subsampleRatio), sizeof(subsampleRatio));
  is.read(reinterpret_cast<char *>(&lambda), sizeof(lambda));
  is.read(reinterpret_cast<char *>(&basePrediction), sizeof(basePrediction));

  size_t numFeatures;
  is.read(reinterpret_cast<char *>(&numFeatures), sizeof(numFeatures));
  binThresholds.resize(numFeatures);
  for (size_t f = 0; f < numFeatures; ++f) {
    size_t numBins;
    is.read(reinterpret_cast<char *>(&numBins), sizeof(numBins));
    binThresholds[f].resize(numBins);
    is.read(reinterpret_cast<char *>(binThresholds[f].data()),
            sizeof(double) * numBins);
  }

  size_t activeTrees;
  is.read(reinterpret_cast<char *>(&activeTrees), sizeof(activeTrees));
  trees.clear();
  trees.reserve(activeTrees);
  for (size_t i = 0; i < activeTrees; ++i) {
    DecisionTree tree;
    tree.load(is);
    trees.push_back(std::move(tree));
  }
}