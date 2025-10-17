<!-- cc11710b-8297-4f65-b69d-d422d2ba5719 8c776dce-5230-4d5a-ac60-63b4865e540a -->
# BLE Pair 功能增强

## 1. libble 库增强

### 1.1 更新头文件 `src/include/bluetooth/device_discovery.hpp`

- 添加配对结果结构体 `PairResult`（包含 success, error_code, error_message）
- 添加配对函数声明：`PairResult PairDevice(const std::string& mac_address, int timeout_seconds = 30)`
- 在 `ErrorCode` 枚举中添加配对相关错误码：`PairingFailed`, `DeviceNotFound`, `PairingTimeout`

### 1.2 实现配对逻辑 `src/lib/bluetooth/device_discovery.cpp`

参考 BlueZ D-Bus API 文档和现有的 `GetPairedDevices()` 实现：

- 实现 `PairDevice()` 函数：
- 连接系统 D-Bus
- 通过 ObjectManager 获取所有设备对象
- 根据 MAC 地址找到对应设备的 D-Bus 对象路径（格式如 `/org/bluez/hci0/dev_XX_XX_XX_XX_XX_XX`）
- 创建设备的 D-Bus proxy（接口：`org.bluez.Device1`）
- 调用 `Pair()` 方法，设置 30 秒超时
- 处理错误和超时情况
- 返回 `PairResult` 结果

关键实现细节：

- MAC 地址格式转换：`AA:BB:CC:DD:EE:FF` -> `dev_AA_BB_CC_DD_EE_FF`
- 使用 `g_dbus_proxy_call_sync()` 调用 Pair 方法
- 使用现有的 RAII wrappers 管理 GLib 对象生命周期

## 2. CLI 工具改造 `src/cli/ble_pair.cpp`

### 2.1 修改参数解析

- 删除 `-v/--verbose` 参数
- 添加位置参数 `mac_address`（可选）
- 更新描述和帮助信息

### 2.2 修改主逻辑

**无参数模式（列出已配对设备）：**

- 调用 `GetPairedDevices()`
- 输出格式：每行一个设备，格式为 `MAC地址 设备名 RSSI连接状态`
- 设备名：无则显示 `N/A`
- RSSI：无则显示 `N/A`，有则显示 `<value>dBm`
- 连接状态：`Connected` 或 `Disconnected`

**带 MAC 地址参数（配对设备）：**

- 验证 MAC 地址格式
- 调用 `PairDevice(mac_address)`
- 输出配对进度和结果信息
- 根据返回的错误码设置退出码

### 2.3 删除旧的辅助函数

- 删除 `PrintDeviceInfo()` 函数（不再需要 verbose 模式）

### To-dos

- [ ] 更新 device_discovery.hpp，添加配对相关结构体、函数声明和错误码
- [ ] 在 device_discovery.cpp 中实现 PairDevice() 函数
- [ ] 修改 ble_pair.cpp，更新参数解析和主逻辑