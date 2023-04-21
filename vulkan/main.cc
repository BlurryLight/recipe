#include  <VulkanExampleBase.h>

int main(int argc,char** argv)
{
    VKApplicationBase app;
    try {
        app.run();
    } catch (const std::exception& e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}