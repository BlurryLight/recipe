// g++ -O3 -std=c++17 -o fibtest fibtest.cc
// time: 40ms
#include <iostream>
#include <chrono>
int fibonacci(int n)
{
    return n < 1 ? 0
        : n <= 2 ? 1
        : fibonacci(n - 1) + fibonacci(n - 2);
}

int main()
{
    constexpr int N = 100'0000;
    auto start = std::chrono::steady_clock::now();
    for(int i = 0; i < N;i++)
    {
        int result = fibonacci(9);
        if(result != 34)
        {
            std::abort();
        }
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    return 0;
}