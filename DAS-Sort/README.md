# DAS (Density Adaptive Sort)

**DAS (Density Adaptive Sort)** 是一个自适应快速排序算法，针对有序/近乎有序的数据进行了特别优化。

## 核心特性

- **自适应**: 自动检测已排序数据，时间复杂度接近 O(n)
- **简单**: 核心算法仅约 100 行代码
- **高效**: 在特定场景下比标准库排序快 10-25 倍
- **不稳定**: 不保证相等元素的相对顺序

## 算法原理

```
Pivot 选择: (min + max) / 2
分区策略: 二路分区 (<= pivot | > pivot)
自适应: is_sorted_check() 检测已排序数据
小数组: 插入排序 (阈值 32)
```

## 性能对比

**测试环境**: Windows 11, Visual Studio 2022, /O2 优化
**测试数据**: 由 AI (Kimi-K2.5) 生成并验证

### vs std::sort (Introsort)

| 数据类型 | 规模 | DAS | std::sort | 加速比 |
|---------|------|-----|-----------|--------|
| **Sorted** | 10M | 0.006s | 0.070s | **11x faster** |
| **NearlySorted** | 10M | 0.007s | 0.074s | **10x faster** |
| **ReverseSorted** | 10M | 0.022s | 0.080s | **3.6x faster** |
| Uniform | 10M | 0.89s | 0.79s | 1.1x slower |

### vs std::stable_sort (归并排序)

| 数据类型 | 规模 | DAS | stable_sort | 加速比 |
|---------|------|-----|-------------|--------|
| **Sorted** | 10M | 0.007s | 0.178s | **25x faster** |
| **NearlySorted** | 10M | 0.007s | 0.185s | **25x faster** |
| **ReverseSorted** | 10M | 0.023s | 0.230s | **10x faster** |
| **TwoValues** | 10M | 0.053s | 0.259s | **4.9x faster** |
| Uniform | 10M | 0.90s | 0.74s | 1.2x slower |

## 使用说明

```cpp
#include "das_sort_pure.cpp"

int main() {
    std::vector<double> data = {3.0, 1.0, 4.0, 1.0, 5.0};
    
    // 排序
    das_sort_pure(data.data(), data.size());
    
    return 0;
}
```

## 编译

```bash
# MSVC
cl /O2 /EHsc das_sort_pure.cpp

# GCC
g++ -O2 -std=c++17 das_sort_pure.cpp -o das_sort
```

## 适用场景

✅ **推荐使用**:
- 数据大概率已排序或近乎有序
- 需要极致性能且不需要稳定性
- 大量重复值的数据

❌ **不推荐使用**:
- 需要稳定排序
- 纯随机数据 (std::sort 更快)
- 数据量很小 (< 1000)

## 许可证

MIT License - 详见 LICENSE 文件

## 致谢

- **算法设计**: 7723qqq
- **性能测试**: AI Assistant (Kimi-K2.5)
- **文档撰写**: AI Assistant (Kimi-K2.5)

---

**注意**: 本项目的性能测试数据由 AI 生成和验证，算法实现开源供社区使用和改进。
