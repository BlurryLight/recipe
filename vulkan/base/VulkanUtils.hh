#pragma once
#include <Volk/volk.h>
#include <iostream>
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
