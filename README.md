# Tesla FSD Controller — ESP32 Web 版

<p align="right"><a href="README_EN.md">English</a> | 中文</p>

> ## 🚫 重要公告：本项目对中国国内车辆已失效
>
> 自 **2026 年 3 月 31 日晚 9 时左右**，Tesla 向中国国内车辆远程下发了底层配置，从芯片层面禁用了 FSD 相关功能。**CAN 总线层面的修改已无法绕过此限制，本项目对中国国内车辆不再有效。**
>
> 注意：封禁期间 **FSD 账号授权不受影响**，已购买或订阅的 FSD 资格仍然保留，仅本 mod 失效。
>
> 海外车辆暂不受影响。本项目代码保留供学习、研究及海外用户使用。

---

基于 [tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) 的 ESP32 + WiFi 控制面板版本。

烧录后，ESP32 会创建一个 WiFi 热点，手机连上就能用浏览器实时控制所有参数，**无需重新编程**。

---

## 与市售闭源模块的对比

市面上存在售价 ¥500 左右的闭源 FSD 激活模块，以下是完整对比：

### 功能对比

| 功能 | 闭源商业模块 | 本项目 |
|------|-------------|--------|
| FSD 激活（HW3 / HW4 / Legacy）| ✅ | ✅ |
| 车速偏移 | ✅ | ✅ |
| 驾驶模式调节 | ✅ | ✅ |
| 滚轮调整车速 | ✅ | ✅ |
| 强制激活模式 | ✅ | ✅ |
| Web 控制面板 | ❌ | ✅ |
| OTA 无线升级 | ❌ | ✅ |
| 开源可审计 | ❌ | ✅ |
| 2026.2.9 封锁影响 | 同样受影响 | 同样受影响 |

### 其他差异

| 对比项 | 闭源商业模块 | 本项目 |
|--------|-------------|--------|
| **价格** | ¥500 | ¥30–99（硬件成本） |
| **代码** | 闭源黑盒，无法验证内部逻辑 | 完全开源，GPL，可自行审计 |
| **运行时配置** | 通常需要换固件 | Web 面板实时切换，断电不丢失 |
| **一车一码** | 是（换车需重新购买） | 否，通用固件 |
| **芯片** | 非公芯片，不透明 | 标准 ESP32，公开规格 |
| **安装方式** | 即插即用 | 需自行接线刷机 |

> 本项目不提供商业服务，所有代码免费开放。如果你愿意自己动手，硬件成本不超过 ¥100。

---

## ⚡ 快速刷机（推荐，无需安装任何软件）

> 使用预编译固件，全程用浏览器完成，5 分钟搞定。

### 第一步：选择对应你的板子的固件

> 每个板子有两个固件文件，用途不同，请注意区分：
> - **`firmware_xxx.bin`**（合并固件）→ 首次刷机用，通过 ESP Web Flasher 写入，地址 `0x0`
> - **`firmware_xxx_ota.bin`**（OTA 固件）→ 后续无线升级用，通过控制面板「固件更新」上传

| 板子 | 首次刷机固件 | OTA 升级固件 | 供电 |
|------|------|------|------|
| 标准 ESP32 开发板 + SN65HVD230 | [firmware_esp32.bin](https://github.com/wjsall/tesla-fsd-controller/releases/latest/download/firmware_esp32.bin) | [firmware_esp32_ota.bin](https://github.com/wjsall/tesla-fsd-controller/releases/latest/download/firmware_esp32_ota.bin) | 5V USB |
| **Waveshare ESP32-S3-RS485-CAN** | [firmware_esp32s3_waveshare.bin](https://github.com/wjsall/tesla-fsd-controller/releases/latest/download/firmware_esp32s3_waveshare.bin) | [firmware_esp32s3_waveshare_ota.bin](https://github.com/wjsall/tesla-fsd-controller/releases/latest/download/firmware_esp32s3_waveshare_ota.bin) | **7–36V 直接供电** 或 **USB-C** |

### 第二步：刷入固件

1. 用 **Chrome 或 Edge** 打开 [ESP Web Flasher](https://esp.huhn.me/)
2. 板子通过 **Type-C** 连接电脑 → 点击「**Connect**」选择串口
3. 点「**Add File**」选择刚下载的固件，地址填 `0x0`
4. 点「**Program**」等待完成
5. 连接 WiFi 热点 `FSD-Controller`（密码 `12345678`），浏览器访问 `192.168.4.1`

> ⚠️ Safari 不支持 WebUSB，请使用 Chrome 或 Edge。
>
> **Waveshare ESP32-S3 进入烧录模式：** 按住板子上的 **BOOT 键不松**，按一下 **RST 键**后松开，再松开 BOOT 键，然后点击 Connect 并在弹出窗口中选择端口。烧录完成后板子会自动重启。

<p align="center">
  <img src="images/screenshot_zh.jpg" width="45%" alt="Web UI - 中文">
  &nbsp;&nbsp;
  <img src="images/screenshot_en.jpg" width="45%" alt="Web UI - English">
</p>

---

## ⚠️ 重要声明（请先读完）

1. **仅限已购买 FSD 的车辆** — 本设备只是解锁 CAN 总线层面的限制，Tesla 服务器端必须有有效的 FSD 授权，否则不生效。
2. **风险自担** — CAN 总线直接连接车辆关键系统，发送错误报文可能导致不可预期的行为或电子部件损坏。本项目仅供学习和测试用途，作者不承担任何因使用本项目造成的车辆损坏、人身伤害或其他损失。
3. **保修失效** — 对车辆 CAN 总线进行改装可能导致 Tesla 官方保修失效，风险由使用者自行承担，作者对此不承担任何责任。
4. **遵守当地法律法规** — 在公共道路上使用辅助驾驶功能须符合你所在地区的交通法规。部分功能在特定地区可能存在法律限制，请在使用前自行了解并遵守当地相关法律，作者不对任何法律后果负责。
5. **交通事故免责** — 使用本设备期间发生的任何交通事故，包括但不限于车辆碰撞、财产损失、人身伤亡，责任由驾驶员本人承担，作者及项目贡献者不承担任何连带责任。驾驶时请始终保持专注，手握方向盘，随时准备接管车辆。
6. **不适用于公共道路测试** — 初次使用请在私有场地进行，确认功能正常后再上路。

---

## 所需材料

> 以下材料在淘宝/京东均可购买，搜索括号内的关键词即可。

### 方案一：标准 ESP32 开发板（入门推荐）

| 材料 | 说明 | 参考价 |
|------|------|--------|
| ESP32 开发板 | 搜「ESP32-DevKitC」或「ESP32 38Pin 开发板」 | ¥20–35 |
| SN65HVD230 模块 | 搜「SN65HVD230 CAN 模块 3.3V」，注意买 **3.3V** 版本 | ¥5–15 |
| 杜邦线（母对母）| 搜「杜邦线 母对母 20cm」，买一包备用 | ¥5 |
| USB 数据线 | Micro-USB 或 Type-C，取决于你的 ESP32 型号，用于烧录 | 通常自备 |

> **为什么选 ESP32？** 内置 WiFi 和原生 CAN（TWAI）控制器，无需额外芯片，成本最低。

### 方案二：Waveshare ESP32-S3-RS485-CAN（推荐车内永久安装）

| 材料 | 说明 | 参考价 |
|------|------|--------|
| **Waveshare ESP32-S3-RS485-CAN** | [淘宝购买链接](https://m.tb.cn/h.ikuhpPd)，CAN 收发器已集成，导轨安装外壳 | [**¥99**](https://m.tb.cn/h.ikuhpPd) |
| Type-C 数据线 | 烧录或 USB-C 车内供电时使用 | 通常自备 |

> **为什么选这个板？** 支持两种供电方式：**直接接车内 12V**（7–36V 宽压端子供电，无需降压模块）或 **USB-C 供电**。CAN 收发器已集成（TJA1051T），端子排螺丝接线（无需杜邦线），DIN 导轨外壳适合车内隐蔽安装。120Ω 终端电阻默认未接入，无需处理。

---

## 接线方法

> 🚫 **禁止使用 OBD2 接口**：OBD2 诊断口连接的是诊断 CAN 总线，经过车辆网关 ECU 隔离，修改后的报文**永远无法到达 Autopilot 电脑**，设备将完全无效。必须直接接车辆内部 CAN 总线（X179 / X652 连接器）。
>
> ⚠️ **以下接线位置仅适用于 Model 3 / Model Y，已经过社区验证。Model S、Model X、Model 3 Highland、Model Y Juniper、Cybertruck 等其他车型的连接器位置不同，请勿照搬，必须自行查阅 Tesla 官方服务手册。**
>
> - **Model 3 / Model Y（2021 及以后）**：推荐接 <a href="https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/" target="_blank" rel="noopener">X179 连接器</a>，Pin 13（CAN-H）/ Pin 14（CAN-L）
> - **Model 3（2020 及以前旧款）**：推荐接 <a href="https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/" target="_blank" rel="noopener">X652 连接器</a>，Pin 1（CAN-H）/ Pin 2（CAN-L）
>
> 不确定时请先查阅 Tesla 服务手册，切勿盲目拆车接线。

---

### 方案一：标准 ESP32 + SN65HVD230

**Model 3 / Model Y 2021 及以后（X179）：**
![接线图 X179](images/wiring.svg)

**Model 3 2020 及以前（X652）：**
![接线图 X652](images/wiring_x652.svg)

#### ESP32 ↔ SN65HVD230 模块

```
ESP32 GPIO 5  →  SN65HVD230 TX（有的板子标 CTX/D）
ESP32 GPIO 4  →  SN65HVD230 RX（有的板子标 CRX/R）
ESP32 3.3V    →  SN65HVD230 VCC
ESP32 GND     →  SN65HVD230 GND
```

#### SN65HVD230 ↔ 车辆 CAN 总线

```
SN65HVD230 CANH  →  车辆 CAN-H（通常为白色/棕色线）
SN65HVD230 CANL  →  车辆 CAN-L（通常为蓝色/绿色线）
```

---

### 方案二：Waveshare ESP32-S3-RS485-CAN（推荐车内永久安装）

![接线图 Waveshare](images/wiring_waveshare.svg)

接线极为简单，只需 4 根线，全部拧螺丝端子：

```
车辆 12V（保险丝盒 ACC 路）→  板子左侧端子 VCC+
车辆 GND                   →  板子左侧端子 GND
车辆 CAN-H（X179 Pin 13）  →  板子右侧端子 CAN H
车辆 CAN-L（X179 Pin 14）  →  板子右侧端子 CAN L
```

> - **无需** SN65HVD230 模块，CAN 收发器已集成（TJA1051T）
> - **无需**处理终端电阻，120Ω 默认未接入
> - **无需** USB 常供电，可直接接车内 12V（ACC 路，随车熄火断电）

---

## 第一步：安装软件

![编译烧录步骤](images/compile-steps.svg)

### 1.1 安装 VS Code

1. 打开 [https://code.visualstudio.com](https://code.visualstudio.com)，点击下载并安装。
2. 安装完成后打开 VS Code。

### 1.2 安装 PlatformIO 插件

1. 点击 VS Code 左侧的「扩展」图标（四个方块的图标）。
2. 搜索 `PlatformIO IDE`，点击「安装」。
3. 安装完成后，VS Code 底部状态栏会出现一个小房子图标，表示 PlatformIO 已就绪（可能需要重启 VS Code）。

> **安装很慢？** PlatformIO 第一次启动会下载 ESP32 工具链，大约 200–500 MB，请耐心等待，保持网络畅通。

---

## 第二步：下载并打开项目

1. 点击本页面右上角的绿色「Code」按钮，选择「Download ZIP」。
2. 解压到任意文件夹，例如 `D:\tesla-fsd`。
3. 打开 VS Code，点击菜单「文件 → 打开文件夹」，选择刚才解压的文件夹。
4. VS Code 右下角可能会弹出「是否信任此文件夹」，点击「是，我信任作者」。

---

## 第三步：修改配置

> 烧录前**无需改任何代码**。WiFi 名称/密码和硬件版本均可在烧录后通过 Web 面板修改，设置自动保存。

### 3.1 修改 WiFi 热点名称和密码

**推荐方式（烧录后在 Web 面板修改）：**

1. 烧录完成后，用手机连上默认热点 `FSD-Controller`，密码 `12345678`。
2. 浏览器访问 `192.168.4.1`，在「**WiFi 设置**」卡片中填入新名称和新密码。
3. 点「保存并重启」，设备重启后用新名称/密码重连即可。

**也可以烧录前在代码中修改初始默认值：**

打开文件 `src/main.cpp`，找到第 20–21 行：

```cpp
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";
```

将引号内的内容改成你自己的名称和密码（密码至少 8 位），烧录后直接生效。

### 3.2 了解你的硬件版本

**不需要改代码。** 烧录完成后，在 Web 控制面板的「硬件版本」下拉菜单中选择即可，设置会自动保存，断电后也不丢失。

烧录前先确认你的版本，以便连上面板后能立刻选对：

| 车机显示 | 软件版本 | FSD 版本 | 控制面板选择 |
|----------|----------|----------|------|
| HW4.x | 2026.2.9.x 或更新 | V14 | `HW4` |
| HW4.x | 2026.8.x 或更旧 | V13 | `HW3` ⚠️ |
| HW3.x | 任意 | — | `HW3` |
| HW2.5 或更早 | 任意 | — | `LEGACY` |

> ⚠️ **HW4 硬件 + V13 固件必须选 `HW3`**：HW4 模式专为 FSD V14 设计，在 V13 固件上会导致功能不稳定（时好时坏）。在车机「控制 → 软件」中查看软件版本号来判断。

> **如何判断 HW2.5 还是 HW3？** 在车机「软件」→「额外车辆信息」中查看：有「全自动驾驶计算机」字样 → HW3；没有 → HW2.5，选 `LEGACY`。2019 年中后期起大多数车辆为 HW3，但同年也存在 HW2.5，需以车机实际显示为准。

---

## 第四步：烧录固件

### 连接 ESP32

用 USB 线将 ESP32 连接到电脑。

### 在 VS Code 中烧录

1. 点击 VS Code 底部状态栏左侧的「✓ Build」按钮先编译，确认没有错误。
2. 编译成功后，点击旁边的「→ Upload」按钮开始烧录。
3. 烧录成功后，终端会显示 `success`。

> **遇到「上传失败」？**
> - 检查 USB 线是否支持数据传输（有些线只能充电）。
> - 在 VS Code 底部状态栏点「端口」，确认已正确识别 ESP32 的 COM 口。
> - 部分 ESP32 开发板需要在上传时**按住 BOOT 按钮**，直到出现上传进度条后松开。

---

## 第五步：使用控制面板

### 5.1 连接 WiFi

1. 将 ESP32 上电（可以用充电宝供电，也可以直接从 USB 供电）。
2. 用手机打开 WiFi 设置，找到 `FSD-Controller`（或你修改的名字），输入密码连接。
3. 连接后，手机浏览器访问 `192.168.4.1`。

### 5.2 控制面板说明

| 设置项 | 说明 |
|--------|------|
| **FSD 开关** | 总开关，关闭后设备不修改任何 CAN 报文 |
| **硬件版本** | 对应你的车辆硬件，与第 3.2 步一致（可运行时切换，无需重新烧录）|
| **速度模式** | 控制 FSD 行驶激进程度，见下方速度模式说明 |
| **模式来源** | 「自动（拨杆）」= 用跟车距离拨杆控制速度模式；「手动」= 用上面的下拉菜单固定 |
| **限速提示音抑制** | 关闭 ISA 限速提示音（仅 HW4 有效）|
| **紧急车辆检测** | 启用紧急车辆靠近检测（仅 HW4 FSD V14 有效）|
| **强制激活** | 适用于「交通灯和停车标志控制」功能在车机 UI 中被隐藏的地区（中国、日本等）。这些地区的触发条件永远不满足，设备完全无反应。开启后强制绕过此判断，直接激活 FSD。 |
| **WiFi 设置** | 修改热点名称（SSID）和密码，点「保存并重启」后设备重启并以新名称/密码创建热点，重连后继续使用 |

### 5.3 速度模式对照表

| 设置值 | HW3 显示 | HW4 显示 |
|--------|---------|---------|
| 保守 | ❄️ Chill | ❄️ Chill |
| 默认 | 🟢 Normal | 🟢 Normal |
| 适中 | ⚡ Hurry | ⚡ Hurry |
| 激进 | — | 🔥 Max（最快）|
| 最大 | — | 🐢 Sloth（最慢）|

> 跟车距离拨杆和速度模式的关系：拨杆数字越小 = 越激进；拨杆数字越大 = 越保守。

### 5.4 状态监控

控制面板下方的状态卡片实时显示：
- **已修改**：本次启动共修改了多少个 CAN 帧（大于 0 说明工作正常）
- **已接收**：收到的 CAN 帧总数
- **错误**：CAN 总线错误计数（正常应为 0）
- **运行时间**：ESP32 已运行时长
- **CAN 总线**：显示「正常」说明已成功接入车辆 CAN 总线
- **FSD 已触发**：显示「是」说明 FSD 已被激活

---

## 第六步：无线更新固件（OTA）

修改代码后不需要再插 USB，可以直接通过 WiFi 更新：

1. 在 VS Code 中修改代码并编译（点「Build」）。
2. 编译完成后，项目根目录会自动生成 **`firmware_esp32_ota.bin`**（Waveshare 则为 `firmware_esp32s3_waveshare_ota.bin`）。
3. 在控制面板底部找到「固件更新」卡片，点「选择文件」，选择对应的 **`_ota.bin`** 文件。
4. 点「上传固件」，等待进度条完成，设备会自动重启。

> ⚠️ **OTA 必须使用 `_ota.bin` 文件**，不能用合并固件（`firmware_esp32.bin`）。合并固件包含 bootloader，OTA 上传器会报 "Wrong Magic Byte" 错误。

---

## 常见问题

**Q：连上 WiFi 后浏览器打不开 192.168.4.1？**
> 部分手机在连接无网络的 WiFi 时会自动断开，在手机 WiFi 设置中找到该热点，关闭「自动切换更优网络」或「智能 WiFi」选项。

**Q：CAN 总线显示「异常」？**
> 检查 CANH/CANL 接线是否正确，确认车辆已上电（至少开到「待机」状态）。CAN 总线只有在车辆通电后才有信号。

**Q：FSD 已触发显示「否」？**
> 确认车机「控制 → 自动辅助驾驶」中已开启「交通灯和停车标志控制」，这是 FSD 触发的条件。

**Q：需要拆掉 CAN 模块上的 120Ω 终端电阻吗？**
> Tesla CAN 总线两端已有内置终端电阻，模块上如果再叠加一个会导致信号失真和通信失败，**必须确保模块的 120Ω 电阻断开**。
> - **SN65HVD230 模块**：芯片本身不带终端电阻，但部分厂商的模块会在 PCB 上额外加一颗，通过跳线或焊桥控制。收到模块后翻到背面检查 **120R** 标注旁的跳线帽或焊桥，如果已连通需要断开（拔掉跳线帽，或用刀片划断焊桥）。
> - **MCP2515 蓝色模块**：**必须拆掉**板上的 R1/R2 电阻或切断 J1 跳线。

**Q：「已修改」帧数一直是 0，设备没有反应？（中国、日本等地区）**
> 开启控制面板中的「**强制激活**」。中国、日本等地区的车机 UI 中没有「交通灯和停车标志控制」选项，该功能被区域限制隐藏，导致设备的触发条件（`UI_fsdStopsControlEnabled` bit）永远为 0，mod 永远不触发。开启强制激活后会绕过此判断，直接激活 FSD。

**Q：设备工作了但 FSD 还是用不了？**
> 检查 Tesla 账号是否有有效的 FSD 授权（购买或订阅）。设备只处理 CAN 层面，不能绕过 Tesla 服务器的授权验证。

**Q：速度模式设置后没有效果？**
> 确认「模式来源」设置为「手动」，否则会被拨杆位置覆盖。

**Q：烧录时报错「A fatal error occurred: Failed to connect to ESP32」？**
> 烧录时按住开发板上的「BOOT」按钮，看到上传进度条出现后松开。

---

## 项目结构（供开发者参考）

```
src/
  main.cpp              ← 入口：WiFi AP + Web 服务器 + CAN 任务（双核）
include/
  handlers.h            ← CAN 报文处理（Legacy / HW3 / HW4）
  can_helpers.h         ← 位操作工具函数
  can_frame_types.h     ← CAN 帧数据结构
  web_ui.h              ← 内嵌 Web 界面 HTML
  drivers/
    can_driver.h        ← 驱动抽象基类
    twai_driver.h       ← ESP32 TWAI 驱动
platformio.ini          ← 构建配置（ESP32 + ESPAsyncWebServer）
```

---

## 💰 支持项目

如果这个项目对你有帮助，欢迎打赏支持，让开发工作持续更新！

| 微信打赏 | 支付宝打赏 |
|---------|---------|
| ![微信打赏码](https://raw.githubusercontent.com/wjsall/teslamate-chinese-dashboards/main/images/wechat-donate.jpg) | ![支付宝打赏码](https://raw.githubusercontent.com/wjsall/teslamate-chinese-dashboards/main/images/alipay-donate.jpg) |

**您的支持是我持续更新的动力！** ❤️

---

## Credits

本项目基于以下开源项目开发，核心 CAN 报文处理逻辑直接来源于原项目：

| 项目 | 链接 | 许可证 |
|------|------|--------|
| tesla-open-can-mod | [gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) | GPLv3 |

---

## 许可证

GPLv3 — 基于 [tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod)（GPLv3）开发。
