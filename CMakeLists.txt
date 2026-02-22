cmake_minimum_required(VERSION 3.10)
project(MLPriceGuesser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_library(core src/parser.cpp src/encoder.cpp src/normalizer.cpp src/model.cpp)

add_executable(ml_price_guesser src/main.cpp)
target_link_libraries(ml_price_guesser PRIVATE core)