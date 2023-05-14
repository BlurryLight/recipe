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
        virtual void drawFrames();
        void initWindow();
        void setupDebugMessenger();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSurface();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffer();
        void recordCommandBuffer(VkCommandBuffer cmdBuf, uint32_t imageIndex);
        void createSyncObjects();
        void createVertexBuffer();

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
    VkFormat mSwapchainImageFormat = VK_FORMAT_MAX_ENUM;
    VkExtent2D mSwapchainExtent = {};

    VkPipelineLayout mPipelineLayout = nullptr;
    VkRenderPass mRenderPass = nullptr;
    VkPipeline mGraphicsPipeline = nullptr;

    // swapchain
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;

    VkBuffer mVertexBuffer = nullptr;
    VkDeviceMemory mVertexBufferMemory = nullptr;

    std::vector<VkFramebuffer> mSwapchainFramebuffers;
    VkCommandPool mCommandPool = nullptr;
    VkCommandBuffer mCommandBuffer = nullptr;

    VkSemaphore mImageAvailableSemaphore;// trigged when image was aquired from wapchain
    VkSemaphore mRenderFinishedSemaphore;// triggerd when drawcalls are all issued
    VkFence mInFlightFence;
};
