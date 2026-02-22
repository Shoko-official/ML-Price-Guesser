#include "../include/normalizer.hpp"
#include <cmath>
#include <stdexcept>

// Compute per-feature mean and standard deviation across all samples.
void Normalizer::fit(const std::vector<EncodeData> &data) {
  if (data.empty())
    throw std::runtime_error("Cannot fit normalizer on empty dataset");

  size_t numFeats = data[0].feats.size();
  means.assign(numFeats, 0.0);
  stds.assign(numFeats, 0.0);

  // First pass: accumulate sums to get the mean
  for (const auto &sample : data) {
    for (size_t i = 0; i < numFeats; i++) {
      means[i] += sample.feats[i];
    }
  }
  for (size_t i = 0; i < numFeats; i++) {
    means[i] /= static_cast<double>(data.size());
  }

  // Second pass: accumulate squared deviations for variance
  for (const auto &sample : data) {
    for (size_t i = 0; i < numFeats; i++) {
      double diff = sample.feats[i] - means[i];
      stds[i] += diff * diff;
    }
  }
  for (size_t i = 0; i < numFeats; i++) {
    stds[i] = std::sqrt(stds[i] / static_cast<double>(data.size()));

    // Guard against constant features (std = 0).
    // If a feature never changes, we just leave it as-is.
    if (stds[i] < 1e-9)
      stds[i] = 1.0;
  }
}

// Apply z-score to every feature of every sample: (x - mean) / std
void Normalizer::transform(std::vector<EncodeData> &data) const {
  if (means.empty())
    throw std::runtime_error("Normalizer has not been fitted yet");

  for (auto &sample : data) {
    for (size_t i = 0; i < sample.feats.size(); i++) {
      sample.feats[i] = (sample.feats[i] - means[i]) / stds[i];
    }
  }
}

void Normalizer::fitTransform(std::vector<EncodeData> &data) {
  fit(data);
  transform(data);
}
