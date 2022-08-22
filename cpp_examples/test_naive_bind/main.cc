#include "binderlist_test.hpp"
#include "calleelist_test.hpp"
#include "binder_test.hpp"
#include <functional>
int foobar(int a,int& b)
{
    std::cout << a << std::endl;
    return ++b;
}
using Func = int(int,int&);
int main()
{
    // test_binder_list();
    // test_callee_list();
    // test_binder();

    int n = 0;
    int& n_ref = n;
    auto fn = Redchards_Bind(foobar,1,n_ref);
    std::cout << fn() << " " << n << std::endl;
    std::cout << fn() << " " << n << std::endl;

    int b = 0;
    int& b_ref = b;
    auto fn1 = std::bind(foobar,1,b_ref);
    std::cout << fn1() << " " << b << std::endl;
    std::cout << fn1() << " " << b << std::endl;
    return 0;
}