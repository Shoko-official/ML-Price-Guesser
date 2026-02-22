#include "../include/model.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

LinearRegression::LinearRegression(double lr, int epochs, double lambda,
                                   WeightInit init, unsigned int seed)
    : learningRate(lr), epochs(epochs), lambda(lambda), initStrategy(init),
      seed(seed), bias(0.0) {}

// The core training loop: batch gradient descent.
// For each epoch we compute the prediction error on every sample and
// nudge the weights in the direction that reduces average squared error.
void LinearRegression::train(const std::vector<EncodeData> &data) {
  if (data.empty())
    throw std::runtime_error("Training set is empty");

  size_t numFeatures = data[0].feats.size();
  size_t numSamples = data.size();

  // Weight initialization : Xavier scales the weights by 1/sqrt(numFeatures)
  // which helps when features have different magnitudes.
  if (initStrategy == WeightInit::Xavier) {
    std::mt19937 rng(seed);
    double range = std::sqrt(1.0 / static_cast<double>(numFeatures));
    std::uniform_real_distribution<double> dist(-range, range);
    weights.resize(numFeatures);
    for (auto &w : weights)
      w = dist(rng);
  } else {
    weights.assign(numFeatures, 0.0);
  }
  bias = 0.0;

  // Separate the feature matrix and targets into flat arrays
  std::vector<double> featureMatrix(numSamples * numFeatures);
  std::vector<double> targetValues(numSamples);
  for (size_t i = 0; i < numSamples; i++) {
    targetValues[i] = data[i].target;
    for (size_t j = 0; j < numFeatures; j++)
      featureMatrix[i * numFeatures + j] = data[i].feats[j];
  }

  std::vector<double> weightGradients(numFeatures);

  for (int epoch = 0; epoch < epochs; epoch++) {
    // Reset accumulators
    weightGradients.assign(numFeatures, 0.0);
    double biasGradient = 0.0;
    double totalLoss = 0.0;

    // Accumulate gradients
    for (size_t i = 0; i < numSamples; i++) {
      const double *sampleFeatures = &featureMatrix[i * numFeatures];

      double predicted = bias;
      for (size_t j = 0; j < numFeatures; j++)
        predicted += weights[j] * sampleFeatures[j];

      double error = predicted - targetValues[i];
      totalLoss += error * error;
      biasGradient += error;

      for (size_t j = 0; j < numFeatures; j++)
        weightGradients[j] += error * sampleFeatures[j];
    }

    // Average gradients over the dataset, apply L2 penalty, then update
    double invNumSamples = 1.0 / static_cast<double>(numSamples);
    bias -= learningRate * biasGradient * invNumSamples;

    for (size_t j = 0; j < numFeatures; j++) {
      // Ridge penalty: pulls weights toward zero to avoid overfitting
      weightGradients[j] =
          weightGradients[j] * invNumSamples + lambda * weights[j];
      weights[j] -= learningRate * weightGradients[j];
    }

    if (epoch % 50 == 0) {
      double currentRmse = std::sqrt(totalLoss * invNumSamples);
      std::cout << "Epoch " << epoch << "  RMSE: " << currentRmse << "\n";
    }
  }
}

double LinearRegression::predict(const std::vector<double> &feats) const {
  double predicted = bias;
  for (size_t i = 0; i < feats.size(); i++)
    predicted += weights[i] * feats[i];
  return predicted;
}

double LinearRegression::rmse(const std::vector<EncodeData> &testData) const {
  if (testData.empty())
    return 0.0;

  double sumSquaredErrors = 0.0;
  for (const auto &sample : testData) {
    double error = predict(sample.feats) - sample.target;
    sumSquaredErrors += error * error;
  }
  return std::sqrt(sumSquaredErrors / static_cast<double>(testData.size()));
}