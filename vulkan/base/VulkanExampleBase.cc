#include "VulkanExampleBase.h"


const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

void VKApplicationBase::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) { glfwPollEvents(); }
}

void VKApplicationBase::cleanup() {
    glfwDestroyWindow(mWindow);
    mWindow = nullptr;
    glfwTerminate();
}

void VKApplicationBase::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);// no opengl
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);    // currently we don't handle resize
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    mWindow = glfwCreateWindow(kWidth, kHeight, "VulkanExmample", nullptr, nullptr);
    if (!mWindow) { throw std::runtime_error(fmt::format("Create Window {} x {} Failed!", kWidth, kHeight)); }
}