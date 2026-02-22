#pragma once
#include "encoder.hpp"
#include <vector>

// Weight initialization strategy.
// Xavier is generally safer when the number of features is large.
enum class WeightInit { Zero, Xavier };

// Linear regression trained with batch gradient descent.
// I added L2 (ridge) regularization because without it the model tends to
// blow up on highly correlated geographic features... (dept and city codes)

class LinearRegression {
public:
  LinearRegression(double lr = 0.01, int epochs = 500, double lambda = 0.001,
                   WeightInit init = WeightInit::Xavier,
                   unsigned int seed = 42);

  // Train the model on a set of already-encoded and normalized samples.
  void train(const std::vector<EncodeData> &data);

  // Predict a single price given a feature vector.
  double predict(const std::vector<double> &feats) const;

  // Compute RMSE on a held-out test set : useful to track progress.
  double rmse(const std::vector<EncodeData> &testData) const;

  // Access trained weights if needed (e.g. for inspection / saving).
  const std::vector<double> &getWeights() const { return weights; }
  double getBias() const { return bias; }

private:
  double learningRate;
  int epochs;
  double lambda; // L2 regularization strength
  WeightInit initStrategy;
  unsigned int seed;

  std::vector<double> weights;
  double bias;
};
