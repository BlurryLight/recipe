#pragma once
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VKApplicationBase
{
    public:
    void run()
    {
        initWindow();
        createInstance();
        initVulkan();
        mainLoop();
        cleanup();
    }
    private:
        void createInstance();
        virtual void initVulkan() {}
        virtual void mainLoop();
        virtual void cleanup();
        void initWindow();

    protected:
    GLFWwindow* mWindow = nullptr;
    VkInstance mInstance;
};
