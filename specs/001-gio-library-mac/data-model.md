# Data Model: Bluetooth Paired Device Discovery

**Date**: 2025-10-14
**Feature**: Bluetooth Paired Device Discovery

## 核心实体

### 1. PairedBluetoothDevice

表示已配对的蓝牙设备信息。

**属性**:

- `mac_address`: string - MAC 地址 (格式: XX:XX:XX:XX:XX:XX)
- `device_name`: string (可选) - 设备名称
- `device_class`: uint32_t (可选) - 设备类别
- `rssi`: int16_t (可选) - 信号强度
- `connected`: bool - 连接状态

**验证规则**:

- MAC 地址格式验证 (正则: ^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$)
- 必填字段: mac_address
- 可选字段默认值处理

**状态转换**:

```
Unknown -> Validating -> Valid/Invalid
```

### 2. DeviceQueryResult

查询操作的返回结果。

**属性**:

- `devices`: vector<PairedBluetoothDevice> - 设备列表
- `success`: bool - 查询是否成功
- `error_code`: int - 错误代码 (0 表示成功)
- `error_message`: string - 错误描述
- `query_time_ms`: int64 - 查询耗时 (毫秒)

**错误代码定义**:

- 0: 成功
- 1: 蓝牙服务不可用
- 2: 权限不足
- 3: 查询超时
- 4: D-Bus 连接失败
- 5: 未知错误

## 数据流设计

### 查询流程

```
1. 初始化 D-Bus 连接
2. 获取 BlueZ 适配器
3. 查询已配对设备列表
4. 提取 MAC 地址和属性
5. 验证数据格式
6. 构建 DeviceQueryResult
7. 返回结果
```

### 错误处理

- 每个步骤都有明确的错误处理
- 错误信息包含上下文和恢复建议
- 支持错误码到用户友好消息的映射

## 性能考虑

### 内存管理

- 使用 vector 的 reserve() 预分配空间
- RAII 模式管理 GIO 对象
- 避免不必要的字符串拷贝

### 时间复杂度

- 设备列表查询: O(n)，n 为已配对设备数量
- MAC 地址验证: O(1) 每个设备
- 总体复杂度: O(n)

## 扩展性设计

### 未来可能扩展

- 设备类型过滤
- 信号强度阈值
- 连接状态筛选
- 设备别名管理

### 接口兼容性

- 向后兼容的 API 设计
- 可选属性的渐进式添加
- 版本化错误处理
