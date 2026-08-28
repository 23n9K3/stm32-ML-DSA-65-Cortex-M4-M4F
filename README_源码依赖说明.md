# ML-DSA-65 源码依赖说明

## 1. 原始目录结论

`dilithium3/ml-dsa-65` 只有两个实现目录：`m4f` 和 `m4fstack`，没有 `ref`、`clean` 或独立纯 C 实现。

两个目录中的绝大多数“文件”实际只有一行相对路径，例如 `../../ml-dsa-44/m4f/sign.c`。它们原本是 pqm4 Git 符号链接，在当前 Windows 剥离目录中已退化成普通文本文件；只有 `m4f/config.h` 是有效内容，定义 `DILITHIUM_MODE 3`。因此不能把当前 `dilithium3/ml-dsa-65` 直接加入 Keil。

## 2. 原始 m4f/m4fstack 文件清单

| 文件 | 类型 | 预期用途 | 当前状态 | 默认 Keil 编译 |
|---|---|---|---|---|
| api.h | H | 签名公开 API | 断开的符号链接文本 | 否 |
| config.h | H | `DILITHIUM_MODE=3` | 有效；m4fstack 再链接到它 | 否 |
| params.h | H | ML-DSA 参数和长度 | 断开的符号链接文本 | 否 |
| sign.c / sign.h | C/H | keypair/sign/verify/open | 断开的符号链接文本 | 否 |
| packing.c / packing.h | C/H | 公私钥和签名编解码 | 断开的符号链接文本 | 否 |
| poly.c / poly.h | C/H | 多项式操作与采样 | 断开的符号链接文本 | 否 |
| polyvec.c / polyvec.h | C/H | 向量/矩阵运算 | 断开的符号链接文本 | 否 |
| rounding.c / rounding.h | C/H | power2round/decompose/hint | 断开的符号链接文本 | 否 |
| reduce.h | H | 模约减宏/接口 | 断开的符号链接文本 | 否 |
| symmetric-shake.c / symmetric.h | C/H | SHAKE 流初始化 | 断开的符号链接文本 | 否 |
| smallpoly.c / smallpoly.h | C/H | 小模多项式辅助 | 断开的符号链接文本 | 否 |
| ntt.S / ntt.h | ASM/H | M4F NTT/iNTT | 链接文本；上游 GAS 汇编不兼容 ArmClang 6.16 | 否 |
| pointwise_mont.s / .h | ASM/H | 点乘 Montgomery | 链接文本；上游单文件可由 armclang 汇编 | 否 |
| smallntt_769.S / smallntt.h | ASM/H | 小模 NTT | 链接文本；上游 GAS 汇编不兼容 | 否 |
| vector.s / vector.h | ASM/H | reduce/caddq/向量辅助 | 链接文本；上游 GAS 汇编不兼容 | 否 |
| macros.i | ASM include | NTT 汇编宏 | 断开的符号链接文本 | 否 |
| macros_smallntt.i | ASM include | small NTT 汇编宏 | 断开的符号链接文本 | 否 |
| stack.c / stack.h | C/H | m4fstack 栈策略 | 仅 m4fstack；断开的符号链接文本 | 否 |

## 3. 原始 common 依赖审计

恢复上游 m4f 后，真实依赖为：

```text
sign.c -> randombytes.h / randombytes()
symmetric-shake.c, symmetric.h -> fips202.h
fips202.c -> KeccakF1600_StateXORBytes/StatePermute/StateExtractBytes
             -> common/keccakf1600.S (pqm4 默认)
```

原剥离 `dilithium3/common` 缺少 `fips202.c`、`fips202.h`、`keccakf1600.h` 和 `randombytes.h`；这些原本由未初始化的 `mupq` 子模块提供。它现有的 `randombytes.c` 依赖 libopencm3 或测试宏，不适合作为 STM32 HAL 最终随机源。

| 检查项 | 原 pqm4 m4f | 当前 clean Target |
|---|---|---|
| fips202.h | 是，原剥离缺失 | 是，PQClean `fips202.h` |
| shake128/shake256 | 是 | 是，单一 `fips202.c` |
| keccakf1600 | 是，pqm4 汇编 | 是，`fips202.c` 内部 static C permutation |
| randombytes | 是 | 是，STM32 HAL RNG 适配 |
| hal.h | 仅 profiling/common HAL 条件路径 | 否 |
| send_USART_str/sendfn | 仅 pqm4 测试框架 | 否 |
| libopencm3 | 原 randombytes/HAL 使用 | 否 |
| mupq | 原 fips202/header 来源 | 否，已采用自包含 PQClean clean 组合 |
| pqclean | 否 | 是，算法和 C Keccak 来源 |
| arm-none-eabi-gcc 专用头 | 算法无；汇编按 GNU AS | 否 |

## 4. GNU/GCC 专用内容

- 原 pqm4 有 `.S/.s`、`.syntax unified`、`.global`、`.type`、`.req`、GAS 宏与 C 预处理 include。
- `common/test.c`、HAL/MPS2/CMSIS 中含 `__attribute__`、inline asm 和 GCC builtins，但它们未加入 Keil。
- pqm4 构建系统通常传入 Cortex-M4、Thumb/FPU 等 GCC 参数；当前 Keil 由 Device/Target 生成等价 ArmClang CPU/FPU 参数。
- 当前 clean 算法 C 文件没有算法层 inline asm 或 GCC-only builtin。
- STM32 CMSIS 自身按 `__clang__` 选择 `cmsis_armclang.h`，这是正常的供应商兼容层。

## 5. 默认 Target 实际文件表

| 文件 | 所属目录 | 功能 | 编译进 Keil | 主要依赖者/依赖 | pqm4 公共依赖 | 平台相关 |
|---|---|---|---|---|---|---|
| api.h | Middlewares/MLDSA65 | PQClean ML-DSA-65 API/长度 | 头文件 | mldsa_port.h | 否 | 否 |
| params.h | Middlewares/MLDSA65 | K=6、L=5 等参数 | 头文件 | 全部算法模块 | 否 | 否 |
| sign.c/h | Middlewares/MLDSA65 | keypair/sign/verify/open | 是 | packing/polyvec/fips202/randombytes | randombytes、SHAKE 概念相同 | 否 |
| packing.c/h | Middlewares/MLDSA65 | pk/sk/sig 编解码 | 是 | sign.c；依赖 poly/polyvec | 否 | 否 |
| ntt.c/h | Middlewares/MLDSA65 | 纯 C NTT/iNTT | 是 | poly.c；依赖 reduce | 否 | 否 |
| poly.c/h | Middlewares/MLDSA65 | 多项式、采样、挑战 | 是 | sign/polyvec；依赖 NTT/SHAKE | SHAKE | 否 |
| polyvec.c/h | Middlewares/MLDSA65 | 矩阵/向量运算 | 是 | sign/packing；依赖 poly | 否 | 否 |
| reduce.c/h | Middlewares/MLDSA65 | Montgomery/Barrett 约减 | 是 | ntt/poly | 否 | 否 |
| rounding.c/h | Middlewares/MLDSA65 | rounding/hint | 是 | poly | 否 | 否 |
| symmetric-shake.c/h | Middlewares/MLDSA65 | 域分离 SHAKE 流 | 是 | poly；依赖 fips202 | 对应 pqm4 同类文件 | 否 |
| fips202.c/h | Middlewares/MLDSA65 | SHAKE128/256 和 C Keccak-f[1600] | 是 | sign/symmetric | 替代原 pqm4+mupq组合 | 否 |
| mldsa_port.c/h | Core | 统一 API、RNG 错误传播 | 是 | mldsa_test；依赖 api/randombytes | 否 | 是 |
| randombytes.c/h | Core | HAL RNG/测试 RNG | 是 | sign/mldsa_port；依赖 hrng/HAL | 替代 pqm4 randombytes | 是 |
| uart_log.c/h | Core | 阻塞式 LPUART1 文本/hex 日志 | 是 | main/test/randombytes | 否 | 是 |
| cycle_counter.c/h | Core | DWT CYCCNT | 是 | main/test；依赖 CMSIS | 否 | 是 |
| mldsa_test.c/h | Core | 正确/篡改/恢复/10 次测试 | 是 | main；依赖 port/log/counter | 否 | 是 |
| main.c | Core | HAL/clock/UART/RNG 初始化与总流程 | 是 | 全应用 | 否 | 是 |
| stm32l4xx_hal_msp.c | Core | LPUART1 PG7/PG8、RNG MSP | 是 | HAL | 否 | 是 |
| stm32l4xx_hal_uart.c/_ex.c | HAL Driver | UART 驱动 | 是 | uart_log/main | 否 | 是 |
| stm32l4xx_hal_rng.c | HAL Driver | RNG 驱动 | 是 | randombytes/main | 否 | 是 |

## 6. 唯一定义核验

默认 Target 中：

- `shake128`、`shake256` 及 incremental/absorb/squeeze API 只由 `fips202.c` 定义一次；
- `KeccakF1600_StatePermute` 是同一文件内的 static 函数，不导出冲突符号；
- 没有编译原 `common/keccakf1600.S`；
- 没有同时编译 m4f 和 clean 的同名实现；
- `randombytes` 只由 `Core/Src/randombytes.c` 定义一次；
- UART 句柄 `hlpuart1` 和 RNG 句柄 `hrng` 只在 `main.c` 定义，其他文件使用 `extern`。

## 7. 构建诊断记录

第一次完整链接的唯一错误是 `HAL_UARTEx_RxFifoFullCallback`、`HAL_UARTEx_TxFifoEmptyCallback`、`HAL_UARTEx_WakeupCallback` 未定义。原因是只加入了 `stm32l4xx_hal_uart.c`；加入供应商配套的 `stm32l4xx_hal_uart_ex.c` 后重新构建为 0 error、0 warning，没有通过空函数或屏蔽错误规避。
