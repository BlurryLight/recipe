#include "VulkanUtils.hh"
#include <fstream>
std::vector<char> DR::readFile(fs::path filename) {
    std::ifstream file(filename.string(), std::ios::ate | std::ios::binary);// open at end
    CHECK(file.is_open(), "file not open");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);// return to first
    file.read(buffer.data(), fileSize);
    return buffer;
}
