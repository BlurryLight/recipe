#pragma once

#if defined(SIMPLE_DLL_EXPORT) && defined(_MSC_VER)
#define DLL_EXPORT __declspec(dllexport)
#else
// #define DLL_EXPORT __declspec(dllimport)
#define DLL_EXPORT
#endif

void DLL_EXPORT foo();
