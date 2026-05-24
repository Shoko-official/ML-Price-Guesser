# Tests for ML-Price-Guesser

This test pack adds a lightweight C++ test executable for the `GBDT` branch.

## Files to add or replace

```text
CMakeLists.txt
tests/test_core.cpp
.github/workflows/ci.yml
```

## What is covered

- `LabelEncode`
  - stable IDs
  - empty labels
  - lookup of unknown labels
  - reset behavior

- `TargetEncoder`
  - per-category means
  - fallback to global mean for unknown categories

- `Encoder`
  - output size
  - target preservation
  - temporal weight contract
  - feature vector size contract
  - finite feature values

- `Parser`
  - small CSV map parsing for market indicators
  - BDNB parsing and section padding
  - macro indicator parsing

- `DecisionTree`
  - simple split behavior
  - prediction behavior
  - serialization/deserialization roundtrip

- `GBDT`
  - empty training set error
  - constant-target fit
  - save/load prediction roundtrip

## Run locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Notes

These tests are designed to be fast and not require real DVF data files.
They protect the core logic and give the repository a much stronger engineering signal.
