# API Contract: Bluetooth Device Discovery Library

**Version**: 1.0.0
**Date**: 2025-10-14

## Library API Contract

### 核心接口

#### 1. 函数式接口

```cpp
namespace bluetooth {
// 核心查询接口
DeviceQueryResult get_paired_devices();
}
```

**实现架构**: 资源管理使用 RAII 类（内部实现），核心逻辑通过函数暴露

#### 2. 数据结构

```cpp
struct PairedBluetoothDevice {
    std::string mac_address;
    std::optional<std::string> device_name;
    std::optional<uint32_t> device_class;
    std::optional<int16_t> rssi;
    bool connected;

    // 验证 MAC 地址格式
    bool isValidMacAddress() const;
};

struct DeviceQueryResult {
    std::vector<PairedBluetoothDevice> devices;
    bool success;
    int error_code;
    std::string error_message;
    std::chrono::milliseconds query_time;

    // 便利方法
    bool hasError() const;
    size_t deviceCount() const;
};
```

### 错误处理

#### 错误代码枚举

```cpp
enum class ErrorCode {
    Success = 0,
    BluetoothServiceUnavailable = 1,
    PermissionDenied = 2,
    QueryTimeout = 3,
    DBusConnectionFailed = 4,
    UnknownError = 5
};
```

#### 异常类型

```cpp
class BluetoothException : public std::runtime_error {
public:
    BluetoothException(ErrorCode code, const std::string& message);
    ErrorCode errorCode() const;
};
```

## CLI 接口规范

### 命令行格式

```bash
ble_pair [OPTIONS]
```

### 选项参数

- `--timeout, -t`: 查询超时时间 (毫秒)，默认: 1500
- `--verbose, -v`: 详细输出，包含错误详情
- `--help, -h`: 显示帮助信息

### 输出格式

#### 文本格式 (唯一格式)

```
AA:BB:CC:DD:EE:FF
11:22:33:44:55:66
22:33:44:55:66:77
```

#### 错误输出

```bash
Error: Bluetooth service is not available (code: 1)
```

## 性能要求

### 响应时间

- 库函数调用: < 100ms (不包括系统响应时间)
- CLI 程序总执行时间: < 2 秒
- 超时处理: 默认 1.5 秒，可配置

### 资源使用

- 内存占用: < 10MB (不包括 GIO 库)
- CPU 使用: 查询期间 < 5% 单核使用率

## 兼容性要求

### 平台支持

- Linux 发行版: Ubuntu 18.04+, CentOS 7+, Debian 9+
- 架构: ARM64
- BlueZ 版本: 5.x

### 依赖库版本

- GLib: >= 2.50
- BlueZ: >= 5.40
- C++ 编译器: 支持 C++17 标准

## 测试契约

### 单元测试覆盖

- MAC 地址格式验证
- 错误代码映射
- 数据结构序列化/反序列化

### 集成测试场景

- 正常设备查询流程
- 蓝牙服务不可用情况
- 权限不足处理
- 超时机制验证

### 性能测试

- 大量设备 (50+) 的查询性能
- 内存泄漏检测
- 并发调用安全性
