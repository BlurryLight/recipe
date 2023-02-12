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

Fix64::operator double()
{
    return (double)mRawValue / kOne;
}

Fix64 Fix64::FromFloat(float val)
{
    Fix64 f;
    f.mRawValue = val * kOne;
    return f;
}

Fix64 Fix64::FromDouble(double val)
{
    Fix64 f;
    f.mRawValue = val * kOne;
    return f;
}


Fix64 Fix64::FromRaw(int64_t val)
{
    Fix64 f;
    f.mRawValue = val;
    return f;
}