<!-- 7aca152f-7313-4065-92b8-3d327b7d1a5a b0d9ecc0-e6c6-4f09-8e94-ca62e2f6daf9 -->
# 添加 ble_conn CLI 工具

## 概述

创建新的 CLI 工具 `ble_conn`，用于连接和断开已配对的蓝牙设备。

## 实施步骤

### 1. 扩展 libble 库

在 `src/include/bluetooth/device_discovery.hpp` 中添加：

- `ConnectResult` 结构体（类似 `PairResult`）
- `DisconnectResult` 结构体
- `ConnectDevice(mac_address, timeout)` 函数声明
- `DisconnectDevice(mac_address)` 函数声明
- 可能需要添加新的错误码（如 `ConnectionFailed`、`DisconnectFailed`）

在 `src/lib/bluetooth/device_discovery.cpp` 中实现：

- 使用 BlueZ D-Bus 接口的 `Connect()` 方法
- 使用 BlueZ D-Bus 接口的 `Disconnect()` 方法
- 遵循现有的 `PairDevice` 实现模式

### 2. 创建 CLI 工具

创建 `src/cli/ble_conn.cpp`：

- 必选参数：MAC 地址
- 可选参数：`-d`（断开连接）
- 不带 `-d`：调用 `ConnectDevice()`
- 带 `-d`：调用 `DisconnectDevice()`
- 错误处理和结果输出，参考 `ble_pair.cpp`

### 3. 更新构建配置

在 `CMakeLists.txt` 中：

- 添加 `ble_conn` 可执行文件
- 链接到 `ble` 库
- 添加到安装目标
- 更新 clang-format 源文件列表

## 关键文件

- `src/include/bluetooth/device_discovery.hpp` - 添加 API
- `src/lib/bluetooth/device_discovery.cpp` - 实现连接/断开
- `src/cli/ble_conn.cpp` - 新建 CLI
- `CMakeLists.txt` - 构建配置

### To-dos

- [ ] 在 device_discovery.hpp 中添加连接和断开的 API 声明
- [ ] 在 device_discovery.cpp 中实现 ConnectDevice 和 DisconnectDevice
- [ ] 创建 ble_conn.cpp CLI 工具
- [ ] 更新 CMakeLists.txt 构建配置