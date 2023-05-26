#include <Fix64.h>

int main()
{
    Fix64 aa = Fix64::FromRaw(Fix64::kMax);
    Fix64 bb = Fix64(1);
    auto var = aa * bb;
    return 0;
}