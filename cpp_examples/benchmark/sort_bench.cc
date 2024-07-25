#define PICOBENCH_DEBUG
#define PICOBENCH_IMPLEMENT_WITH_MAIN
#include "picobench.hpp"

#include <cstdlib>
#include <deque>
#include <random>

std::vector<uint32_t> InitVector(int size) {
  std::seed_seq seed{1234};
  std::mt19937 gen(seed);
  std::uniform_int_distribution<uint32_t> dis(0, (1 << 30) - 1);
  std::vector<uint32_t> v;
  v.reserve(size);
  for (int i = 0; i < size; ++i) {
    v.push_back(dis(gen));
  }

  // std::cout << "data:";
  // for (int i = 0; i < 5; i++) {
  //   std::cout << v[i] << " ";
  // }
  // std::cout << std::endl;
  return v;
}
void CheckSorted(const std::vector<uint32_t> &v) {
  if (!std::is_sorted(std::begin(v), std::end(v))) {
    std::cout << "Error: data is not sorted" << std::endl;
  }
}


#define PBRT_CONSTEXPR constexpr
static void RadixSort(std::vector<uint32_t> *v) {
    std::vector<uint32_t> tempVector(v->size());
    PBRT_CONSTEXPR int bitsPerPass = 6;
    PBRT_CONSTEXPR int nBits = 30;
    static_assert((nBits % bitsPerPass) == 0,
                  "Radix sort bitsPerPass must evenly divide nBits");
    PBRT_CONSTEXPR int nPasses = nBits / bitsPerPass;

    for (int pass = 0; pass < nPasses; ++pass) {
        // Perform one pass of radix sort, sorting _bitsPerPass_ bits
        int lowBit = pass * bitsPerPass;

        // Set in and out vector pointers for radix sort pass
        std::vector<uint32_t> &in = (pass & 1) ? tempVector : *v;
        std::vector<uint32_t> &out = (pass & 1) ? *v : tempVector;

        // Count number of zero bits in array for current radix sort bit
        PBRT_CONSTEXPR int nBuckets = 1 << bitsPerPass;
        int bucketCount[nBuckets] = {0};
        PBRT_CONSTEXPR int bitMask = (1 << bitsPerPass) - 1;
        for (const uint32_t&mp : in) {
            int bucket = (mp >> lowBit) & bitMask;
            // CHECK_GE(bucket, 0);
            // CHECK_LT(bucket, nBuckets);
            ++bucketCount[bucket];
        }

        // Compute starting index in output array for each bucket
        // 确定每个桶开始的index，这里实际上是一个prefix sum的操作，对应GPU上的scan
        int outIndex[nBuckets];
        outIndex[0] = 0;
        for (int i = 1; i < nBuckets; ++i)
            outIndex[i] = outIndex[i - 1] + bucketCount[i - 1];

        // 这里如果outIndex是原子的，那么这里完全可以做成并行的
        // Store sorted values in output array
        for (const uint32_t &mp : in) {
            int bucket = (mp >> lowBit) & bitMask;
            out[outIndex[bucket]++] = mp;
        }
    }
    // Copy final result from _tempVector_, if needed
    if (nPasses & 1) std::swap(*v, tempVector);
}

void std_sort_bench(picobench::state &s) {
  auto v = InitVector(s.iterations());
  {
    picobench::scope scope(s);
    std::sort(v.begin(), v.end());
  }
  CheckSorted(v);
}

void std_stable_sort_bench(picobench::state &s) {
  auto v = InitVector(s.iterations());
  {
    picobench::scope scope(s);
    std::stable_sort(v.begin(), v.end());
  }
  CheckSorted(v);
}

void heap_sort_bench(picobench::state &s) {
  auto v = InitVector(s.iterations());
  {
    picobench::scope scope(s);
    std::make_heap(v.begin(), v.end());
    std::sort_heap(v.begin(), v.end());
  }
  CheckSorted(v);
}

void radix_sort_bench(picobench::state &s) {
  auto v = InitVector(s.iterations());
  {
    picobench::scope scope(s);
    RadixSort(&v);
  }
  CheckSorted(v);
}


PICOBENCH(std_sort_bench)
    .iterations({64, 512, 4096, 32768, 262144, 2097152})
    .baseline();

PICOBENCH(std_stable_sort_bench)
    .iterations({64, 512, 4096, 32768, 262144, 2097152});

PICOBENCH(heap_sort_bench)
    .iterations({64, 512, 4096, 32768, 262144, 2097152});

PICOBENCH(radix_sort_bench)
    .iterations({64, 512, 4096, 32768, 262144, 2097152});


/*

clang 18.1.4 x86_64_w64_windows_gnu(ucrt64)  Release
// radix sort is fastest, heap sort is slowest

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 std_sort_bench *         |      64 |     0.002 |      23 |      - | 42666666.7
 std_stable_sort_bench    |      64 |     0.003 |      43 |  1.867 | 22857142.9
 heap_sort_bench          |      64 |     0.002 |      32 |  1.400 | 30476190.5
 radix_sort_bench         |      64 |     0.001 |      15 |  0.667 | 64000000.0
 std_sort_bench *         |     512 |     0.014 |      27 |      - | 36312056.7
 std_stable_sort_bench    |     512 |     0.012 |      23 |  0.865 | 41967213.1
 heap_sort_bench          |     512 |     0.021 |      40 |  1.482 | 24497607.7
 radix_sort_bench         |     512 |     0.005 |       9 |  0.340 |106666666.7
 std_sort_bench *         |    4096 |     0.142 |      34 |      - | 28743859.6
 std_stable_sort_bench    |    4096 |     0.114 |      27 |  0.801 | 35866900.2
 heap_sort_bench          |    4096 |     0.204 |      49 |  1.430 | 20098135.4
 radix_sort_bench         |    4096 |     0.043 |      10 |  0.299 | 96150234.7
 std_sort_bench *         |   32768 |     1.550 |      47 |      - | 21144737.7
 std_stable_sort_bench    |   32768 |     1.036 |      31 |  0.669 | 31614085.9
 heap_sort_bench          |   32768 |     2.128 |      64 |  1.373 | 15399943.6
 radix_sort_bench         |   32768 |     0.495 |      15 |  0.319 | 66238124.1
 std_sort_bench *         |  262144 |    13.606 |      51 |      - | 19267077.3
 std_stable_sort_bench    |  262144 |     9.942 |      37 |  0.731 | 26366534.9
 heap_sort_bench          |  262144 |    20.306 |      77 |  1.492 | 12909491.1
 radix_sort_bench         |  262144 |     4.593 |      17 |  0.338 | 57077164.3
 std_sort_bench *         | 2097152 |   130.585 |      62 |      - | 16059633.1
 std_stable_sort_bench    | 2097152 |    89.266 |      42 |  0.684 | 23493342.4
 heap_sort_bench          | 2097152 |   222.211 |     105 |  1.702 |  9437677.6
 radix_sort_bench         | 2097152 |    42.109 |      20 |  0.322 | 49802585.2
*/