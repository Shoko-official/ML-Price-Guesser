cmake_minimum_required(VERSION 3.10)
project(MLPriceGuesser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenMP REQUIRED)
include_directories(include)

add_library(core src/parser.cpp src/encoder.cpp src/normalizer.cpp src/model.cpp src/tree.cpp)

add_executable(ml_price_guesser src/main.cpp)
target_link_libraries(core PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(ml_price_guesser PRIVATE core)