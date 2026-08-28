# STM32L4R5ZIT6P / Keil ML-DSA-65 移植说明

## 1. 当前状态

- Keil 工程：`MDK-ARM/ML-DSA.uvprojx`
- MCU：STM32L4R5ZITxP，Cortex-M4F，120 MHz
- 编译器：µVision 5.35，Arm Compiler 6.16（ArmClang）
- 默认实现：`MLDSA_IMPLEMENTATION_CLEAN`
- 实际构建：0 error、0 warning，已生成 `ML-DSA.axf` 和 `ML-DSA.hex`
- ROM：32,276 bytes（31.52 KiB）
- RW + ZI RAM：124,312 bytes（121.40 KiB）
- 链接器静态最大栈深：79,604 bytes，加少量工具链标记为 unknown 的路径；启动栈已设为 98,304 bytes。
- 堆：16,384 bytes。PQClean 当前版 `fips202.c` 的上下文通过 `malloc/free` 管理，故不能把堆改回 CubeMX 默认的 512 bytes。

上述数据来自 `MDK-ARM/ML-DSA/ML-DSA.map`、`ML-DSA.htm` 和本次真实构建日志。尚未连接开发板，因此串口实测周期、下载结果和板上测试结果不能伪造为已完成。

## 2. 源码来源与选择理由

原始剥离目录为：

- `../dilithium3/ml-dsa-65/m4f`
- `../dilithium3/ml-dsa-65/m4fstack`
- `../dilithium3/common`

本机完整 pqm4 源树位于 `C:/Users/3nigma/Desktop/armPrj/packageEverything/pqm4`，提交为 `cc2c1b992b602d285bd15991a566d5f17b34c1fa`。检查确认 `ml-dsa-65` 的绝大多数文件原本是 Git 符号链接，目标为 `ml-dsa-44/m4f`；当前剥离过程把符号链接变成了一行路径文本，因此 `api.h`、`sign.c`、`params.h` 等并非可编译源码。

pqm4 只提供 `m4f` 和 `m4fstack`，没有 clean/ref。使用 ArmClang 6.16 直接试编译时：

- `pointwise_mont.s` 可单独通过；
- `ntt.S` 因 `ldr =#symbol`、`vmov.w` 等失败；
- `smallntt_769.S` 因 `vpush.w/vpop.w`、后索引 `.w` 语法等失败；
- `vector.s` 因 `mov.w` 立即数、`smmulr.w`、`mls.w` 等失败；
- `common/keccakf1600.S` 因宏展开产生 `ror 0-0` 等表达式失败。

因此默认 Target 按方案 B 使用 PQClean 的真实 ML-DSA-65 clean 实现，来源为 PQClean 提交 `202a8f96315f9ed219387a50f7e40d04af037ea8`：

- `crypto_sign/ml-dsa-65/clean/*`
- `common/fips202.c`
- `common/fips202.h`

算法代码未塞入 HAL、UART 或调试打印。平台适配和测试均位于 `Core/Src`。

## 3. 实际复制文件

复制到 `Middlewares/MLDSA65`：

- API/参数：`api.h`、`params.h`
- 签名：`sign.c`、`sign.h`
- 多项式：`ntt.c/.h`、`poly.c/.h`、`polyvec.c/.h`
- 编解码：`packing.c/.h`
- 约减/舍入：`reduce.c/.h`、`rounding.c/.h`
- SHAKE 适配：`symmetric-shake.c`、`symmetric.h`
- Keccak/SHAKE：`fips202.c`、`fips202.h`
- 许可证：`LICENSE`

没有从原 `dilithium3/common` 编译任何文件。以下公共文件均未加入 Keil：benchmark/test 主程序、libopencm3 HAL、MPS2 HAL/CMSIS、AES、SHA-512、原汇编 Keccak 和原 randombytes。原目录本身未删除，便于后续追溯。

## 4. 实际 API

`api.h` 中真实 API 使用 PQClean 命名空间：

```c
PQCLEAN_MLDSA65_CLEAN_crypto_sign_keypair(...)
PQCLEAN_MLDSA65_CLEAN_crypto_sign_signature(...)
PQCLEAN_MLDSA65_CLEAN_crypto_sign_verify(...)
```

verify 返回 0 表示成功，-1 表示失败。上层在 `Core/Src/mldsa_port.c` 中封装为 `mldsa65_keypair/sign/verify`。

实际长度由 `api.h` 提供：公钥 1952 bytes、私钥 4032 bytes、签名 3309 bytes；测试代码不手写数组长度。

## 5. Keil Group、Include Path 和 Define

Group：

- `Application/MDK-ARM`
- `Application/Core`
- `Application/Test`
- `Middleware/ML-DSA-65`
- `Middleware/Keccak`
- `Drivers/STM32L4xx_HAL_Driver`
- `Drivers/CMSIS`

Include Paths：

```text
../Core/Inc
../Middlewares/MLDSA65
../Drivers/STM32L4xx_HAL_Driver/Inc
../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy
../Drivers/CMSIS/Device/ST/STM32L4xx/Include
../Drivers/CMSIS/Include
```

Defines：

```text
USE_HAL_DRIVER
STM32L4R5xx
MLDSA_IMPLEMENTATION_CLEAN
MLDSA_USE_DETERMINISTIC_TEST_RNG=0
MLDSA_TEST_REPEAT_COUNT=10
```

`mldsa_port.h` 已保留 `MLDSA_IMPLEMENTATION_M4F` 选择分支，但 m4f 源码尚未适配，不能仅切换宏就得到可构建优化版。优化 Target 必须在全部汇编通过 ArmClang 并完成 KAT/板上测试后再启用。

## 6. UART 配置

原 `.ioc` 没有 UART。当前按 NUCLEO-L4R5ZI 默认 ST-LINK VCP 配置：

- LPUART1
- PG7：TX，AF8
- PG8：RX，AF8
- 115200, 8 data bits, 1 stop bit, no parity, no flow control
- 调用 `HAL_PWREx_EnableVddIO2()` 为 GPIOG 的 VDDIO2 供电

ST 的 UM2179 说明 NUCLEO-L4R5ZI 默认通过 PG7/PG8 把 LPUART1 连接到 VCP。若实际硬件是自定义 STM32L4R5ZIT6P 板而非 NUCLEO，必须依据原理图修改 `main.c`、`stm32l4xx_hal_msp.c` 和 `.ioc`，不能直接沿用 PG7/PG8。

日志由 `uart_log.c` 直接调用 `HAL_UART_Transmit`，没有定义 `fputc` 或 `__io_putchar`，也不依赖 semihosting。

## 7. RNG 配置

- 默认使用 STM32 硬件 RNG。
- 系统时钟打开 HSI48，并把 RNG kernel clock 选择为 HSI48。
- `stm32l4xx_hal_conf.h` 已启用 `HAL_RNG_MODULE_ENABLED`。
- `randombytes()` 支持任意字节长度，末尾不足 4 bytes 时只复制需要的字节。
- HAL RNG 失败会清零剩余输出、记录 sticky error、打印 HAL error code；统一封装在算法返回后检查该错误，避免把算法核心忽略的 `randombytes()` 返回值误判为成功。
- 仅当 `MLDSA_USE_DETERMINISTIC_TEST_RNG=1` 时使用可重复 xorshift 测试源，并在启动时打印两条安全警告。该模式禁止用于产品。

## 8. SHAKE/Keccak 依赖

```text
sign.c/poly.c
  -> symmetric-shake.c / symmetric.h
  -> fips202.h
  -> fips202.c
  -> static KeccakF1600_StatePermute() in fips202.c
```

默认 Target 不编译 `dilithium3/common/keccakf1600.S`，因此不会与 C 实现产生重复符号。`shake128`、`shake256`、incremental/absorb/squeeze API 均只来自一份 `fips202.c`。

## 9. RAM、栈和堆

公钥、私钥、签名和修改后的消息都是 `mldsa_test.c` 的 static 缓冲区，不占用 `main()` 栈，也不输出完整私钥。

clean 算法内部仍有大局部变量。链接器报告：

- keypair 最大深度约 60,932 bytes；
- sign 最大深度约 79,508 bytes；
- verify 最大深度约 57,620 bytes；
- 全程序静态最大值 79,604 bytes。

启动文件的 96 KiB 栈有约 18 KiB 静态余量。若增加中断深度、RTOS、浮点 printf 或 OTA 调用链，必须重新审查栈；不能照搬当前数值。

## 10. 构建、烧录和串口

1. 用 Keil µVision 5.35 或兼容版本打开 `MDK-ARM/ML-DSA.uvprojx`。
2. 确认 Target 使用 Arm Compiler 6，选择 Build。
3. 输出位于 `MDK-ARM/ML-DSA/ML-DSA.axf` 和 `.hex`。
4. 连接 ST-LINK，在 Options for Target / Debug 中选择对应 ST-LINK 驱动，然后 Download。
5. 打开 ST-LINK Virtual COM Port，115200 8N1，无流控，复位板卡。

命令行构建：

```powershell
D:\Keil\UV4\UV4.exe -b .\MDK-ARM\ML-DSA.uvprojx -j0 -o build.log
```

## 11. 正常输出格式

下面是程序预期格式，不是未连接硬件时伪造的实测数据：

```text
[BOOT] ML-DSA-65 test start
[INFO] Public key size: 1952
[INFO] Secret key size: 4032
[INFO] Signature size: 3309
[TEST] Keypair generation start
[TIME] keypair cycles = <board measurement>
[TEST] Keypair generation success
[TEST] Signing start
[TIME] sign cycles = <board measurement>
[TEST] Signing success
[TEST] Verification start
[TIME] verify cycles = <board measurement>
[PASS] Valid signature accepted
[PASS] Modified message rejected
[PASS] Modified signature rejected
[PASS] Restored signature accepted
[PASS] Repeat 10/10 keypair/sign/verify
[RESULT] ALL TESTS PASSED
```

DWT 是 32-bit 计数器；120 MHz 下单次可无歧义覆盖约 35.79 秒。减法能处理一次自然回绕，但不能检测单次操作跨越两个及以上完整周期。

## 12. 已执行的正确性检查

除 ArmClang 目标构建外，还在主机上用相同 PQClean clean 源码执行了 keypair、sign、正确验签、篡改消息拒绝、篡改签名拒绝、恢复后成功，结果为 `HOST_MLDSA_TEST_PASS siglen=3309`。这验证算法源组合，但不能替代 STM32 上的 RNG、UART、栈和时序实测。

## 13. 常见错误与 HardFault 排查

- `HAL_UARTEx_*Callback` 未定义：必须同时加入 `stm32l4xx_hal_uart_ex.c`。
- RNG 无输出：确认 HSI48、RNG kernel clock、`HAL_RNG_MODULE_ENABLED` 和 `stm32l4xx_hal_rng.c`。
- VCP 无输出：确认是 NUCLEO-L4R5ZI、PG7/PG8 焊桥默认连接、VDDIO2 已使能。
- `malloc`/SHAKE 崩溃：确认堆仍为 0x4000，检查堆栈相撞。
- sign 进入 HardFault：首先检查 MSP 是否低于 `Stack_Mem`；clean sign 静态深度接近 80 KiB。
- m4f 汇编报错：不要屏蔽错误或只链接部分 m4f 对象；需逐文件修复并保证 ABI/符号一致。
- HardFault 调试：在 `HardFault_Handler` 断点，读取 MSP/PSP、CFSR/HFSR/BFAR/MMFAR，检查 FPU、未对齐访问、RNG 写越界和堆栈边界。

## 14. 后续接入 OTA Bootloader

当前工程不包含 OTA。后续建议把 `mldsa65_verify()` 作为独立验证边界：Bootloader 仅持有可信公钥，从固定升级包格式解析签名和消息摘要，验证成功后再写入/切换镜像；同时重新评估 Bootloader 分区的约 31.52 KiB clean 代码、约 58 KiB verify 栈、堆依赖、看门狗喂养和 DWT 不可用场景。接入前还应锁定 KAT、升级包域分离、版本回滚策略和公钥更新策略。

## 15. 尚未完成

- 未连接板卡，未实际下载、采集串口或测量板上 cycles/ms。
- 没有已配置 LED，当前只循环打印完成提示。
- pqm4 m4f/m4fstack 汇编尚未适配 ArmClang。
- `.ioc` 原项目标记为 `board=custom`；若硬件不是 NUCLEO-L4R5ZI，UART 引脚必须由用户原理图确认。
