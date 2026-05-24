#include "../include/encoder.hpp"
#include "../include/model.hpp"
#include "../include/parser.hpp"
#include "../include/tree.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static void assert_near(double actual, double expected, double tol = 1e-9) {
    if (std::abs(actual - expected) > tol) {
        std::cerr << "Expected " << expected << " but got " << actual
                  << " with tolerance " << tol << "\n";
        std::abort();
    }
}

static Data make_sample(int year, double property_value, double price_per_m2) {
    Data d{};

    d.propertyValue = property_value;
    d.pricePerM2 = price_per_m2;
    d.totalArea = 50.0;

    d.year = year;
    d.month = 6;

    d.deptCode = "75";
    d.cityCode = "75056";
    d.section = "AB";
    d.streetCode = "0010";

    d.localTypeCode = 2;
    d.totalCarrezArea = 50.0;
    d.realBuiltArea = 50.0;
    d.mainRooms = 2;
    d.lotCount = 1;

    d.landArea = 0.0;
    d.landType = "S";
    d.landTypeSpecial = "";

    d.macroPrixM2Moyen = 10000.0;
    d.macroNbMutations = 100;
    d.macroMedianIncome = 30000.0;

    d.bdnbBuildYear = 1980;
    d.bdnbDpeStatus = "C";
    d.bdnbHeight = 12.0;
    d.bdnbUnitCount = 20;

    d.riskInundation = 1;
    d.riskClay = 0;

    d.socioPovertyRate = 0.10;
    d.socioDensity = 21000.0;

    d.distSchool = 300.0;
    d.distHospital = 900.0;
    d.distSupermarket = 250.0;
    d.distPublicTransport = 150.0;

    d.marketPriceM2 = 9800.0;
    d.marketVolume = 120;

    d.sectionPriceM2 = 9500.0;
    d.buildingPriceM2 = 9600.0;
    d.cityPriceM2 = 9700.0;
    d.priceMomentum = 1.02;
    d.typeRollingPriceM2 = 9400.0;

    d.lastSalePriceM2 = 9000.0;
    d.monthsSinceLastSale = 24;

    d.wealthGapIndex = 1.1;
    d.lat = 48.8566;
    d.lon = 2.3522;
    d.greenScore = 0.7;
    d.noiseIndex = 0.2;
    d.serviceDensity = 0.8;

    return d;
}

static void test_label_encode() {
    LabelEncode enc;

    assert(enc.encode("") == -1);
    assert(enc.size() == 0);

    int a = enc.encode("A");
    int b = enc.encode("B");

    assert(a == 0);
    assert(b == 1);
    assert(enc.encode("A") == a);
    assert(enc.lookup("B") == b);
    assert(enc.lookup("missing") == -1);
    assert(enc.size() == 2);

    enc.reset();
    assert(enc.size() == 0);
    assert(enc.lookup("A") == -1);
}

static void test_target_encoder() {
    TargetEncoder enc(0.0);
    enc.fit({"A", "A", "B"}, {10.0, 14.0, 20.0});

    assert_near(enc.transform("A"), 12.0);
    assert_near(enc.transform("B"), 20.0);
    assert_near(enc.transform("unknown"), (10.0 + 14.0 + 20.0) / 3.0);
}

static void test_encoder_transform_contract() {
    std::vector<Data> train{
        make_sample(2020, 100000.0, 2000.0),
        make_sample(2023, 150000.0, 3000.0),
    };

    Encoder encoder;
    encoder.fit(train);

    std::vector<Data> raw{make_sample(2025, 200000.0, 4000.0)};
    auto encoded = encoder.transform(raw);

    assert(encoded.size() == 1);
    assert_near(encoded[0].target, 4000.0);
    assert_near(encoded[0].weight, 1.0);

    // Encoder::transform currently emits 64 features.
    // This protects the feature contract used by the GBDT.
    assert(encoded[0].feats.size() == 64);

    for (double value : encoded[0].feats) {
        assert(std::isfinite(value));
    }
}

static void test_parser_small_csv_maps() {
    {
        const std::string path = "test_market_indicators.csv";
        std::ofstream f(path);
        f << "code;year;volume;c3;c4;c5;c6;c7;price\n";
        f << "75056;2024;12;;;;;;9500,5\n";
        f.close();

        Parser parser;
        auto map = parser.parseMarket(path);
        std::remove(path.c_str());

        assert(map.size() == 1);
        assert(map.count("75056_2024") == 1);
        assert(map["75056_2024"].volume == 12);
        assert_near(map["75056_2024"].priceM2, 9500.5);
    }

    {
        const std::string path = "test_bdnb.csv";
        std::ofstream f(path);
        f << "code_commune,section,annee_construction,dpe,hauteur,nb_logements\n";
        f << "75056,A,1980,C,12.5,20\n";
        f.close();

        Parser parser;
        auto map = parser.parseBdnb(path);
        std::remove(path.c_str());

        assert(map.size() == 1);
        assert(map.count("75056_0A") == 1);
        assert(map["75056_0A"].buildYear == 1980);
        assert(map["75056_0A"].dpe == "C");
        assert_near(map["75056_0A"].height, 12.5);
        assert(map["75056_0A"].unitCount == 20);
    }

    {
        const std::string path = "test_indicateurs.csv";
        std::ofstream f(path);
        f << "row,code,year,mut,c4,c5,c6,c7,c8,price\n";
        f << "0,75056,2024,12,,,,,,9800.25\n";
        f.close();

        Parser parser;
        auto map = parser.parseIndicateurs(path);
        std::remove(path.c_str());

        assert(map.size() == 1);
        assert(map.count("75056_2024") == 1);
        assert(map["75056_2024"].nbMutations == 12);
        assert_near(map["75056_2024"].prixM2Moyen, 9800.25);
    }
}

static void test_decision_tree_split_and_serialization() {
    const int num_features = 1;
    std::vector<unsigned char> binned_feats{0, 0, 1, 1};
    std::vector<double> residuals{-1.0, -1.0, 1.0, 1.0};
    std::vector<double> weights{1.0, 1.0, 1.0, 1.0};
    std::vector<int> indices{0, 1, 2, 3};
    std::vector<std::vector<double>> thresholds{{0.0, 1.0}};

    DecisionTree tree(/*maxDepth=*/2, /*minSamplesSplit=*/1, /*lambda=*/0.0);
    tree.fit(binned_feats.data(), num_features, residuals, weights, indices, thresholds);

    assert_near(tree.predict({0.0}), -1.0);
    assert_near(tree.predict({1.0}), 1.0);

    std::stringstream buffer;
    tree.save(buffer);
    buffer.seekg(0);

    DecisionTree loaded;
    loaded.load(buffer);

    assert_near(loaded.predict({0.0}), -1.0);
    assert_near(loaded.predict({1.0}), 1.0);
}

static void test_gbdt_empty_training_throws() {
    GBDT model;
    bool thrown = false;

    try {
        model.fit({});
    } catch (const std::runtime_error&) {
        thrown = true;
    }

    assert(thrown);
}

static void test_gbdt_save_load_roundtrip() {
    std::vector<EncodeData> train{
        EncodeData{2.0, 1.0, {1.0}},
        EncodeData{2.0, 1.0, {1.0}},
        EncodeData{2.0, 1.0, {1.0}},
    };

    GBDT model(/*numTrees=*/3, /*learningRate=*/0.1, /*maxDepth=*/1,
               /*minSamplesSplit=*/1, /*subsampleRatio=*/1.0, /*lambda=*/0.0);
    model.fit(train);

    double before = model.predict({1.0});
    assert(std::isfinite(before));
    assert_near(before, 2.0);

    const std::string path = "test_gbdt_model.bin";
    model.save(path);

    GBDT loaded;
    loaded.load(path);
    std::remove(path.c_str());

    double after = loaded.predict({1.0});
    assert_near(after, before);
}

int main() {
    test_label_encode();
    test_target_encoder();
    test_encoder_transform_contract();
    test_parser_small_csv_maps();
    test_decision_tree_split_and_serialization();
    test_gbdt_empty_training_throws();
    test_gbdt_save_load_roundtrip();

    std::cout << "All ML-Price-Guesser core tests passed.\n";
    return 0;
}
