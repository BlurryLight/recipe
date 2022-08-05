#include <new>
#include <EASTL/vector.h>

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line){
	return new uint8_t[size];
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line){
	return new uint8_t[size];
}
#include <iostream>
void vla_test()
{
    int  n = 10;
    int  arr[n];
    // 我第一次知道不能用memset设置整数。。
    // 因为memset是按字节来的，1的字节表示是0x01,因此填充到int就会是0x01010101 16843009
    memset(arr, 0, sizeof(arr));
    for(int i =0;i <= 9 ;i++)
    {
        arr[i] = i  * 10;
    }
    std::cout << arr[9] << std::endl; //should be 90

}
int main()
{
    eastl::vector<int> vec;
    vec.push_back(1);
    return 0;
}