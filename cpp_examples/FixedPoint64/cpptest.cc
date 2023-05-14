#include <Fix64.h>

int main()
{
    // .....-7941.02963524776 9441.716323266735 -0.841057850433304
    Fix64 aa = Fix64::FromDouble(-7941.02963524776);
    Fix64 bb = Fix64::FromDouble(9441.716323266735);
    auto var = aa / bb;
    return 0;
}