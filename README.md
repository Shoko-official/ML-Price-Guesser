# ML Price Guesser

C++17 machine-learning project for real-estate price prediction using a gradient-boosted decision tree model built from scratch.

## Goal

Predict real-estate price per square meter from public and enriched datasets using a temporal train/validation/test split.

## Current approach

- C++17 implementation.
- CMake build.
- OpenMP acceleration.
- Custom parser and feature engineering pipeline.
- Separate handling for houses and apartments.
- Log transformation of the target.
- Temporal split:
  - training: historical data up to 2023
  - validation: 2024
  - test: 2025

## Main engineering work

- Parser for DVF-like transaction data.
- Feature enrichment with market, socio-economic, POI, risk, and local indicators.
- Target encoding with folds to reduce leakage risk.
- Custom GBDT implementation with histogram-style binning.
- Binary model serialization.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/ml_price_guesser
```

The current code expects local data files under `data/`. This will be replaced by a configuration file or CLI arguments.

## Known limitations

- Some data paths are still hardcoded.
- C++ unit tests are not complete yet.
- Model serialization does not yet include format versioning.
- Metrics should be exported to a structured report file.
- Dataset download/preparation is not automated.

## Next steps

- Add CLI flags for data paths and output paths.
- Add unit tests for parser, encoder, tree splits, and model serialization.
- Add CI build.
- Add a small synthetic dataset for tests.
- Add benchmark results and reproducible metrics.
