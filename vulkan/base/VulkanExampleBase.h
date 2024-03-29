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
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createVertexBuffer();
        void createIndexBuffer();
        void createDesciptorSetLayout();
        void createUniformBuffers();
        void updateUniformBuffer(uint32_t imageIndex);
        void createDescriptorPool();
        void createDescriptorSets();


    protected:
        GLFWwindow *mWindow = nullptr;
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

        VkDescriptorSetLayout mDescriptorSetLayout = nullptr;
        VkPipelineLayout mPipelineLayout = nullptr;
        VkRenderPass mRenderPass = nullptr;
        VkPipeline mGraphicsPipeline = nullptr;

        // swapchain
        std::vector<VkImage> mSwapchainImages;
        std::vector<VkImageView> mSwapchainImageViews;

        VkBuffer mVertexBuffer = nullptr;
        VkDeviceMemory mVertexBufferMemory = nullptr;
        VkBuffer mIndexBuffer = nullptr;
        VkDeviceMemory mIndexBufferMemory = nullptr;

        std::vector<VkBuffer> mUniformBuffers;
        std::vector<VkDeviceMemory> mUniformBuffersMemory;
        std::vector<void *> mUniformBuffersMapped;

        std::vector<VkFramebuffer> mSwapchainFramebuffers;
        VkCommandPool mCommandPool = nullptr;
        VkCommandBuffer mCommandBuffer = nullptr;

        VkSemaphore mImageAvailableSemaphore;// trigged when image was aquired from wapchain
        VkSemaphore mRenderFinishedSemaphore;// triggerd when drawcalls are all issued
        VkFence mInFlightFence;

        VkDescriptorPool mDescriptorPool = nullptr;
        std::vector<VkDescriptorSet> mDescriptorSets;
};
