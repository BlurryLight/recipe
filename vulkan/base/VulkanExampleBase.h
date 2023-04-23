#pragma once
#include "VulkanUtils.hh"
#include <Volk/volk.h>
#include <spdlog/spdlog.h>


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VKApplicationBase
{
    public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    private:
        void createInstance();
        virtual void initVulkan();
        virtual void mainLoop();
        virtual void cleanup();
        void initWindow();
        void setupDebugMessenger();
        void pickPhysicalDevice();
        void createLogicalDevice();

    protected:
    GLFWwindow* mWindow = nullptr;
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;// auto cleanup with VkDevice
};
