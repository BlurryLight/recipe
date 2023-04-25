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
        void createSurface();
        void createSwapChain();

    protected:
    GLFWwindow* mWindow = nullptr;
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkPhysicalDevice mPhysicalDevice = nullptr;
    VkDevice mDevice = nullptr;
    VkQueue mGraphicsQueue = nullptr;// auto cleanup with VkDevice
    VkSurfaceKHR mSurface = nullptr;
    VkQueue mPresentQueue = nullptr;
    VkSwapchainKHR mSwapchain = nullptr;
    std::vector<VkImage> mSwapchainImages;
    VkFormat mSwapchainImageFormat = VK_FORMAT_MAX_ENUM;
    VkExtent2D mSwapchainExtent = {};
};
