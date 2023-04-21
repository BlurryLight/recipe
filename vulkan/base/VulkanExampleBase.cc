#include "VulkanExampleBase.h"
#include <set>
#include <string>
#include <vector>


const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

void VKApplicationBase::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.applicationVersion = VK_API_VERSION_1_0;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pNext = nullptr;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    // list supported extensions
    uint32_t supportedExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());
    std::set<std::string> supportedExtensionNameSet;
    for (const auto &Prop : supportedExtensions) {
        supportedExtensionNameSet.emplace(Prop.extensionName);
        spdlog::info("available extention name {}", Prop.extensionName);
    }


    std::vector<const char *> requiredExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);// 交换链相关的extensions
    assert(glfwExtensions);
    requiredExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
    // needed for macos, but on windows its also ok
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    for (int i = 0; i < requiredExtensions.size(); i++) {
        auto extName = std::string(requiredExtensions[i]);
        spdlog::info("extention name {} / {}, {}", i + 1, requiredExtensions.size(), extName);
        if (supportedExtensionNameSet.find(extName) == supportedExtensionNameSet.end()) {
            spdlog::warn(fmt::format("Ext {} maybe not supported!", extName));
        }
    }
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    createInfo.enabledExtensionCount = 0;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
    if (result != VK_SUCCESS) { throw std::runtime_error("Failed to create VKInstance"); }
}

void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) { glfwPollEvents(); }
}

void VKApplicationBase::cleanup() {
    vkDestroyInstance(mInstance, nullptr);
    mInstance = nullptr;
    glfwDestroyWindow(mWindow);
    mWindow = nullptr;
    glfwTerminate();
}

void VKApplicationBase::initWindow() {
    glfwInit();
    assert(glfwVulkanSupported());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);// no opengl
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);    // currently we don't handle resize
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    mWindow = glfwCreateWindow(kWidth, kHeight, "VulkanExmample", nullptr, nullptr);
    if (!mWindow) { throw std::runtime_error(fmt::format("Create Window {} x {} Failed!", kWidth, kHeight)); }
}