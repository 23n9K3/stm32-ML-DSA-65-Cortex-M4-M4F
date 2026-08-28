# Keil 四 Target 配置说明

工程使用 Keil ArmClang 6.16，并将 CPU/FPU 配置与算法实现分别拆开。四个 Target 共用 HAL、UART、硬件 RNG、ML-DSA 测试和 DWT 周期计数代码，但使用独立输出目录。

| Target | CPU 配置 | Keccak | NTT / inverse NTT | 输出目录 |
|---|---|---|---|---|
| `MLDSA_CLEAN_M4` | Cortex-M4，禁用 FPU | C | C | `MDK-ARM/Objects/CLEAN_M4/` |
| `MLDSA_CLEAN_M4F` | Cortex-M4F，FPv4-SP | C | C | `MDK-ARM/Objects/CLEAN_M4F/` |
| `MLDSA_ASM_M4` | Cortex-M4，禁用 FPU | Cortex-M4 ASM | C | `MDK-ARM/Objects/ASM_M4/` |
| `MLDSA_ASM_M4F` | Cortex-M4F，FPv4-SP | Cortex-M4 ASM | Cortex-M4F ASM | `MDK-ARM/Objects/ASM_M4F/` |

## 预处理宏

四个 Target 均定义 `USE_HAL_DRIVER`、`STM32L4R5xx`、`MLDSA_USE_DETERMINISTIC_TEST_RNG=0` 和 `MLDSA_TEST_REPEAT_COUNT=10`。

| Target | 实现选择宏 |
|---|---|
| `MLDSA_CLEAN_M4` | `MLDSA_IMPL_CLEAN=1`, `MLDSA_USE_KECCAK_ASM=0`, `MLDSA_USE_NTT_ASM=0`, `MLDSA_TARGET_M4F=0` |
| `MLDSA_CLEAN_M4F` | `MLDSA_IMPL_CLEAN=1`, `MLDSA_USE_KECCAK_ASM=0`, `MLDSA_USE_NTT_ASM=0`, `MLDSA_TARGET_M4F=1` |
| `MLDSA_ASM_M4` | `MLDSA_IMPL_CLEAN=0`, `MLDSA_USE_KECCAK_ASM=1`, `MLDSA_USE_NTT_ASM=0`, `MLDSA_TARGET_M4F=0` |
| `MLDSA_ASM_M4F` | `MLDSA_IMPL_CLEAN=0`, `MLDSA_USE_KECCAK_ASM=1`, `MLDSA_USE_NTT_ASM=1`, `MLDSA_TARGET_M4F=1` |

## M4 汇编版为何保留 C NTT

当前 `ntt_armclang.S` 使用 `S0-S10` 浮点寄存器，因此不能放入禁用 FPU 的 M4 Target。`MLDSA_ASM_M4` 只启用不依赖 FPU 的 Keccak 汇编，NTT 继续使用 C。只有 `MLDSA_ASM_M4F` 链入 `ntt_armclang.S`。

目标芯片仍是 STM32L4R5ZIT6P（物理内核为 Cortex-M4F）。这里的 M4 Target 表示编译器不生成 FPU 指令，且算法汇编不依赖 FPU，可用于比较无 FPU 的 Cortex-M4 配置。

## 构建与切换

切换 Target 后执行 `Rebuild All`。不要只执行增量 Build。每个 Target 的 AXF、HEX、map 位于各自 Objects 子目录，不会交叉复用目标文件。

工程修改前备份：`MDK-ARM/ML-DSA.before_four_targets.uvprojx.bak`。
