# RoboMaster 电控招新 · 作业四：C 板双 CAN 接口数据收发

许竣皓 / 202605311924 / 湘潭大学 智能制造工程

基于 **RoboMaster 开发板 C 型（STM32F407IGH6）**，用 **经典 CAN、标准数据帧、1 Mbps、8 字节** 实现两路
CAN 的收发：CAN1 每秒定时发送固定数据，CAN2 接收上位机报文并原样回传。

---

## 1. 报文定义

| 方向 | 报文 ID | 周期/触发 | DLC | 数据内容 |
|---|---|---|---|---|
| CAN1 → 上位机 | `0x100` | 每 1 s 定时 | 8 | `01 00 01 00 01 00 01 00`（固定值） |
| 上位机 → CAN2 | `0x200` | 手动发送 | 8 | `01 00 01 00 01 00 01 00` |
| CAN2 → 上位机 | `0x201` | 收到 `0x200` 后立即 | 8 | 原样回传收到的 8 字节 |

两个接口的分工：**CAN1 负责往外发状态（定时心跳），CAN2 负责收指令并回执**。实际比赛中这条链路对应
「底盘/云台向上位机广播状态」+「上位机下发控制指令并确认」的典型结构。

## 2. CAN 参数

### 2.1 位时序（两路相同）

| 参数 | 值 | 说明 |
|---|---|---|
| 时钟源 | HSE 12 MHz → PLL → SYSCLK 168 MHz | APB1 = 42 MHz（CAN 挂在 APB1） |
| Prescaler | 3 | tq = 3 / 42 MHz = 71.43 ns |
| BS1 / BS2 / SJW | 11 TQ / 2 TQ / 1 TQ | 一位 = 1 + 11 + 2 = 14 tq |
| 波特率 | 42 MHz / (3 × 14) = **1 Mbps** | |
| 采样点 | (1 + 11) / 14 = **85.7 %** | CiA 推荐 80~87.5 % |

### 2.2 过滤器

| | CAN1 | CAN2 |
|---|---|---|
| Filter Bank | 0 | 14 |
| Filter ID / Mask | `0x100` / `0x000`（全收，便于回环自测） | `0x200` / `0x7FF`（只收 0x200） |
| FIFO | FIFO0 | FIFO0 |

注意：CAN2 的过滤器组编号只能从 14 开始（0~13 归 CAN1）；`HAL_CAN_ConfigFilter` 必须在 `HAL_CAN_Start` 之前调用。

### 2.3 工作模式与高级参数

- 两路均为 `CAN_MODE_NORMAL`（题面要求 CAN1 也要能被上位机收到，故不用 Loopback）
- `AutoBusOff = DISABLE`、`AutoRetransmission = DISABLE`（调试期让故障立刻暴露，不掩盖成"时好时坏"）

## 3. 硬件与接线

| 项目 | 说明 |
|---|---|
| 开发板 | RoboMaster 开发板 C 型，STM32F407IGH6，24 V（8~28 V）输入 |
| CAN1 引脚 | PD0（RX）/ PD1（TX），收发器 U14 = TJA1044GTK/3 |
| CAN2 引脚 | PB5（RX）/ PB6（TX），收发器 U15 = TJA1044GTK/3 |
| 终端电阻 | 板载 R102 120 Ω（CAN1）、R91 120 Ω（CAN2），接一路时并联测得 60 Ω 为正常 |
| 上位机 | USB-CAN 适配器（单通道，虚拟串口 921600），按题面备注分别接 CAN1 / CAN2 分两次验收 |
| 接线 | 板 `CAN_H / CAN_L / GND` ↔ 适配器同名端，H、L **不可接反** |

> **最容易踩的坑**：C 板的 CAN 收发器由板上 DCDC 的 5 V 供电，只接 ST-Link 的 3.3 V 时能烧写成功，
> 但总线没有任何电平，表现为"烧录成功、代码不报错、一帧数据都没有"。必须接 24 V。

## 4. 代码结构

`Core/Src/main.c` 分三层：

| 层 | 内容 |
|---|---|
| 驱动层 | `Can_Filter_Init()` / `Can_Start()` / `Can_Send()`，都做返回值检查，失败进 `Error_Handler()` |
| 中断层 | `HAL_CAN_RxFifo0MsgPendingCallback()`：先 `HAL_CAN_GetRxMessage` 取帧，再按 `hcan->Instance` 与 `rx.StdId` 分发，只做"取帧 + 判断 + 回传"，不放 `printf`/`HAL_Delay` |
| 应用层 | 主循环用 `HAL_GetTick()` 做 1 s 软定时发送；`g_tx_count` / `g_echo_count` 便于 Keil Watch 观察 |

发送时基用 `HAL_GetTick` 而不是定时器中断，是因为作业四主循环无阻塞、1 ms 精度足够；到了 PID 那种
毫秒级周期就必须换成定时器中断驱动。

## 5. 构建

- IDE：Keil MDK-ARM（ARMCC V5.06）
- 工程文件：`MDK-ARM/can.uvprojx`，器件包 `Keil.STM32F4xx_DFP.2.13.0`
- 当前状态：**0 Error(s), 0 Warning(s)**

## 6. 验证结果

| 项 | 结果 |
|---|---|
| CAN1 定时发送 | 上位机连续记录 77 帧 `0x100`，帧间隔稳定 1.000 s，数据 `01 00 01 00 01 00 01 00` |
| CAN2 接收与回传 | 连续 5 次发送 `0x200`，均收到 `0x201` 且数据原样一致 |

技术日志（含接线说明、问题现象/原因/解决、上位机截图）：见提交压缩包内 `作业四视频及其文档/作业四开发日志.docx`。
