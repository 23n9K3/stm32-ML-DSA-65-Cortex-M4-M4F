# ML-DSA-65 汇编依赖分析

> 当前工程已拆为四个 Target：`MLDSA_CLEAN_M4`、`MLDSA_CLEAN_M4F`、`MLDSA_ASM_M4` 与 `MLDSA_ASM_M4F`。M4 汇编版只使用 Keccak 汇编并保留 C NTT；M4F 汇编版同时使用 Keccak、正向 NTT 和逆 NTT 汇编。四个版本均已通过板上完整测试。

分析日期：2026-07-14。源码基线：工作区 `dilithium3`，以及为解析工作区中失效 Git 符号链接而使用的本机既有 pqm4 副本，提交 `cc2c1b992b602d285bd15991a566d5f17b34c1fa`。本次没有下载源码。

## 结论

`dilithium3/common/keccakf1600.S` 是完整文件。`dilithium3/ml-dsa-65/m4f` 与 `m4fstack` 中除 `config.h` 外的算法文件不是源码，而是普通文本形式的失效符号链接，例如 `ntt.S` 的内容只有 `../../ml-dsa-44/m4f/ntt.S`。ML-DSA-65 通过 `config.h` 的 `DILITHIUM_MODE 3` 复用 ML-DSA-44 的 M4F 文件；不能把这些 25～39 字节占位文件直接加入 Keil。

## 汇编文件与符号

| 模块 | 工作区路径 | 真实目标/依赖 | 导出符号 | 替换或服务的 C 接口 | FPU |
|---|---|---|---|---|---|
| Keccak-f1600 | `dilithium3/common/keccakf1600.S` | 无外部符号 | `KeccakF1600_Initialize`, `KeccakF1600_StateXORBytes`, `KeccakF1600_StateExtractBytes`, `KeccakF1600_StatePermute` | `fips202.c` 的状态 XOR、提取和置换 | 否 |
| NTT + inverse NTT | `dilithium3/ml-dsa-65/m4f/ntt.S`（失效链接） | `../../ml-dsa-44/m4f/ntt.S`, `macros.i`；常量表在同一对象 | `pqcrystals_dilithium_ntt`, `pqcrystals_dilithium_invntt_tomont` | `PQCLEAN_MLDSA65_CLEAN_ntt`, `PQCLEAN_MLDSA65_CLEAN_invntt_tomont` 由 C wrapper 映射 | 使用 S0～S10 |
| pointwise | `.../m4f/pointwise_mont.s`（失效链接） | `../../ml-dsa-44/m4f/pointwise_mont.s` | `pqcrystals_dilithium_asm_pointwise_montgomery`, `pqcrystals_dilithium_asm_pointwise_acc_montgomery` | `poly_pointwise_montgomery`, `poly_pointwise_acc_montgomery` | 否 |
| vector | `.../m4f/vector.s`（失效链接） | `../../ml-dsa-44/m4f/vector.s` | `pqcrystals_dilithium_asm_reduce32`, `pqcrystals_dilithium_small_asm_reduce32_central`, `pqcrystals_dilithium_asm_caddq`, `pqcrystals_dilithium_asm_rej_uniform` | `poly_reduce`, `poly_caddq`, uniform rejection 等 | 否 |
| small NTT 769 | `.../m4f/smallntt_769.S`（失效链接） | `macros.i`, `macros_smallntt.i` | `small_ntt_asm_769`, `small_invntt_asm_769`, `small_pointmul_asm_769`, `small_asymmetric_mul_asm_769` | small polynomial/NTT 路径 | 使用 S0～S24；显式保存 S16～S24 |

这些汇编对象没有调用 libopencm3、mupq HAL、UART 或 randombytes。Keccak 内部辅助符号均为对象内局部符号。NTT 的 zeta 表属于同一 `ntt.S`，不可换用其他参数或版本的表。

## 调用链与状态布局

`symmetric-shake.c -> fips202.c -> KeccakF1600_*`。pqm4 Keccak 汇编内部使用 bit-interleaved 状态，因此不能只把 C 版 `KeccakF1600_StatePermute` 换成汇编：吸收必须经 `StateXORBytes`，输出必须经 `StateExtractBytes`。`fips202.c` 上层 SHAKE API 保持不变。

Keccak 汇编使用 LDRD/STRD，状态必须至少 8 字节对齐并连续占用 200 字节。当前上下文来自 Arm C library `malloc`，满足 AAPCS 的 8 字节对齐要求。NTT 输入仍是连续 `int32_t coeffs[256]`。

## GNU/ArmClang 兼容性

Keccak 原文件使用 `.syntax unified`, `.thumb`, `.macro`, `.if/.elseif/.endif`, `.rept`, `.global`, `.type`, `.align`, `.long` 和 C 预处理。ArmClang 适配文件仅调整立即数表达式，例如 `ror 32-x` 改为 `ror #(32-x)`，并单独处理旋转量 0；未修改轮常量、寄存器分配和状态布局。

NTT 原文件使用 GNU 的冗余 `.w` 后缀、`ldr reg, =#symbol`、C 预处理 include 和 FPU 寄存器。ArmClang 适配仅移除冗余 `.w` 与 `=#` 中的 `#`。独立汇编已通过。该文件需要 Cortex-M4F/FPv4-SP-D16；它用 S0～S10 作整数暂存，不传递浮点参数。S0～S15 为 AAPCS caller-saved，函数不调用其他函数。

## 宏和命名空间

- `dilithium3/ml-dsa-65/m4f/config.h`：`DILITHIUM_MODE 3`。
- Keil 实现选择：`MLDSA_USE_KECCAK_ASM`、`MLDSA_USE_NTT_ASM`。
- clean C 对外符号使用 `PQCLEAN_MLDSA65_CLEAN_...`；M4F 汇编使用 `pqcrystals_dilithium_...`。工程通过明确的 C wrapper 映射，没有删除或伪造命名空间。
- `.S` 必须经预处理；相同宏同时写入 Keil C/C++ 和 Assembler Defines。

## 当前接入状态

| 文件 | Keil Group | 当前状态 |
|---|---|---|
| `asm/keccak/keccakf1600_armclang.S` | Middleware/Keccak ASM | 编译、链接、map、板上 KAT 和 ML-DSA 测试通过 |
| `asm/m4f/ntt_armclang.S` + `macros.i` | Middleware/M4F ASM | ArmClang 汇编成功，入口解析成功；受本机 32 KiB Keil 链接许可限制，M4F Target 尚不能生成 AXF |
| pointwise/vector/small NTT | 未复制到工程 | 因 NTT 阶段未完成链接和板测，按阶段门禁没有继续 |
