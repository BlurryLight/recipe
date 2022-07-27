#pragma once

#ifdef SIMPLE_DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif


void DLL_EXPORT print_value();
class DLL_EXPORT MyClass
{
    public:
        MyClass();
        void print();
        static int value;
};

class DLL_EXPORT SimpleDLLClass
{
  public:
    SimpleDLLClass(){};
    virtual ~SimpleDLLClass(){};

    virtual int getValue() { return m_nValue;};
  private:
    static int m_nValue;
};
