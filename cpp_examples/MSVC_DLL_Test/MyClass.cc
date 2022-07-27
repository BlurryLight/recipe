#include "MyClass.h"
#include <cstdio>

int MyClass::value = 1000;
int SimpleDLLClass::m_nValue = 2000;
MyClass::MyClass()
{
    value = 1001;
}
void MyClass::print()
{
    printf("My Class Value is %d",MyClass::value);
}

void print_value()
{
    static int func_var = 999;
    printf("Class value %d",MyClass::value);
    printf("func value %d",func_var);
}
