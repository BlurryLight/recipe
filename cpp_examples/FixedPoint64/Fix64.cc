#include <climits>
#include "Fix64.h"
Fix64::Fix64(int64_t val) {
    mRawValue = val * kOne;
}

Fix64::operator int64_t ()
{
    return (mRawValue + kHalf) >> kFracBit;
}

Fix64::operator float()
{
    return (float)mRawValue / kOne;
}

Fix64::operator bool ()
{
    return this->mRawValue != 0;
}

bool Fix64::operator !()
{
    return !bool(*this);
}

Fix64::operator double()
{
    return (double)mRawValue / kOne;
}

Fix64 Fix64::FromFloat(float val)
{
    Fix64 f;
    f.mRawValue = int64_t(val * (float)kOne);
    return f;
}

Fix64 Fix64::FromDouble(double val)
{
    Fix64 f;
    f.mRawValue = int64_t(val * (float)kOne);
    return f;
}


Fix64 Fix64::FromRaw(int64_t val)
{
    Fix64 f;
    f.mRawValue = val;
    return f;
}


bool Fix64::operator==(const Fix64& other)
{
    return this->mRawValue == other.mRawValue;
}

bool Fix64::operator!=(const Fix64& other)
{
    return this->mRawValue != other.mRawValue;
}

bool Fix64::operator>(const Fix64& other)
{
    // 内部的raw_value用有符号数的好处就是避免处理正负号的判断
    return this->mRawValue > other.mRawValue;
}

bool Fix64::operator>=(const Fix64& other)
{
    return *this == other || *this > other ;
}

bool Fix64::operator<(const Fix64& other)
{
    return this->mRawValue < other.mRawValue;
}

bool Fix64::operator<=(const Fix64& other)
{
    return *this == other || *this < other ;
}

Fix64 Fix64::operator-()
{
    return Fix64::FromRaw(-this->mRawValue);
}

int Fix64::Sign() const {
  if (this->mRawValue == 0) {
    return 0;
  }
  return this->mRawValue > 0 ? 1 : -1;
}

Fix64 Fix64::Abs() const {
  if(this->mRawValue == kMin) return Fix64::FromRaw(kMax);
  return this->FastAbs();
}

Fix64 Fix64::FastAbs() const {
  int64_t mask = (mRawValue >> (sizeof(mRawValue) * CHAR_BIT - 1));
  return Fix64::FromRaw((mRawValue + mask) ^ mask);
}

Fix64 Fix64::operator+(const Fix64& other) const {
    return Fix64::FromRaw(this->mRawValue + other.mRawValue);
}

Fix64 Fix64::operator-(const Fix64& other) const {
    return Fix64::FromRaw(this->mRawValue - other.mRawValue);
}