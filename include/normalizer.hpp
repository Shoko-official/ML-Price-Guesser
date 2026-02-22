#pragma once
#include "encoder.hpp"
#include <vector>

// Simple feature-wise standardization (zero mean, unit variance).
// I'm going with z-score normalization rather than min-max because it
// handles outliers better - property prices have a lot of extreme values.

class Normalizer {
public:
  // Learn mean and std from a set of encoded samples.
  // Must be called before transform().
  void fit(const std::vector<EncodeData> &data);

  // Normalize in-place. Assumes fit() was already called.
  void transform(std::vector<EncodeData> &data) const;

  // Convenience: fit then transform in one go.
  void fitTransform(std::vector<EncodeData> &data);

  // Getters - useful later if we want to normalize new samples
  // with the same parameters (e.g. for inference).
  const std::vector<double> &getMeans() const { return means; }
  const std::vector<double> &getStds() const { return stds; }

private:
  std::vector<double> means;
  std::vector<double> stds;
};
