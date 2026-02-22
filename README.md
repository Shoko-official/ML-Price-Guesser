# ML Price Guesser

A from-scratch linear regression pipeline written in C++, trained on the
French *Demandes de Valeurs Foncières* (DVF) open dataset to predict
residential property transaction prices.

---

## Motivation

I wanted to see how far a plain linear model could go on that
volume of data without relying on any ML library : just math, C++, and the
standard library.

---

## Dataset

| Field | Description |
|---|---|
| Source | DVF "ValeursFoncieres-2025-S1.txt" |
| Raw rows | ~1 000 000+ |
| Kept after filtering | 850 020 |
| Target variable | Declared transaction price (EUR) |

Filtering rules applied:

- Keep only "Vente" (sale) transactions, not donations or expropriations.
- Drop prices below €10 000: those are almost always symbolic family
  transfers, not market prices.
- Drop prices above €2 000 000: luxury outliers are too few and too
  heterogeneous to be useful at this stage.
- Drop built properties (localTypeCode > 0) that have zero Carrez area and
  zero real built area, they carry no surface signal.

---

## Features (14 total)

| # | Feature | Notes |
|---|---|---|
| 1 | year | Transaction year |
| 2 | month | Transaction month (1–12) |
| 3 | localTypeCode | Category code for the built structure |
| 4 | totalCarrezArea | Sum of all Carrez law surface areas (m²) |
| 5 | realBuiltArea | Actual built area per lot (m²) |
| 6 | mainRooms | Number of main rooms |
| 7 | lotCount | Number of lots in the transaction |
| 8 | landArea | Land (plot) area (m²) |
| 9 | deptCode | French department code |
| 10 | cityCode | City code within department |
| 11 | section | Cadastral section |
| 12 | streetCode | Street identifier |
| 13 | landType | Nature of the land |
| 14 | landTypeSpecial | Special land nature |

Categorical features are label-encoded to integers (ordinal assignment in
encounter order). it is not perfect, however it permits to keep the feature vector dense and avoids exploding
the dimensionality across thousands of unique city codes.

---

## Model

Batch gradient descent linear regression with L2 (Ridge) regularization.

The model learns a weight vector $\mathbf{w} \in \mathbb{R}^{14}$ and a bias $b$ such that:

$\hat{y} = \mathbf{w}^\top \mathbf{x} + b$

The objective is to minimize the regularized MSE over the training set of
$n = 680\,016$ samples:

$\mathcal{L}(\mathbf{w}, b) = \frac{1}{n} \sum_{i=1}^{n} (\hat{y}_i - y_i)^2 + \lambda \|\mathbf{w}\|^2$

Update rules at each epoch:

$\frac{\partial \mathcal{L}}{\partial w_j} = \frac{1}{n} \sum_{i=1}^{n} (\hat{y}_i - y_i)\, x_{ij} + \lambda w_j$

$\frac{\partial \mathcal{L}}{\partial b} = \frac{1}{n} \sum_{i=1}^{n} (\hat{y}_i - y_i)$

$w_j \leftarrow w_j - \alpha \cdot \frac{\partial \mathcal{L}}{\partial w_j}, \qquad b \leftarrow b - \alpha \cdot \frac{\partial \mathcal{L}}{\partial b}$

Hyperparameters:

| Parameter | Value | Rationale |
|---|---|---|
| Learning rate α | 0.01 | Conservative enough to converge without oscillating |
| Epochs | 500 | Loss plateaus well before that |
| λ (L2 strength) | 0.001 | Light regularization; geographic features are correlated |
| Weight init | Xavier | Scales weights by 1/√(numFeatures) avoids vanishing gradients |

Normalization: z-score standardization per feature, fitted on the
training set only and applied identically to the test set to avoid data
leakage:

$\tilde{x} = \frac{x - \mu_{\text{train}}}{\sigma_{\text{train}}}$

---

## Train / Test Split

An 80/20 stratified split with a fixed seed (42) after a full shuffle of the
dataset. Shuffling is important here because the raw DVF file is roughly
ordered by department, without it, the test set would be geographically skewed.

| Split | Samples |
|---|---|
| Train | 680 016 |
| Test | 170 004 |

---

## Results

```
Epoch   0   RMSE:  350 469 EUR
Epoch  50   RMSE:  291 870 EUR
Epoch 100   RMSE:  267 803 EUR
Epoch 150   RMSE:  258 395 EUR
Epoch 200   RMSE:  254 808 EUR
Epoch 250   RMSE:  253 442 EUR
Epoch 300   RMSE:  252 911 EUR
Epoch 350   RMSE:  252 696 EUR
Epoch 400   RMSE:  252 603 EUR
Epoch 450   RMSE:  252 558 EUR

Results:
    Train RMSE :  252 534 EUR
    Test  RMSE :  254 246 EUR
```

The gap between train and test RMSE is only ~1 700 EUR (~0.7%), which
is a really good sign: the model is not overfitting. The L2 penalty and the sheer
volume of data (680k samples) both contribute to that stability.

---

## My Take on These Numbers

An RMSE of ~252 000 EUR on a dataset where prices span from €10 000 to
€2 000 000 is honestly a mixed result, and I think it's worth being honest
about why.

What went right:

- Convergence is clean and monotone. The loss drops from 350k to 252k in
  the first 100 epochs and then flatten, which is what we usually expect
  from a well-conditioned gradient descent.
- The near-zero train/test gap tells me the model generalised and didn't
  memorise noise. For 680k samples with only 14 features, that's expected,
  but still reassuring.

What limits the model:

- Label encoding for geographic features is the main bottleneck. City
  codes and section codes are assigned integers in encounter order, which
  implies a false ordinal relationship. Paris 1st and Paris 2nd get
  consecutive IDs that mean nothing to a linear weight. One-hot or embedding
  approaches would fix this but are much heavier.
- Linear models cannot capture interactions. The price of a 50m²
  apartment in Paris is fundamentally different from a 50m² house in
  rural Creuse, but a linear model sees both as $area \times w_{area} + dept \times w_{dept}$
  with no multiplicative term. A simple decision tree or polynomial regression
  would already capture part of this.
- The price distribution is heavily right-skewed. Working in log-price
  space (i.e., predicting $log(price)$ and exponentiating the output) would
  likely reduce RMSE significantly by making the target closer to Gaussian.
- No building age or floor level features are included yet. These are
  strong predictors but are absent or inconsistent in the raw DVF data.
- The lack of dataset is also a problem, we have only 1 year of data, which is
  not enough to capture the market trends, and we dont have enough data to 
  help the model learn the complex patterns of the real estate market.

Conclusion: For a first linear baseline built from zero on raw government
data, I consider this a good starting point rather than a final result.
The convergence behaviour is correct, the model is not overfit, and the next
logical steps are clear: log-transform the target, replace label encoding
with something geography-aware, and experiment with polynomial or tree-based
models as a second iteration.

---

##  Dataset :

[Link](https://static.data.gouv.fr/resources/demandes-de-valeurs-foncieres/20251018-234902/valeursfoncieres-2025-s1.txt.zip)

You will need to unzip the dataset into the "data" folder

---

## How to Build and Run

```bash
cmake -S . -B build
cmake --build build --config Debug

.\build\Debug\ml_price_guesser.exe
```
