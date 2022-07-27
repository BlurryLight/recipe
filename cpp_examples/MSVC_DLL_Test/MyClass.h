#pragma once

#ifdef SIMPLE_DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#else
// #define DLL_EXPORT __declspec(dllimport)
// 似乎dll import 不是必须的
#define DLL_EXPORT
#endif


void DLL_EXPORT print_value();
class DLL_EXPORT MyClass
{
    public:
        MyClass();
        void print();
        static int value;
};
