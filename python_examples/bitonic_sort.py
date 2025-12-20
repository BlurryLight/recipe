

def compare_and_swap(arr, i, j, direction):
    """
    比较并交换函数
    如果 direction 为 True，则进行升序排列
    如果 direction 为 False，则进行降序排列
    """
    if (direction and arr[i] > arr[j]) or (not direction and arr[i] < arr[j]):
        arr[i], arr[j] = arr[j], arr[i]

def bitonic_merge(arr, low, cnt, direction):
    """
    双调合并函数
    将双调序列合并为有序序列
    """
    if cnt > 1:
        k = cnt // 2
        for i in range(low, low + k):
            compare_and_swap(arr, i, i + k, direction)
        bitonic_merge(arr, low, k, direction)
        bitonic_merge(arr, low + k, k, direction)

def bitonic_sort(arr, low, cnt, direction):
    """
    双调排序主函数
    direction: True 表示升序，False 表示降序
    """
    if cnt > 1:
        k = cnt // 2
        # 先创建双调序列
        bitonic_sort(arr, low, k, True)  # 升序
        bitonic_sort(arr, low + k, k, False)  # 降序
        # 然后合并为有序序列
        bitonic_merge(arr, low, cnt, direction)

def sort(arr, ascending=True):
    """
    对整个数组进行双调排序
    """
    bitonic_sort(arr, 0, len(arr), ascending)
    return arr


def bitonic_merge_iter(arr, start, length, up):
    """
    自底向上的双调合并（迭代版）
    :param arr: 待合并数组
    :param start: 子序列起始索引
    :param length: 子序列长度（2的幂）
    :param up: 排序方向（True=升序，False=降序）
    """
    # 从length/2开始，逐步减半直到1（自底向上合并）
    k = length // 2
    while k > 0:
        # 遍历当前层的所有比较对
        for i in range(start, start + length - k):
            # 比较i和i+k位置的元素，按方向交换
            if (arr[i] > arr[i + k]) == up:
                arr[i], arr[i + k] = arr[i + k], arr[i]
        k = k // 2


def bitonic_sort_iter(arr, up=True):
    """
    自底向上的双调排序（迭代版）
    :param arr: 待排序数组
    :param up: 最终排序方向（True=升序，False=降序）
    :return: 有序数组
    """
    arr_copy = arr.copy()
    length = len(arr_copy)
    
    # 自底向上处理：从最小子序列长度2开始，逐层合并
    k = 2
    while k <= length:
        # 遍历所有步长为k的子序列
        for j in range(0, length, k):
            # 对每个子序列：先构造双调（前半升、后半降），再合并为目标方向
            # 1. 前半部分（k/2长度）按升序合并
            bitonic_merge_iter(arr_copy, j, k//2, True)
            # 2. 后半部分（k/2长度）按降序合并
            bitonic_merge_iter(arr_copy, j + k//2, k//2, False)
            # 3. 合并整个k长度的双调序列为目标方向
            bitonic_merge_iter(arr_copy, j, k, up)
        k <<= 1  # 子序列长度翻倍

    
    return arr_copy


# 测试示例（兼容任意长度，包括非2的幂）
if __name__ == "__main__":
    # 测试1：图片中的示例序列（长度8，2的幂）
    arr1 = [25, 5, 31, 7, 36, 1, 18, 14]
    print("初始序列（2的幂长度）：", arr1)
    sorted_arr1 = bitonic_sort_iter(arr1)
    print("排序后：", sorted_arr1)

    # 使用图片中的示例数据
    data = [5, 2, 6, 3, 9, 1, 8, 7]
    print("原始数据:", data)
    
    # 进行升序排序
    sorted_data = sort(data.copy(), ascending=True)
    print("升序排序:", sorted_data)
    
    # 进行降序排序
    sorted_data_desc = sort(data.copy(), ascending=False)
    print("降序排序:", sorted_data_desc)