#pragma once
#include <Volk/volk.h>
#include <iostream>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>


#define VK_CHECK_RESULT(f)                                                                                             \
    {                                                                                                                  \
        VkResult res = (f);                                                                                            \
        if (res != VK_SUCCESS) {                                                                                       \
            std::cerr << "Fatal : VkResult is \"" << string_VkResult(res) << "\" in " << __FILE__ << " at line "       \
                      << __LINE__ << "\n";                                                                             \
            assert(res == VK_SUCCESS);                                                                                 \
        }                                                                                                              \
    }


#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
#define DEBUG_BREAK() __builtin_trap()
#else
#error "Unsupported compiler"
#endif


#define ENSURE(condition, message)                                                                                     \
    if (!(condition)) {                                                                                                \
        std::cerr << "Ensure Failed: " << #condition << " (file " << __FILE__ << ", line " << __LINE__                 \
                  << "): " << message << std::endl;                                                                    \
        DEBUG_BREAK();                                                                                                 \
    }

#define CHECK(condition, message)                                                                                      \
    if (!(condition)) {                                                                                                \
        DEBUG_BREAK();                                                                                                 \
        throw std::runtime_error("Check Failed: " + std::string(#condition) + std::string(message) + " (file " +       \
                                 __FILE__ + ", line " + std::to_string(__LINE__) + ")");                               \
    }