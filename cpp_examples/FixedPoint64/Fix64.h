// 64 bit
// 39.24 bit

#include <stdint.h>
#include <string>
#pragma once

constexpr int kIntegerBit = 40;
constexpr int kFracBit = 24;
class Fix64 {
private:
  int64_t mRawValue = 0;

public:
  Fix64() {}
  explicit Fix64(int64_t val);

  // constants
  static constexpr int64_t kMin = 1LL << 63;
  static constexpr int64_t kPosMin = 1LL;
  static constexpr int64_t kMax = ~(1LL << 63); // 0_ 11111.....
  static constexpr int64_t kOne = 1LL << kFracBit;
  static constexpr int64_t kHalf = 1LL << (kFracBit - 1);
  static constexpr double kDelta = 1.0 / kOne;

  // cast
  static Fix64 FromRaw(int64_t val);
  static Fix64 FromFloat(float val);
  static Fix64 FromDouble(double val);
  explicit operator int64_t() const;
  explicit operator float() const;
  explicit operator double() const;
  operator bool() const;

  // compare
  bool operator==(const Fix64 &other) const;
  bool operator!=(const Fix64 &other) const;
  bool operator>(const Fix64 &other) const;
  bool operator<(const Fix64 &other) const;
  bool operator>=(const Fix64 &other) const;
  bool operator<=(const Fix64 &other) const;

  // unary
  Fix64 operator-() const;
  bool operator!() const;

  // members
  int Sign() const;
  // return kMax at abs(kMin)
  Fix64 Abs() const;
  // return kMin at abs(kMin)
  Fix64 FastAbs() const;

  // binary
  Fix64 operator+(const Fix64 &other) const;
  Fix64 SafeAdd(const Fix64 &other) const;
  Fix64 operator-(const Fix64 &other) const;
  Fix64 SafeMinus(const Fix64 &other) const;
  Fix64 operator*(const Fix64 &other) const;
  Fix64 operator/(const Fix64 &other) const;

  // for debug
  int64_t GetRaw() const { return mRawValue; }
  std::string GetDesc() const;
};
