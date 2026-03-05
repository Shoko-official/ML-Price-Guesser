#include "../include/tree.hpp"
#include <iostream>
#include <omp.h>

DecisionTree::DecisionTree(int maxDepth, int minSamplesSplit, double lambda)
    : maxDepth(maxDepth), minSamplesSplit(minSamplesSplit), lambda(lambda) {}

void DecisionTree::fit(const uint8_t *binnedFeats, int numFeatures,
                       const std::vector<double> &residuals,
                       const std::vector<double> &weights,
                       const std::vector<int> &indices,
                       const std::vector<std::vector<double>> &binThresholds) {
  if (indices.empty())
    return;
  this->thresholds = binThresholds;
  root = buildNode(binnedFeats, numFeatures, residuals, weights, indices, 0);
}

double DecisionTree::predict(const std::vector<double> &feats) const {
  return traverse(*root, feats);
}

void TreeNode::serialize(std::ostream &os) const {
  os.write(reinterpret_cast<const char *>(&isLeaf), sizeof(isLeaf));
  if (isLeaf) {
    os.write(reinterpret_cast<const char *>(&leafValue), sizeof(leafValue));
  } else {
    os.write(reinterpret_cast<const char *>(&splitFeature),
             sizeof(splitFeature));
    os.write(reinterpret_cast<const char *>(&splitThreshold),
             sizeof(splitThreshold));
    left->serialize(os);
    right->serialize(os);
  }
}

std::unique_ptr<TreeNode> TreeNode::deserialize(std::istream &is) {
  auto node = std::make_unique<TreeNode>();
  is.read(reinterpret_cast<char *>(&node->isLeaf), sizeof(node->isLeaf));
  if (node->isLeaf) {
    is.read(reinterpret_cast<char *>(&node->leafValue),
            sizeof(node->leafValue));
  } else {
    is.read(reinterpret_cast<char *>(&node->splitFeature),
            sizeof(node->splitFeature));
    is.read(reinterpret_cast<char *>(&node->splitThreshold),
            sizeof(node->splitThreshold));
    node->left = deserialize(is);
    node->right = deserialize(is);
  }
  return node;
}

void DecisionTree::save(std::ostream &os) const {
  os.write(reinterpret_cast<const char *>(&maxDepth), sizeof(maxDepth));
  os.write(reinterpret_cast<const char *>(&minSamplesSplit),
           sizeof(minSamplesSplit));
  os.write(reinterpret_cast<const char *>(&lambda), sizeof(lambda));

  // Save thresholds
  size_t numFeatures = thresholds.size();
  os.write(reinterpret_cast<const char *>(&numFeatures), sizeof(numFeatures));
  for (const auto &featThr : thresholds) {
    size_t numBins = featThr.size();
    os.write(reinterpret_cast<const char *>(&numBins), sizeof(numBins));
    os.write(reinterpret_cast<const char *>(featThr.data()),
             sizeof(double) * numBins);
  }

  bool hasRoot = (root != nullptr);
  os.write(reinterpret_cast<const char *>(&hasRoot), sizeof(hasRoot));
  if (hasRoot) {
    root->serialize(os);
  }
}

void DecisionTree::load(std::istream &is) {
  is.read(reinterpret_cast<char *>(&maxDepth), sizeof(maxDepth));
  is.read(reinterpret_cast<char *>(&minSamplesSplit), sizeof(minSamplesSplit));
  is.read(reinterpret_cast<char *>(&lambda), sizeof(lambda));

  size_t numFeatures;
  is.read(reinterpret_cast<char *>(&numFeatures), sizeof(numFeatures));
  thresholds.resize(numFeatures);
  for (size_t f = 0; f < numFeatures; ++f) {
    size_t numBins;
    is.read(reinterpret_cast<char *>(&numBins), sizeof(numBins));
    thresholds[f].resize(numBins);
    is.read(reinterpret_cast<char *>(thresholds[f].data()),
            sizeof(double) * numBins);
  }

  bool hasRoot;
  is.read(reinterpret_cast<char *>(&hasRoot), sizeof(hasRoot));
  if (hasRoot) {
    root = TreeNode::deserialize(is);
  } else {
    root.reset();
  }
}

std::unique_ptr<TreeNode>
DecisionTree::buildNode(const uint8_t *binnedFeats, int numFeatures,
                        const std::vector<double> &residuals,
                        const std::vector<double> &weights,
                        const std::vector<int> &indices, int depth) {
  auto node = std::make_unique<TreeNode>();

  double sumG = 0.0;
  double sumW = 0.0;
  for (int i : indices) {
    sumG += residuals[i] * weights[i];
    sumW += weights[i];
  }
  double leafVal = sumG / (sumW + lambda);

  if (depth >= maxDepth || static_cast<int>(indices.size()) < minSamplesSplit) {
    node->isLeaf = true;
    node->leafValue = leafVal;
    return node;
  }

  int bestF = -1;
  double bestThr = 0.0, bestGain = 0.0;
  bool ok = bestSplit(binnedFeats, numFeatures, residuals, weights, indices,
                      bestF, bestThr, bestGain);

  if (!ok || bestGain <= 0.0) {
    node->isLeaf = true;
    node->leafValue = leafVal;
    return node;
  }

  std::vector<int> left, right;
  uint8_t bestBinIdx = 0;
  for (size_t i = 0; i < thresholds[bestF].size(); ++i) {
    if (thresholds[bestF][i] == bestThr) {
      bestBinIdx = (uint8_t)i;
      break;
    }
  }

  for (int i : indices) {
    if (binnedFeats[i * numFeatures + bestF] <= bestBinIdx)
      left.push_back(i);
    else
      right.push_back(i);
  }

  node->splitFeature = bestF;
  node->splitThreshold = bestThr;
  node->left =
      buildNode(binnedFeats, numFeatures, residuals, weights, left, depth + 1);
  node->right =
      buildNode(binnedFeats, numFeatures, residuals, weights, right, depth + 1);
  return node;
}

bool DecisionTree::bestSplit(const uint8_t *binnedFeats, int numFeatures,
                             const std::vector<double> &residuals,
                             const std::vector<double> &weights,
                             const std::vector<int> &indices, int &outFeature,
                             double &outThreshold, double &outGain) {
  int n = static_cast<int>(indices.size());
  double totalSumG = 0.0;
  double totalSumW = 0.0;
  for (int i : indices) {
    totalSumG += residuals[i] * weights[i];
    totalSumW += weights[i];
  }

  double parentGain = (totalSumG * totalSumG) / (totalSumW + lambda);

  double globalBestGain = 0.0;
  int globalBestF = -1;
  double globalBestThr = 0.0;
  bool globalFound = false;

#pragma omp parallel
  {
    double localBestGain = 0.0;
    int localBestF = -1;
    double localBestThr = 0.0;
    bool localFound = false;

#pragma omp for
    for (int f = 0; f < numFeatures; f++) {
      int numBins = (int)thresholds[f].size();
      if (numBins == 0)
        continue;

      std::vector<HistogramBin> hist(numBins);
      for (int i : indices) {
        uint8_t bin = binnedFeats[i * numFeatures + f];
        hist[bin].sumG += residuals[i] * weights[i];
        hist[bin].sumW += weights[i];
      }

      double leftSumG = 0.0;
      double leftSumW = 0.0;
      int leftCount = 0;
      for (int b = 0; b < numBins - 1; b++) {
        leftSumG += hist[b].sumG;
        leftSumW += hist[b].sumW;
        leftCount += 1; // Not really used for Gain but for consistency with
                        // minSamplesSplit

        if (leftCount < 1) // Simple safety
          continue;

        double rightSumG = totalSumG - leftSumG;
        double rightSumW = totalSumW - leftSumW;
        double currentGain = (leftSumG * leftSumG / (leftSumW + lambda)) +
                             (rightSumG * rightSumG / (rightSumW + lambda)) -
                             parentGain;

        if (!localFound || currentGain > localBestGain) {
          localBestGain = currentGain;
          localBestF = f;
          localBestThr = thresholds[f][b];
          localFound = true;
        }
      }
    }

#pragma omp critical
    {
      if (localFound && (!globalFound || localBestGain > globalBestGain)) {
        globalBestGain = localBestGain;
        globalBestF = localBestF;
        globalBestThr = localBestThr;
        globalFound = true;
      }
    }
  }

  if (globalFound) {
    outGain = globalBestGain;
    outFeature = globalBestF;
    outThreshold = globalBestThr;
  }
  return globalFound;
}

double DecisionTree::traverse(const TreeNode &node,
                              const std::vector<double> &feats) const {
  if (node.isLeaf)
    return node.leafValue;
  if (feats[node.splitFeature] <= node.splitThreshold)
    return traverse(*node.left, feats);
  return traverse(*node.right, feats);
}