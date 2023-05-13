#include "Fix64.h"
#include <climits>
#include <numeric>
#include <iostream>

#ifdef _MSC_VER
#include <intrin.h>
#include <immintrin.h>
#endif

Fix64::Fix64(int64_t val) { mRawValue = val * kOne; }

Fix64::operator int64_t() const { return (mRawValue + kHalf) >> kFracBit; }

Fix64::operator float() const { return (float)mRawValue / kOne; }

Fix64::operator bool() const { return this->mRawValue != 0; }

bool Fix64::operator!() const { return !bool(*this); }

Fix64::operator double() const { return (double)mRawValue / kOne; }

Fix64 Fix64::FromFloat(float val) {
  Fix64 f;
  f.mRawValue = int64_t(val * (float)kOne);
  return f;
}

Fix64 Fix64::FromDouble(double val) {
  Fix64 f;
  f.mRawValue = int64_t(val * (float)kOne);
  return f;
}

Fix64 Fix64::FromRaw(int64_t val) {
  Fix64 f;
  f.mRawValue = val;
  return f;
}

bool Fix64::operator==(const Fix64 &other) const {
  return this->mRawValue == other.mRawValue;
}

bool Fix64::operator!=(const Fix64 &other) const {
  return this->mRawValue != other.mRawValue;
}

bool Fix64::operator>(const Fix64 &other) const {
  // 内部的raw_value用有符号数的好处就是避免处理正负号的判断
  return this->mRawValue > other.mRawValue;
}

bool Fix64::operator>=(const Fix64 &other) const {
  return *this == other || *this > other;
}

bool Fix64::operator<(const Fix64 &other) const {
  return this->mRawValue < other.mRawValue;
}

bool Fix64::operator<=(const Fix64 &other) const {
  return *this == other || *this < other;
}

Fix64 Fix64::operator-() const { return Fix64::FromRaw(-this->mRawValue); }

int Fix64::Sign() const {
  if (this->mRawValue == 0) {
    return 0;
  }
  return this->mRawValue > 0 ? 1 : -1;
}

Fix64 Fix64::Abs() const {
  if (this->mRawValue == kMin)
    return Fix64::FromRaw(kMax);
  return this->FastAbs();
}

Fix64 Fix64::FastAbs() const {
  int64_t mask = (mRawValue >> (sizeof(mRawValue) * CHAR_BIT - 1));
  return Fix64::FromRaw((mRawValue + mask) ^ mask);
}

Fix64 Fix64::operator+(const Fix64 &other) const {
  return Fix64::FromRaw(this->mRawValue + other.mRawValue);
}

Fix64 Fix64::SafeAdd(const Fix64 &other) const {
  auto xlong = this->mRawValue;
  auto ylong = other.mRawValue;
  if (ylong > 0 && xlong > std::numeric_limits<int64_t>::max() - ylong) {
    return Fix64::FromRaw(std::numeric_limits<int64_t>::max());
  } else if (ylong < 0 && xlong < std::numeric_limits<int64_t>::min() - ylong) {
    return Fix64::FromRaw(std::numeric_limits<int64_t>::min());
  }
  return Fix64::FromRaw(xlong + ylong);
}

Fix64 Fix64::operator-(const Fix64 &other) const {
  return Fix64::FromRaw(this->mRawValue - other.mRawValue);
}
Fix64 Fix64::SafeMinus(const Fix64 &other) const {
  auto xlong = this->mRawValue;
  auto ylong = other.mRawValue;
  if (ylong > 0 && xlong < std::numeric_limits<int64_t>::min() + ylong) {
    return Fix64::FromRaw(std::numeric_limits<int64_t>::min());
  } else if (ylong < 0 && xlong > std::numeric_limits<int64_t>::max() + ylong) {
    return Fix64::FromRaw(std::numeric_limits<int64_t>::max());
  }
  return Fix64::FromRaw(xlong - ylong);
}
Fix64 Fix64::operator*(const Fix64 &other) const {
  // no overflow check
  // method 1
  // 定点数可以表示为 2^{-n} * F
  // A * B = (2^{-n} * A) * (2^{-n} * B) = 2^{-n} ( A * B *
  // 2^{-n}),括号内的既是需要计算的值
  // return Fix64::FromRaw(this->mRawValue * other.mRawValue >> kFracBit);

  // method 2 模拟十进制手算的方式,取出小数部分和整数部分,分开手算
//   uint64_t mask =
//       ~(~0ULL << kFracBit); // stupid trick to get all kFracBit 1 mask

//   uint64_t xfrac = this->mRawValue & mask;
//   uint64_t xinteger = this->mRawValue >> kFracBit;

//   uint64_t yfrac = other.mRawValue & mask;
//   uint64_t yinteger = other.mRawValue >> kFracBit;

//   auto xyff = xfrac * yfrac;
//   auto xyfi = xfrac * yinteger << kFracBit;
//   auto yxfi = yfrac * xinteger << kFracBit;
//   auto xyii = xinteger * yinteger << (kFracBit + kFracBit);

//   return Fix64::FromRaw(xyii + xyfi + yxfi + xyff);


    std::int64_t retHigh = 0;
    std::int64_t retLow = _mul128(this->mRawValue,other.mRawValue,&retHigh);
    uint64_t mask = ~(~0ULL << kFracBit);
    uint64_t hi = (retHigh & mask) << kIntegerBit ;
    uint64_t lo = retLow >> kFracBit;
    lo &= ~(~0ULL << kIntegerBit);
    return Fix64(hi | lo);
}

Fix64 Fix64::operator/(const Fix64 &other) const {
    // without int128_t, it's unusable
    // return Fix64::FromRaw(this->mRawValue << kFracBit / other.mRawValue);

    uint64_t mask = (std::numeric_limits<int64_t>::min() >> (kFracBit - 1));
    std::int64_t xhi = (this->mRawValue & mask) >> (kIntegerBit);
    std::int64_t xli = this->mRawValue << kFracBit;

    std::int64_t res = _div128(xhi, xli, other.mRawValue, nullptr);
    return Fix64::FromRaw(res);
}

std::string Fix64::GetDesc() const {
  char tmp[256];
  sprintf(tmp, "Fixed(%lld, %g)", this->GetRaw(), (double)(*this));
  return {tmp};
}