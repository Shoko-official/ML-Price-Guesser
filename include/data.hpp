#pragma once
#include <string>

struct Data {
  // Target
  double propertyValue;

  // Temporal
  int year;
  int month;

  // Geographic
  std::string deptCode;
  std::string cityCode;
  std::string section;
  std::string streetCode;

  // Building features
  int localTypeCode;
  double totalCarrezArea;
  double realBuiltArea;
  int mainRooms;
  int lotCount;

  // Land features
  double landArea;
  std::string landType;
  std::string landTypeSpecial;
};
