# Research: GIO Library Bluetooth Integration

**Date**: 2025-10-14
**Feature**: Bluetooth Paired Device Discovery
**Platform**: Linux ARM64

## 技术选型分析

### 1. GIO Library 与 BlueZ 集成

**Decision**: 使用 GIO library 通过 D-Bus 接口访问 BlueZ 服务

**Rationale**:

- GIO 是 GLib 的一部分，提供现代化的异步 I/O 和 D-Bus 集成
- BlueZ (Linux 蓝牙协议栈) 通过 D-Bus 提供设备管理 API
- 在 Linux ARM64 平台上广泛可用，稳定性高

**Key APIs**:

- `GDBusConnection`: D-Bus 连接管理
- `GDBusProxy`: BlueZ 对象代理
- `org.bluez.Device1`: 设备接口，包含 Address 属性 (MAC 地址)

### 2. C++17 现代化实现

**Decision**: 使用 C++17 特性包装 GIO API

**Rationale**:

- RAII 模式管理 GIO 对象生命周期
- 智能指针避免内存泄漏
- 异常安全保证
- 符合项目 C++17 要求

**Implementation Pattern**:

```cpp
// RAII 包装器
class BluetoothDeviceDiscovery {
    std::unique_ptr<GDBusConnection, decltype(&g_object_unref)> connection_;
    // ...
};
```

### 3. 依赖管理

**Required Dependencies**:

- `libglib2.0-dev`: GLib 开发包 (包含 GIO)
- `pkg-config`: 依赖配置工具

**Simplification Note**: 移除 JSON 库依赖，专注纯文本输出

**Cross-compilation for ARM64**:

```bash
# Docker 环境中的交叉编译配置
pkg-config --define-prefix --static --libs gio-2.0
```

### 4. 错误处理策略

**Decision**: 使用错误码 + 异常混合模式

**Rationale**:

- GIO 使用 GError，需要转换为 C++ 异常
- 提供清晰的错误信息给 CLI 用户
- 库接口保持异常安全

**Error Categories**:

- 蓝牙服务不可用
- 权限不足
- 设备查询超时
- D-Bus 连接失败

### 5. 性能优化

**目标**: <2 秒查询时间，<100ms 函数调用开销

**Optimizations**:

- D-Bus 连接复用
- 异步查询避免阻塞
- 结果缓存机制
- 超时控制 (默认 1.5 秒)

## 架构设计

### 数据流

```
CLI Program ──┐
             ├──> C++ Library ──> GIO ──> D-Bus ──> BlueZ ──> Device List
API Caller ──┘
```

### 关键组件

1. **DeviceDiscovery**: 核心查询类
2. **BluetoothDevice**: 设备信息封装
3. **ResultFormatter**: JSON/文本输出格式化
4. **ErrorHandler**: 错误处理和日志

## 替代方案考虑

### 1. 直接使用 BlueZ D-Bus API

**Rejected**: 需要手动处理 D-Bus 协议，开发复杂度高

### 2. 使用 Qt Bluetooth

**Rejected**: 引入重依赖，不符合项目轻量化要求

### 3. 系统 CLI 工具包装

**Rejected**: 解析输出不稳定，错误处理困难

## 风险评估

### 高风险

- **权限依赖**: 需要 D-Bus 访问权限
- **BlueZ 版本**: 不同 Linux 发行版版本差异

### 中风险

- **异步处理**: GIO 异步模式在 C++ 中集成复杂度

### 缓解措施

- 提供详细的权限检查和错误提示
- 支持 BlueZ 5.x 版本兼容性检查
- 封装异步操作为同步接口，简化使用
