// 64 bit
// 39.24 bit

#include <stdint.h>
#pragma once

constexpr int kFracBit = 24;
class Fix64 {
private:
  int64_t mRawValue = 0;

public:
  Fix64() {}
  explicit Fix64(int64_t val);


  // constants
  static constexpr int64_t kOne = 1L << kFracBit;
  static constexpr int64_t kHalf = 1L << (kFracBit - 1);
  static constexpr double kDelta = 1.0 / kOne;

  // cast
  static Fix64 FromRaw(int64_t val);
  static Fix64 FromFloat(float val);
  static Fix64 FromDouble(double val);
  explicit operator int64_t();
  explicit operator float();
  explicit operator double();
  operator bool();

  // compare
  bool operator==(const Fix64& other);
  bool operator!=(const Fix64& other);
  bool operator>(const Fix64& other);
  bool operator<(const Fix64& other);
  bool operator>=(const Fix64& other);
  bool operator<=(const Fix64& other);
  
  // unary
  Fix64 operator-();
  bool operator!();

  // members
  int sign() const;

  // binary
  Fix64 operator+(const Fix64& other) const;
  Fix64 operator-(const Fix64& other) const;

};
