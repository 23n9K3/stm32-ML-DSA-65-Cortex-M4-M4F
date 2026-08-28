# 串口测试说明

1. 连接 NUCLEO-L4R5ZI 的 ST-LINK USB 接口。
2. 在 Windows 设备管理器中找到 STMicroelectronics Virtual COM Port。
3. 串口助手设置为 115200 baud、8 data bits、1 stop bit、no parity、no flow control。
4. 烧录 `MDK-ARM/ML-DSA/ML-DSA.hex` 后复位。
5. 以最终 `[RESULT] ALL TESTS PASSED` 为成功依据；该行每 5 秒重复一次。

必须同时看到：

```text
[PASS] Valid signature accepted
[PASS] Modified message rejected
[PASS] Modified signature rejected
[PASS] Restored signature accepted
[PASS] Repeat 10/10 keypair/sign/verify
```

若没有输出：

- 确认实际板卡是 NUCLEO-L4R5ZI/ZI-P；当前 VCP 使用 LPUART1 PG7/PG8 AF8。
- 确认 PG7/PG8 到 ST-LINK VCP 的板载焊桥仍为默认连接。
- 确认打开的是 ST-LINK VCP 而不是其他 USB 串口。
- 在 `MX_LPUART1_UART_Init` 后打断点，检查 `hlpuart1.gState`。
- 检查 `PWR->CR2` 的 IOSV 位；代码通过 `HAL_PWREx_EnableVddIO2()` 打开 VDDIO2。

若停在 RNG：检查 HSI48 ready、RNG 时钟、`RNG->SR` 和串口中的 HAL RNG error code。禁止把产品构建改成固定随机数来掩盖硬件 RNG 问题。

周期输出是 32-bit DWT CYCCNT 的板上实测值。120 MHz 下单次操作应小于约 35.79 秒，否则输出可能因多次回绕而失真。
