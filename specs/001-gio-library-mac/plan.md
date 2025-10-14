# Implementation Plan: Bluetooth Paired Device Discovery

**Branch**: `001-gio-library-mac` | **Date**: 2025-10-14 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-gio-library-mac/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

基于 GIO library 实现蓝牙已配对设备查询功能，提供 C++17 函数式库接口和简洁的 CLI 工具，专注于获取已配对设备的 MAC 地址列表。采用 CMake 构建系统，支持 Linux ARM64 平台交叉编译。

## Technical Context

**Language/Version**: C++17
**Primary Dependencies**: GIO library (GLib), BlueZ D-Bus API
**Storage**: N/A (runtime query only)
**Testing**: Google Test, Google Mock
**Target Platform**: Linux (ARM64)
**Project Type**: Single project (library + CLI)
**Performance Goals**: <2s query time, <100ms function call overhead
**Constraints**: 简洁性优先，仅文本输出格式，函数式接口设计
**Scale/Scope**: Support up to 50 paired devices, minimal memory footprint

## Constitution Check

_GATE: Must pass before Phase 0 research. Re-check after Phase 1 design._

### Phase 0 Research Gates - ✅ PASSED

- ✅ **库优先原则**: 研究确认 GIO library 支持独立库实现
- ✅ **CLI 接口原则**: 确认可通过 CLI 访问库功能，支持文本输出格式
- ✅ **测试优先原则**: 研究确定测试策略和工具链
- ✅ **集成测试原则**: 明确 D-Bus 和 BlueZ 集成测试方法
- ✅ **平台特定原则**: 确认 Linux ARM64 平台兼容性和交叉编译方案

### Phase 1 Design Gates - ✅ PASSED

- ✅ **库优先原则**: 设计独立 `libble` 库，函数式接口，自包含且可测试
- ✅ **CLI 接口原则**: `ble_paired` CLI 工具通过库函数实现，支持文本输出格式
- ✅ **测试优先原则**: 设计单元测试、集成测试、契约测试结构
- ✅ **集成测试原则**: 定义 D-Bus 连接和 BlueZ API 集成测试
- ✅ **平台特定原则**: 使用 CMake + Makefile 交叉编译到 ARM64，依赖管理明确

## Project Structure

### Documentation (this feature)

```
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```
src/
├── include/
│   └── bluetooth/
│       └── device_discovery.hpp
├── lib/
│   └── bluetooth/
│       └── device_discovery.cpp
└── cli/
    └── ble_paired.cpp

tests/
├── unit/
│   └── bluetooth/
│       └── test_device_discovery.cpp
├── integration/
│   └── test_bluetooth_integration.cpp
└── contract/
    └── test_api_contract.cpp

CMakeLists.txt
Makefile
README.md
```

**Structure Decision**: 单项目结构，包含蓝牙设备查询库和 CLI 工具，遵循库优先原则和简洁性设计

## Complexity Tracking

_Fill ONLY if Constitution Check has violations that must be justified_

**无复杂性违规**：所有设计选择都符合宪法原则，没有需要额外复杂性的情况。

### 简洁性设计选择

| 设计选择       | 简洁性理由                       | 替代方案被拒绝的原因          |
| -------------- | -------------------------------- | ----------------------------- |
| 函数式接口     | 避免面向对象复杂性，直接暴露功能 | 类接口增加不必要的抽象层      |
| 仅文本输出     | 避免序列化复杂性，专注核心功能   | JSON 格式增加依赖和格式化代码 |
| CMake+Makefile | 利用现有工具链，最小化配置复杂度 | 纯 CMake 需要重构现有流程     |
