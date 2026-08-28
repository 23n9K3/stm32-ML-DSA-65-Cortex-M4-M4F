# ML-DSA-65 四 Target 构建与实测对比

测试平台：NUCLEO-L4R5ZI / STM32L4R5ZIT6P，Keil ArmClang 6.16，硬件 RNG，串口 115200 8N1。以下四个固件均已 Rebuild、下载、verify，并在板上完成一轮完整测试和连续 10 次 keypair/sign/verify。

## 构建资源

| Target | Code | RO data | RW data | ZI data | Code + RO |
|---|---:|---:|---:|---:|---:|
| `MLDSA_CLEAN_M4` | 22,522 | 2,854 | 16 | 124,296 | 25,376 |
| `MLDSA_CLEAN_M4F` | 22,568 | 2,856 | 16 | 124,296 | 25,424 |
| `MLDSA_ASM_M4` | 25,508 | 2,948 | 16 | 124,296 | 28,456 |
| `MLDSA_ASM_M4F` | 27,752 | 4,996 | 16 | 124,296 | 32,748 |

`MLDSA_ASM_M4F` 使用 Keil size optimization level 7（ArmClang `-Oz`），其 Code + RO 距免费版链接器 32 KiB 限制仅余 20 字节。其余两个 CLEAN Target 使用优化等级 2；因此本表不能当作同编译选项下的严格代码尺寸基准。

## 单次启动周期

| 指标 | CLEAN_M4 | CLEAN_M4F | ASM_M4 | ASM_M4F |
|---|---:|---:|---:|---:|
| Keypair cycles | 7,381,852 | 7,383,790 | 7,342,937 | 7,039,196 |
| Sign cycles | 11,227,859 | 10,908,285 | 25,645,521 | 11,352,604 |
| Verify cycles | 7,158,011 | 7,159,455 | 7,180,905 | 6,724,262 |
| Keypair ms | 61.515 | 61.531 | 61.191 | 58.659 |
| Sign ms | 93.565 | 90.902 | 213.712 | 94.605 |
| Verify ms | 59.650 | 59.662 | 59.840 | 56.035 |

签名包含拒绝采样并使用硬件 RNG，单次 sign 数据波动很大；`ASM_M4` 的 213.712 ms 不能据此判定 Keccak 汇编更慢。严谨性能结论应统一优化等级、固定测试条件并统计多轮均值/中位数。纯 C 的 M4F 版也不会自动显著提速，因为该整数算法的 C 路径基本不使用浮点运算。

## 板上正确性

四个 Target 均通过：

- 正确签名验签成功；
- 修改消息后验签失败；
- 修改签名后验签失败；
- 恢复消息和签名后验签成功；
- 连续 10 次 keypair/sign/verify 全部成功；
- 无 HardFault。

两个汇编 Target 还通过 SHAKE128/SHAKE256 的 empty 和 `abc` 已知答案测试。完整串口日志位于：

- `MDK-ARM/serial_clean_m4.log`
- `MDK-ARM/serial_clean_m4f.log`
- `MDK-ARM/serial_asm_m4.log`
- `MDK-ARM/serial_asm_m4f.log`

## map 符号来源

| Target | `KeccakF1600_StatePermute` | ML-DSA NTT |
|---|---|---|
| `MLDSA_CLEAN_M4` | `fips202.o` | `ntt.o` 的 C 实现 |
| `MLDSA_CLEAN_M4F` | `fips202.o` | `ntt.o` 的 C 实现 |
| `MLDSA_ASM_M4` | `keccakf1600_armclang.o` | `ntt.o` 的 C 实现 |
| `MLDSA_ASM_M4F` | `keccakf1600_armclang.o` | `ntt_armclang.o` 导出的 `pqcrystals_dilithium_ntt` 和 `pqcrystals_dilithium_invntt_tomont` |

对应 map 文件位于 `MDK-ARM/Objects/<Target>/`。pointwise、vector 和 small NTT 汇编尚未接入。
