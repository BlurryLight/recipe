#include "VulkanExampleBase.h"
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
        spdlog::info("available extention name {}", Prop.extensionName);
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

void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) { glfwPollEvents(); }
}

void VKApplicationBase::cleanup() {
    if (enableValidationLayers) { vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr); }
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
