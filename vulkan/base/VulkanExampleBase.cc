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
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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

std::vector<const char *> getRequiredInstanceExtensions() {

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
        createInfo.enabledLayerCount = (uint32_t) validationLayers.size();
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

    auto requiredExtensions = getRequiredInstanceExtensions();
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
    createSwapChain();
}


void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) { glfwPollEvents(); }
}

void VKApplicationBase::cleanup() {
    if (enableValidationLayers) { vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr); }
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
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

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details{};
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities));

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    CHECK(formatCount > 0, "format should > 0");
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, nullptr);
    CHECK(presentCount > 0, "presentCount should > 0");
    details.presentModes.resize(presentCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, nullptr);

    return details;
}

// find proper surface format, default target is RGBA8 SRGB
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    ENSURE(false, "Should not be here");
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    if (std::find(availablePresentModes.begin(), availablePresentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) !=
        availablePresentModes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities) {
    // always get from glfw
    int width, height;
    // 在hidpi系统下， screen coords width/height != screen pixels.
    // 这里需要准确的pixels数量，所以需要重新回读
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width =
            DR::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
            DR::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    spdlog::info("SwapChain Extent: {}x{}", width, height);
    return actualExtent;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {

    QueueFamilyIndices queueIndices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queueIndices.graphicsFamily = i; }
        if (queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { queueIndices.computeFamily = i; }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) { queueIndices.presentFamily = i; }
        if (queueIndices.isComplete()) { break; }
    }
    return queueIndices;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // list supported device extension
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());

    std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());
    for (uint32_t index = 0; index < deviceExtensionCount && requiredExtension.size() > 0; index++) {
        requiredExtension.erase(availableDeviceExtensions[index].extensionName);
    }

    return requiredExtension.empty();
}

static bool checkDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    if (!checkDeviceExtensionSupport(device)) { return false; }

    VkPhysicalDeviceProperties deviceProps{};
    vkGetPhysicalDeviceProperties(device, &deviceProps);
    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    spdlog::info("device Name: {}", deviceProps.deviceName);
    bool bDiscrete = deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    QueueFamilyIndices queueIndices = findQueueFamilies(device, surface);

    auto swapChainSupportDetails = querySwapChainSupport(device, surface);
    bool bSwapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();

    return bDiscrete && queueIndices.isComplete() && bSwapChainAdequate;
};
void VKApplicationBase::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (!deviceCount) { throw std::runtime_error("No available gpu device for vulkan found"); }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    spdlog::info("We have {} available GPUs", deviceCount);

    // we just pick the last one discrect gpu
    for (auto &device : devices) {
        bool bFound = checkDeviceSuitable(device, mSurface);
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
    deviceCreateInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECK_RESULT(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice));

    vkGetDeviceQueue(mDevice, /*queueFamilyIndex*/ indices.graphicsFamily.value(), /*queueIndex*/ 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, /*queueFamilyIndex*/ indices.presentFamily.value(), /*queueIndex*/ 0, &mPresentQueue);
    ENSURE(mGraphicsQueue == mPresentQueue, "NOT SAME QUEUE");
}

void VKApplicationBase::createSurface() {
    VK_CHECK_RESULT(glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface));
}

void VKApplicationBase::createSwapChain() {
    SwapChainSupportDetails swapChainDetails = querySwapChainSupport(mPhysicalDevice, mSurface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(mWindow, swapChainDetails.capabilities);

    uint32_t imageCount = 3;// we prefer triple buffers
    imageCount = std::max(swapChainDetails.capabilities.minImageCount + 1,
                          imageCount);// if we set to minImage, driver may need addtional sync before swap

    // if maxImageCount == 0 , it means it is infility
    if (swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount) {
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }
    spdlog::info("Swapchain has {} buffers!", imageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;// only 1 if we need only one window
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice, mSurface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;// 允许1个image在不同的queue families使用
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;    // Optional
        createInfo.pQueueFamilyIndices = nullptr;// Optional
    }
    // we may apply vertical flip in swapchain
    createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
    // we don't need alpha in swapchain
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    // don't render pixels obscured by other window, but will affect readpixels()
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;// currently its null

    VK_CHECK_RESULT(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain));

    mSwapchainImageFormat = surfaceFormat.format;
    mSwapchainExtent = extent;
    mSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());
}
