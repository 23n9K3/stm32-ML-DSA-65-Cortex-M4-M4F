# 第二阶段：Keccak 汇编结果

> 本文记录的是第二阶段历史数据。当前等价配置为 `MLDSA_ASM_M4`（Keccak ASM + C NTT），输出目录为 `MDK-ARM/Objects/ASM_M4/`。

## 验收结论

`MLDSA_CLEAN` 与 `MLDSA_KECCAK_ASM` 均由 Keil ArmClang 6.16 Rebuild All，0 error、0 warning。两个 HEX 都使用 STM32CubeProgrammer 2.22.0 下载并 verify 成功。目标板为 NUCLEO-L4R5ZI，MCU ID 0x470，串口 COM3 115200/8N1。

KECCAK_ASM 已通过 4 个 FIPS 202 SHAKE 已知答案测试、正确签名验签、消息篡改、签名篡改、恢复验签和 10 次 keypair/sign/verify。完整日志见 `MDK-ARM/serial_keccak_asm_full.log`；CLEAN 回归日志见 `MDK-ARM/serial_clean_full.log`。

## 构建与资源

| 指标 | CLEAN | KECCAK_ASM | 改善比例 |
|---|---:|---:|---:|
| Keypair cycles | 7,383,236 | 7,218,081 | 2.24% faster |
| Sign cycles | 22,220,182 | 10,736,802 | 51.68% faster |
| Verify cycles | 7,159,386 | 7,006,292 | 2.14% faster |
| Keypair ms | 61.526 | 60.150 | 2.24% faster |
| Sign ms | 185.168 | 89.473 | 51.68% faster |
| Verify ms | 59.661 | 58.385 | 2.14% faster |
| Code | 22,556 | 27,932 | -23.83%（增大） |
| RO data | 2,824 | 2,836 | -0.42%（增大） |
| RW data | 16 | 16 | 0% |
| ZI data | 124,296 | 124,296 | 0% |
| Code + RO | 25,380 | 30,768 | -21.23%（增大） |
| 静态最大栈 | 79,596 + Unknown | 79,408 + Unknown | 0.24% smaller |

周期是每个固件一次启动样本，SystemCoreClock 为 120 MHz；硬件 RNG 和签名拒绝采样会带来运行间波动，不应当作统计学 benchmark。栈值来自 Keil call graph，汇编和函数指针被标记为 Unknown，需结合水位法确认。

## map 证据

- CLEAN：`KeccakF1600_StatePermute` 来自 `fips202.o(.text.KeccakF1600_StatePermute)`；空汇编对象被移除。
- KECCAK_ASM：`KeccakF1600_StateXORBytes`、`KeccakF1600_StateExtractBytes`、`KeccakF1600_StatePermute` 均来自 `keccakf1600_armclang.o(.text)`。
- 两个 Target 的 Object/HEX/map 分别位于 `Objects/CLEAN` 与 `Objects/KECCAK_ASM`，没有交叉复用旧对象。

## 串口关键结果

```text
[KECCAK] SHAKE128 empty: PASS
[KECCAK] SHAKE256 empty: PASS
[KECCAK] SHAKE128 abc: PASS
[KECCAK] SHAKE256 abc: PASS
[KECCAK] ALL TESTS PASSED
[PASS] Valid signature accepted
[PASS] Modified message rejected
[PASS] Modified signature rejected
[PASS] Restored signature accepted
[PASS] Repeat 10/10 keypair/sign/verify
[RESULT] ALL TESTS PASSED
```

KAT 期望值来自 FIPS 202 SHAKE 定义并用 Python `hashlib.shake_128`/`shake_256` 交叉核对。测试只比较前 32 字节。

## 备份

- `../backup/baseline_clean`：修改前 CLEAN 基准。
- `../backup/stage2_keccak`：第二阶段通过后的完整工程副本。
