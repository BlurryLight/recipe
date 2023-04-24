#include "VulkanExampleBase.h"
#include <optional>
#include <set>
#include <string>
#include <vector>


const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData) {


    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    // whether vulkan should be aborted
    return VK_FALSE;
}

#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = true;
#endif


static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // don't care  info level
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;// Optional
};

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
bool checkValidationLayerSupport() {
    uint32_t layCount = 0;
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&layCount, nullptr));
    std::vector<VkLayerProperties> availableLayers(layCount);
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&layCount, availableLayers.data()));

    for (auto layer : validationLayers) {
        auto It = std::find_if(availableLayers.begin(), availableLayers.end(),
                               [layer](const VkLayerProperties &prop) { return strcmp(prop.layerName, layer) == 0; });
        // one layer needed is not found
        if (It == availableLayers.end()) { return false; }
    }
    return true;
}

std::vector<const char *> getRequiredExtensions() {

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    std::vector<const char *> requiredExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);// 交换链相关的extensions
    assert(glfwExtensions);
    requiredExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) { requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }

    return requiredExtensions;
}

void VKApplicationBase::createInstance() {
    // init volk
    VK_CHECK_RESULT(volkInitialize());

    VkApplicationInfo appInfo{};
    appInfo.applicationVersion = VK_API_VERSION_1_0;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pNext = nullptr;


    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers not found");
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // https://github.com/KhronosGroup/Vulkan-Docs/blob/main/appendices/VK_EXT_debug_utils.adoc#examples
        // To capture events that occur while creating or destroying an instance an application can:
        // link a slink:VkDebugUtilsMessengerCreateInfoEXT structure to the pname:pNext
        // element of the slink:VkInstanceCreateInfo structure given to flink:vkCreateInstance.
        VkDebugUtilsMessengerCreateInfoEXT instanceCreateDebugInfo{};
        populateDebugMessengerCreateInfo(instanceCreateDebugInfo);
        createInfo.pNext = &instanceCreateDebugInfo;
    }

    // list supported extensions
    uint32_t supportedExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());
    std::set<std::string> supportedExtensionNameSet;
    for (const auto &Prop : supportedExtensions) {
        supportedExtensionNameSet.emplace(Prop.extensionName);
        spdlog::debug("available extention name {}", Prop.extensionName);
    }


    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
    // needed for macos, but on windows its also ok
    // requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    // createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    auto requiredExtensions = getRequiredExtensions();
    for (int i = 0; i < requiredExtensions.size(); i++) {
        auto extName = std::string(requiredExtensions[i]);
        spdlog::info("extention name {} / {}, {}", i + 1, requiredExtensions.size(), extName);
        if (supportedExtensionNameSet.find(extName) == supportedExtensionNameSet.end()) {
            spdlog::warn(fmt::format("Ext {} maybe not supported!", extName));
        }
    }
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &mInstance));

    volkLoadInstance(mInstance);
}

void VKApplicationBase::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}


void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) { glfwPollEvents(); }
}

void VKApplicationBase::cleanup() {
    if (enableValidationLayers) { vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr); }
    vkDestroyDevice(mDevice, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
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
void VKApplicationBase::setupDebugMessenger() {

    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger));
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
        return graphicsFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {

    QueueFamilyIndices queueIndices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps.data());

    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queueIndices.graphicsFamily = i; }
        if (queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { queueIndices.computeFamily = i; }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) { queueIndices.presentFamily = i; }
        if (queueIndices.isComplete()) { break; }
    }
    return queueIndices;
}

void VKApplicationBase::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (!deviceCount) { throw std::runtime_error("No available gpu device for vulkan found"); }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    spdlog::info("We have {} available GPUs", deviceCount);

    auto checkDeviceSuitable = [this](VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProps{};
        vkGetPhysicalDeviceProperties(device, &deviceProps);
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        spdlog::info("device Name: {}", deviceProps.deviceName);
        bool bDiscrete = deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        QueueFamilyIndices queueIndices = findQueueFamilies(device, mSurface);
        return bDiscrete && queueIndices.isComplete();
    };
    // we just pick the last one discrect gpu
    for (auto &device : devices) {
        bool bFound = checkDeviceSuitable(device);
        if (bFound) { mPhysicalDevice = device; }
    }
    CHECK(mPhysicalDevice != nullptr, "No PhysicalDevice");
    {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProps);
        spdlog::info("Final Device Name: {} ,device ID: {} ", deviceProps.deviceName, deviceProps.deviceID);
    }
}

void VKApplicationBase::createLogicalDevice() {

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice, mSurface);
    assert(indices.isComplete());
    // 尽管大概率一个QueueFamily就足以创建所有不同的Queue类型，但是这里还是假设他们可能属于多个不同的queue family
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        static float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    // currently we ignore any device extensions

    VK_CHECK_RESULT(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice));

    vkGetDeviceQueue(mDevice, /*queueFamilyIndex*/ indices.graphicsFamily.value(), /*queueIndex*/ 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, /*queueFamilyIndex*/ indices.presentFamily.value(), /*queueIndex*/ 0, &mPresentQueue);
    ENSURE(mGraphicsQueue == mPresentQueue, "NOT SAME QUEUE");
}

void VKApplicationBase::createSurface() {
    VK_CHECK_RESULT(glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface));
}
