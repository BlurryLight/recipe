#include <new>
#include <EASTL/vector.h>

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line){
	return new uint8_t[size];
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line){
	return new uint8_t[size];
}
int main()
{
    eastl::vector<int> vec;
    vec.push_back(1);
    return 0;
}
