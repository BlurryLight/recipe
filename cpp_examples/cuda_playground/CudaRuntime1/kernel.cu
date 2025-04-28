#include <cstdio>
#include <vector>
#include <iostream>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <algorithm>
#include <random>


__global__ void addKernel(int* A, int* B, int* C, int NumElements)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < NumElements)
	{
		C[i] = A[i] + B[i];
	}
}

int main()
{

	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_real_distribution<float> distribution(0,10000);

	const int N = 1024 * 1024;
	const int size = N * sizeof(int);
	std::vector<int> h_A(N), h_B(N), h_C(N);
	for (int i = 0; i < N; i++)
	{
		h_A[i] = distribution(generator);
		h_B[i] = distribution(generator);
		h_C[i] = h_A[i] + h_B[i];
	}
	int* d_A, * d_B, * d_C;
	cudaMalloc(&d_A, size);
	cudaMalloc(&d_B, size);
	cudaMalloc(&d_C, size);
	cudaMemcpy(d_A, h_A.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_B, h_B.data(), size, cudaMemcpyHostToDevice);
	cudaMemset(d_C, 0, size);

	int blockSize = 256;
	int numBlocks = (N + blockSize - 1) / blockSize;
	addKernel <<<numBlocks, blockSize >>> (d_A, d_B, d_C, N);

	std::vector<int> h_C_result(N);
	cudaMemcpy(h_C_result.data(), d_C, size, cudaMemcpyDeviceToHost);
	if (!std::equal(h_C.begin(), h_C.end(), h_C_result.begin()))
	{
		std::cerr << "Error: Results do not match!" << std::endl;
	}
	else
	{
		std::cout << "Success: Results match!" << std::endl;
	}

   return 0;
}

