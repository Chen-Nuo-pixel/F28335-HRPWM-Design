# 基于 F28335 的 HRPWM 设计与实现

本资料包用于配套论文《基于 F28335 的 HRPWM 设计与实现》，内容对应 TI F28335 高分辨率 PWM 相关例程 `lab29-HRPWM_SFO` 和 `lab30-HRPWM`，并在原例程基础上补充了两个创新工程：高分辨率呼吸灯工程和多频率自动切换工程。

## 资料包内容

```text
.
├─ 14-2310280374-陈诺-基于F28335的HRPWM设计与实现（对应例程lab29-HRPWM_SFO、lab30-HRPWM）.docx
├─ 14-2310280374-陈诺-基于F28335的HRPWM设计与实现（对应例程lab29-HRPWM_SFO、lab30-HRPWM）.pdf
├─ visio文件/
│  ├─ 1.vsdx
│  ├─ 2.vsdx
│  ├─ 3.vsdx
│  ├─ 5.vsdx
│  ├─ 6.vsdx
│  ├─ 7.vsdx
│  ├─ 23.vsdx
│  └─ 32.vsdx
└─ 例程及创新工程源程序/
   ├─ lab29-HRPWM_SFO/
   ├─ lab30-HRPWM/
   ├─ F28335_HRPWM_Breathing_LED_/
   └─ F28335_HRPWM_Multi_Frequency/
```

## 工程说明

| 工程目录 | 来源/用途 | 主要输出 | 关键文件 |
| --- | --- | --- | --- |
| `lab29-HRPWM_SFO` | TI 原始 HRPWM + SFO 标定例程 | `EPWM1A`~`EPWM4A` 输出带 MEP 微边沿调节的 PWM | `source/HRPWM_SFO.c` |
| `lab30-HRPWM` | TI 原始 HRPWM 边沿控制例程 | `EPWM1A/B`~`EPWM4A/B` 对比 HRPWM 与普通 PWM 输出 | `source/HRPWM.c` |
| `F28335_HRPWM_Breathing_LED_` | 基于 `lab29-HRPWM_SFO` 的创新工程 | `GPIO0 / EPWM1A` 输出占空比渐变 PWM，可用于 LED 呼吸灯 | `source/F28335_HRPWM_Breathing_LED.c` |
| `F28335_HRPWM_Multi_Frequency` | 基于 `lab30-HRPWM` 的创新工程 | `GPIO0 / EPWM1A` 输出 100 kHz、500 kHz、1 MHz、1.5 MHz 自动切换 PWM | `source/F28335_HRPWM_Multi_Frequency.c` |

## 运行环境

- 目标芯片：TI TMS320F28335
- 开发环境：Code Composer Studio 或兼容 TI C2000 老工程 `.pjt` 的 CCS 版本
- 依赖文件：DSP2833x 头文件、启动文件、链接命令文件、SFO 库文件
- 推荐观察设备：示波器或逻辑分析仪

资料包中的每个工程均包含 `include/`、`source/`、`cmd/`、`lib/` 等目录，通常可直接作为 CCS 工程打开或导入。

## 编译与下载步骤

1. 打开 Code Composer Studio。
2. 通过 `Project -> Import Existing CCS/CCE Eclipse Project` 或旧版 `.pjt` 工程打开方式导入目标工程。
3. 选择对应工程目录下的 `.pjt` 文件，例如：
   - `例程及创新工程源程序/lab29-HRPWM_SFO/HRPWM_SFO.pjt`
   - `例程及创新工程源程序/lab30-HRPWM/HRPWM.pjt`
   - `例程及创新工程源程序/F28335_HRPWM_Breathing_LED_/F28335_HRPWM_Breathing_LED.pjt`
   - `例程及创新工程源程序/F28335_HRPWM_Multi_Frequency/F28335_HRPWM_Multi_Frequency.pjt`
4. 检查工程包含路径是否指向本工程的 `include/` 目录。
5. 检查链接命令文件是否使用 `cmd/F28335.cmd` 或 RAM 调试用链接文件。
6. 编译工程，生成 `.out` 文件。
7. 连接 F28335 开发板，下载 `.out` 至目标板运行。
8. 使用示波器观察对应 GPIO 引脚的 PWM 波形。

## 重点工程功能

### 1. HRPWM 高分辨率呼吸灯工程

工程目录：

```text
例程及创新工程源程序/F28335_HRPWM_Breathing_LED_/
```

核心功能：

- 使用 `EPWM1A`，即 `GPIO0`，输出高分辨率 PWM。
- 通过 `SFO_MepDis()` 初始化 MEP 标定因子，通过 `SFO_MepEn()` 周期性刷新标定结果。
- 使用 `DutyFine` 变量在 `DUTY_MIN_Q15` 和 `DUTY_MAX_Q15` 之间递增/递减。
- 将 Q15 格式占空比换算为 `CMPA` 和 `CMPAHR`，实现比普通 PWM 更细的占空比调节。
- 可外接 LED 或观察 PWM 占空比变化，用于展示 HRPWM 的细分控制效果。

建议观察变量：

```text
UpdateFine
DutyFine
DutyDirection
DutyMin
DutyMax
MEP_ScaleFactor[1]
EPwm1Regs.CMPA.all
```

### 2. HRPWM 多频率自动切换工程

工程目录：

```text
例程及创新工程源程序/F28335_HRPWM_Multi_Frequency/
```

核心功能：

- 使用 `EPWM1A`，即 `GPIO0`，输出 50% 占空比 HRPWM。
- 通过频率表 `HrpwmFreqTable` 依次切换 4 档频率：
  - 100 kHz
  - 500 kHz
  - 1 MHz
  - 1.5 MHz
- 每档保持 `SEGMENT_HOLD_SEC` 秒，默认 8 秒。
- 运行过程中刷新 SFO 标定，使 HRPWM 的 MEP 微边沿控制保持有效。
- 适合用示波器验证不同频率下 HRPWM 输出波形与寄存器配置关系。

建议观察变量：

```text
current_freq_hz
current_tbprd
current_cmpa
current_mode_index
MEP_ScaleFactor[1]
EPwm1Regs.TBPRD
EPwm1Regs.CMPA.all
```

## 示波器观察点

常用输出引脚如下：

| ePWM 通道 | GPIO 引脚 | 说明 |
| --- | --- | --- |
| `EPWM1A` | `GPIO0` | 两个创新工程的主要输出口 |
| `EPWM1B` | `GPIO1` | `lab30-HRPWM` 中普通 PWM 对照输出 |
| `EPWM2A` | `GPIO2` | 原始例程输出 |
| `EPWM2B` | `GPIO3` | 原始例程对照输出 |
| `EPWM3A` | `GPIO4` | 原始例程输出 |
| `EPWM3B` | `GPIO5` | 原始例程对照输出 |
| `EPWM4A` | `GPIO6` | 原始例程输出 |

观察时建议开启 CCS Watch 窗口，同时查看寄存器变量和实际波形变化，便于对应论文中的 HRPWM 原理分析。

## 注意事项

1. `SFO_TI_Build_fpu.lib` 与 `SFO.h` 是 HRPWM SFO 标定所需文件，缺失会导致工程无法链接或运行异常。
2. 工程默认按 F28335 常见 `150 MHz SYSCLKOUT` 配置设计，若实际系统时钟不同，需要重新计算 `TBPRD` 和 `CMPA`。
3. HRPWM 的微边沿调节依赖 MEP 标定，运行前应确保 `MEP_ScaleFactor` 已完成初始化。
4. `Debug/` 目录下的 `.out`、`.obj`、`.map` 为已生成的调试输出文件，重新编译后可能会被覆盖。
5. 若 CCS 导入后路径报错，应优先检查 `include/`、`cmd/`、`lib/` 的工程引用路径。
6. Word 的临时文件 `~$*.docx` 是 Office 打开文档时产生的缓存文件，不属于正式资料内容。

## 与论文内容的对应关系

- `lab29-HRPWM_SFO`：对应 HRPWM 的 SFO 标定、MEP 微边沿控制和高分辨率占空比调节分析。
- `lab30-HRPWM`：对应 HRPWM 边沿控制、普通 PWM 与 HRPWM 对照实验。
- `F28335_HRPWM_Breathing_LED_`：对应基于 HRPWM 的高分辨率呼吸灯设计与实现。
- `F28335_HRPWM_Multi_Frequency`：对应基于 HRPWM 的多频率自动切换实验设计。
- `visio文件/`：存放论文中使用的结构图、流程图或实验示意图的 Visio 源文件。

## 快速验证建议

若只想快速验证资料包是否可用，建议优先运行两个创新工程：

1. 运行 `F28335_HRPWM_Breathing_LED_`，观察 `GPIO0 / EPWM1A` 占空比是否连续变化。
2. 运行 `F28335_HRPWM_Multi_Frequency`，观察 `GPIO0 / EPWM1A` 是否按 100 kHz、500 kHz、1 MHz、1.5 MHz 循环切换。

若两项均可观察到预期波形，说明 HRPWM 初始化、SFO 标定、ePWM 输出和工程依赖文件基本正常。
