#pragma once
#include <memory>
#include <vector>

struct TreeNode {
  bool isLeaf = false;
  double leafValue = 0.0;

  int splitFeature = -1;
  double splitThreshold = 0.0;

  std::unique_ptr<TreeNode> left;
  std::unique_ptr<TreeNode> right;

  void serialize(std::ostream &os) const;
  static std::unique_ptr<TreeNode> deserialize(std::istream &is);
};

struct HistogramBin {
  double sumG = 0.0;
  double sumW = 0.0;
};

class DecisionTree {
public:
  DecisionTree(int maxDepth = 4, int minSamplesSplit = 10, double lambda = 1.0);

  // Optimized fit method for flattened Histogram-based training
  void fit(const uint8_t *binnedFeats, int numFeatures,
           const std::vector<double> &residuals,
           const std::vector<double> &weights, const std::vector<int> &indices,
           const std::vector<std::vector<double>> &binThresholds);

  double predict(const std::vector<double> &feats) const;

  void save(std::ostream &os) const;
  void load(std::istream &is);

private:
  int maxDepth;
  int minSamplesSplit;
  double lambda;
  std::unique_ptr<TreeNode> root;
  std::vector<std::vector<double>> thresholds; // thresholds for each bin

  std::unique_ptr<TreeNode> buildNode(const uint8_t *binnedFeats,
                                      int numFeatures,
                                      const std::vector<double> &residuals,
                                      const std::vector<double> &weights,
                                      const std::vector<int> &indices,
                                      int depth);

  bool bestSplit(const uint8_t *binnedFeats, int numFeatures,
                 const std::vector<double> &residuals,
                 const std::vector<double> &weights,
                 const std::vector<int> &indices, int &outFeature,
                 double &outThreshold, double &outGain);

  double traverse(const TreeNode &node, const std::vector<double> &feats) const;
};
