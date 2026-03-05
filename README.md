# ML Price Guesser

High-performance Gradient Boosted Decision Tree (GBDT) pipeline in C++ for the French *Demandes de Valeurs Foncières* (DVF) dataset.

## Core Features
- **Temporal Splitting**: 2020–2023 for training, 2024 for validation, 2025 for test. Prevents look-ahead bias.
- **Out-of-Fold Encoding**: Target encoding for high-cardinality categorical features (Section, Street, City).
- **Hybrid Features**: Integrates DVF with INSEE (income), BDNB (building data), and OSM (POI proximity).
- **Scalability**: Histogram-based binning and OpenMP parallelization.

## Feature Set (40+)
- **DVF**: Area, Rooms, Local/Land Type, Lot Count.
- **Contextual**: Median Income (INSEE), Construction Year/DPE (BDNB), Flood/Noise risks.
- **Spatial**: Distances to schools, hospitals, transport (OpenStreetMap).
- **Temporal**: 18-month rolling median prices at section level.

## Model
- **GBDT implementation**: Custom gain calculation with L2 regularization.
- **Histogram-based**: 256 bins for fast split searching.
- **Log-target**: Trained on $\log(\text{Price})$ to handle market skew.

## Usage

### Prerequisites
- CMake 3.10+
- OpenMP supported compiler (GCC/Clang)

### Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./ml_price_guesser
```

Alternatively, use `run.sh` for a full reproducibility sweep.

## Data Strategy
To avoid leakage, market indicators (like `sectionPriceM2`) are calculated using a history map that only includes data from the training window (pre-2024). Validating on 2024 and testing on 2025 ensures the model's reliability in a production-like environment.

Current dataset source: [Etalab DVF 2025](https://static.data.gouv.fr/resources/demandes-de-valeurs-foncieres/20251018-234902/valeursfoncieres-2025-s1.txt.zip).
