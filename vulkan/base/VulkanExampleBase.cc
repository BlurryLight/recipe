#include "VulkanExampleBase.h"
#include <array>
#include <optional>
#include <set>
#include <string>
#include <vector>


const uint32_t kWidth = 800;
const uint32_t kHeight = 600;
static fs::path rootPath = fs::path(PD::ROOT_DIR);
static fs::path resourcePath = fs::path(PD::ROOT_DIR) / "resources";

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
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}


void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
        drawFrames();
    }
    vkDeviceWaitIdle(mDevice);
}

void VKApplicationBase::cleanup() {
    if (enableValidationLayers) { vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr); }
    for (auto framebuffer : mSwapchainFramebuffers) { vkDestroyFramebuffer(mDevice, framebuffer, nullptr); }
    for (auto imageView : mSwapchainImageViews) { vkDestroyImageView(mDevice, imageView, nullptr); }
    vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(mDevice, mRenderFinishedSemaphore, nullptr);
    vkDestroyFence(mDevice, mInFlightFence, nullptr);
    vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
    vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
    vkDestroyDevice(mDevice, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyInstance(mInstance, nullptr);
    mInstance = nullptr;
    glfwDestroyWindow(mWindow);
    mWindow = nullptr;
    glfwTerminate();
}

void VKApplicationBase::drawFrames() {
    vkWaitForFences(mDevice, 1, &mInFlightFence, VK_TRUE, /*timeout*/ UINT64_MAX);
    vkResetFences(mDevice, 1, &mInFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(mCommandBuffer, 0);
    recordCommandBuffer(mCommandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphore};
    // TODO: 研究一下
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &mRenderFinishedSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    // 实现很低效，每一帧都要等待上一阵结束
    VK_CHECK_RESULT(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mRenderFinishedSemaphore;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;

    vkQueuePresentKHR(mPresentQueue, &presentInfo);
}

void VKApplicationBase::initWindow() {
    glfwInit();
    assert(glfwVulkanSupported());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);// no opengl
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);    // currently we don't handle resize
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    mWindow = glfwCreateWindow(kWidth, kHeight, "VulkanExample", nullptr, nullptr);
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

void VKApplicationBase::createImageViews() {
    mSwapchainImageViews.resize(mSwapchainImages.size());
    for (size_t i = 0; i < mSwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = mSwapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;// 可以通过view swizzle image的数据，但是这里不需要
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;// 可以通过view swizzle image的数据，但是这里不需要
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;// 可以通过view swizzle image的数据，但是这里不需要
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;// 可以通过view swizzle image的数据，但是这里不需要

        createInfo.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;// 可以通过subresourceRange控制view可以看到哪些范围，这里默认COLOR
        createInfo.subresourceRange.layerCount = 1;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.baseMipLevel = 0;
        VK_CHECK_RESULT(vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]));
    }
}

void VKApplicationBase::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // 不关心之前的layout
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// 用于交换链

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;// 0 is the index , 对应fragment shader的 layout(location = 0) out vec4 outcolor;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassInfo{};
    subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassInfo.colorAttachmentCount = 1;
    subpassInfo.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassInfo;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK_RESULT(vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass));
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = (uint32_t *) code.data();

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}

void VKApplicationBase::createGraphicsPipeline() {
    auto vertShaderCode = DR::readFile(resourcePath / "shader" / "glsl" / "FirstTriangle" / "shader.vert.spirv");
    auto fragShaderCode = DR::readFile(resourcePath / "shader" / "glsl" / "FirstTriangle" / "shader.frag.spirv");
    // auto vertShaderCode = DR::readFile(resourcePath / "shader" / "hlsl" / "FirstTriangle" / "shader.vert.spirv");
    // auto fragShaderCode = DR::readFile(resourcePath / "shader" / "hlsl" / "FirstTriangle" / "shader.frag.spirv");
    CHECK(vertShaderCode.size() > 0, "Vert Shader not found");
    CHECK(fragShaderCode.size() > 0, "Frag Shader not found");

    auto vertShaderModule = createShaderModule(mDevice, vertShaderCode);
    auto fragShaderModule = createShaderModule(mDevice, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // vertShaderStageInfo.pName = "VSMain"; // for hlsl
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.pSpecializationInfo = nullptr;// macro definitnion

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // fragShaderStageInfo.pName = "PSMain"; // for hlsl
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.pSpecializationInfo = nullptr;// macro definitnion

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;// an advanced usage

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) mSwapchainExtent.width;
    viewport.height = (float) mSwapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = mSwapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pScissors = &scissor;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.viewportCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;// whether to clamp points outside NDC to edge. should be false default
    rasterizer.rasterizerDiscardEnable = VK_FALSE;// whether to discard all geometries. should be false
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;


    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;


    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    VK_CHECK_RESULT(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout));


    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisampling;
    pipelineCreateInfo.pDepthStencilState = nullptr;// TODO
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = mPipelineLayout;
    pipelineCreateInfo.renderPass = mRenderPass;
    pipelineCreateInfo.subpass = 0;// index
    // we can derive a pipelien from an existing pipeline
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(
            vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mGraphicsPipeline));


    vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
}

void VKApplicationBase::createFramebuffers() {
    CHECK(mSwapchainImageViews.size() > 0, "");
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());
    for (size_t i = 0; i < mSwapchainImageViews.size(); i++) {
        VkImageView attachments[] = {mSwapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.width = mSwapchainExtent.width;
        framebufferInfo.height = mSwapchainExtent.height;
        framebufferInfo.layers = 1;// number of layers in vkimage
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.attachmentCount = 1;
        vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]);
    }
}

void VKApplicationBase::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysicalDevice, mSurface);
    CHECK(queueFamilyIndices.isComplete(), "");
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;// command buffer will be record every frame individually
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VK_CHECK_RESULT(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool));
}

void VKApplicationBase::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;// can be submited directly
    allocInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(mDevice, &allocInfo, &mCommandBuffer));
}

void VKApplicationBase::recordCommandBuffer(VkCommandBuffer cmdBuf, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;// 这里有一个flag可以指明这个cmdbuf可以被多次提交
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK_RESULT(vkBeginCommandBuffer(mCommandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderpassInfo{};
    renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassInfo.renderPass = mRenderPass;
    renderpassInfo.framebuffer = mSwapchainFramebuffers[imageIndex];
    renderpassInfo.renderArea.offset = {0, 0};
    renderpassInfo.renderArea.extent = mSwapchainExtent;

    VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.0f}}};
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmdBuf, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) mSwapchainExtent.width;
    viewport.height = (float) mSwapchainExtent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = mSwapchainExtent;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
    vkCmdDraw(cmdBuf, /*vertex cound*/ 3, /* instance count*/ 1, /*fisrt vertex*/ 0, /*first instance*/ 0);

    vkCmdEndRenderPass(cmdBuf);
    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
}

void VKApplicationBase::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;// 创建时就是signaled的状态，否则第一帧会卡死

    VK_CHECK_RESULT(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore));
    VK_CHECK_RESULT(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore));
    VK_CHECK_RESULT(vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFence));
}
