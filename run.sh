#!/bin/bash
# Reproductibility script for ML-Price-Guesser

set -e

echo "--- Building MLPriceGuesser ---"
mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo ""
echo "--- Running MLPriceGuesser ---"
echo "Note: This requires data files to be present in ../data/"
./ml_price_guesser
