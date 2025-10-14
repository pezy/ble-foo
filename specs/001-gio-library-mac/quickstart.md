# Quick Start Guide: Bluetooth Device Discovery

**Version**: 1.0.0
**Date**: 2025-10-14

## 环境要求

### 系统依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    libglib2.0-dev \
    libdbus-1-dev \
    pkg-config \
    build-essential

# CentOS/RHEL
sudo yum install -y \
    glib2-devel \
    dbus-devel \
    pkgconfig \
    gcc-c++
```

### 蓝牙服务

```bash
# 确保蓝牙服务运行
sudo systemctl start bluetooth
sudo systemctl enable bluetooth

# 检查蓝牙状态
bluetoothctl show
```

## 编译和构建

### 使用 Makefile

```bash
# 克隆项目
git clone <repository-url>
cd ble-foo

# 使用 CMake 构建库和 CLI 工具
make build

# 交叉编译到 ARM64
make docker

# 在 Docker 容器中构建
make sh
```

### 手动编译 (开发环境)

```bash
# 创建构建目录
mkdir build && cd build

# 配置依赖
pkg-config --cflags --libs gio-2.0

# 编译库
g++ -std=c++17 -fPIC \
    -I../src/include \
    $(pkg-config --cflags gio-2.0) \
    -c ../src/lib/bluetooth/device_discovery.cpp \
    -o device_discovery.o

# 创建共享库
g++ -shared -o libble.so device_discovery.o

# 编译 CLI 工具
g++ -std=c++17 \
    -I../src/include \
    $(pkg-config --cflags gio-2.0) \
    -L. -lble \
    ../src/cli/ble_paired.cpp \
    -o ble_paired
```

## 使用方法

### CLI 工具

```bash
# 基本查询
./ble_paired

# 自定义超时
./ble_paired --timeout 3000

# 详细输出
./ble_paired --verbose
```

### C++ 库集成

```cpp
#include <bluetooth/device_discovery.hpp>
#include <iostream>

int main() {
    try {
        // 函数式接口调用
        auto result = bluetooth::get_paired_devices();

        if (result.success) {
            std::cout << "Found " << result.deviceCount() << " devices\n";
            for (const auto& device : result.devices) {
                std::cout << device.mac_address << std::endl;
            }
        } else {
            std::cerr << "Error: " << result.error_message << std::endl;
        }
    } catch (const bluetooth::BluetoothException& e) {
        std::cerr << "Bluetooth error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

### 编译示例程序

```bash
g++ -std=c++17 \
    -Isrc/include \
    $(pkg-config --cflags gio-2.0) \
    -Lbuild -lble \
    example.cpp \
    -o example \
    $(pkg-config --libs gio-2.0)
```

## 常见问题

### 权限问题

```bash
# 确保用户在 bluetooth 组中
sudo usermod -a -G bluetooth $USER
# 重新登录生效

# 或者使用 sudo 运行
sudo ./ble_paired
```

### D-Bus 连接问题

```bash
# 检查 D-Bus 会话
echo $DBUS_SESSION_BUS_ADDRESS

# 手动设置会话 (如果需要)
export $(dbus-launch)
```

### 蓝牙服务问题

```bash
# 重启蓝牙服务
sudo systemctl restart bluetooth

# 检查 BlueZ 版本
bluetoothctl --version
```

## 性能优化

### 批量查询

```cpp
// 使用超时控制
auto result = bluetooth::get_paired_devices_with_timeout(std::chrono::milliseconds(1000));

// 检查查询时间
std::cout << "Query took " << result.query_time.count() << "ms\n";
```

### 错误处理

```cpp
auto result = bluetooth::get_paired_devices();

switch (static_cast<bluetooth::ErrorCode>(result.error_code)) {
    case bluetooth::ErrorCode::Success:
        // 处理成功结果
        break;
    case bluetooth::ErrorCode::BluetoothServiceUnavailable:
        // 提示用户检查蓝牙服务
        break;
    case bluetooth::ErrorCode::PermissionDenied:
        // 提示权限问题
        break;
    default:
        // 通用错误处理
        break;
}
```

## 故障排除

### 调试模式

```bash
# 启用详细日志
./ble_paired --verbose

# 检查系统日志
journalctl -u bluetooth -f
```

### 依赖检查

```bash
# 检查 GLib 版本
pkg-config --modversion gio-2.0

# 检查 BlueZ
pkg-config --modversion bluez

# 检查编译器支持
g++ --version | grep -E "g\+\+|clang"
```

## 下一步

- 查看完整的 [API 文档](contracts/api-contract.md)
- 了解 [数据模型](data-model.md)
- 阅读 [研究文档](research.md)
- 运行测试用例验证功能
