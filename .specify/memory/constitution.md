<!--
Sync Impact Report:
Version change: null → 1.0.0
Modified principles: N/A (new constitution)
Added sections: Core Principles, Platform Constraints, Development Standards
Removed sections: N/A
Templates requiring updates: ✅ plan-template.md, ✅ spec-template.md, ✅ tasks-template.md
Follow-up TODOs: N/A
-->

# BLE-Foo Constitution

## Core Principles

### I. 库优先原则

所有功能必须作为独立库开始；库必须自包含、可独立测试、有文档；必须有明确目的 - 不允许仅用于组织的库

### II. CLI 接口原则

每个库必须通过 CLI 暴露功能；文本输入输出协议：stdin/args → stdout，错误 → stderr；支持 JSON 和人类可读格式

### III. 测试优先原则（不可妥协）

TDD 强制要求：先编写测试 → 用户批准 → 测试失败 → 然后实现；严格遵循红-绿-重构循环

### IV. 集成测试原则

需要集成测试的重点领域：新库契约测试、契约变更、服务间通信、共享模式

### V. 平台特定原则

仅支持 Linux ARM64 平台；使用标准交叉编译流程；所有生成产物必须能在目标 ARM64 环境运行

## Platform Constraints

### 技术栈要求

- **语言**: C++17
- **目标平台**: Linux (ARM64)
- **编译标准**: 遵循 Makefile 交叉编译流程
- **版本管理**: Semantic Versioning 2.0.0
- **代码规范**: Google C++ Style Guide

### 扩展性要求

- 必须支持方便扩展 CLI 工具
- 库设计应考虑模块化和可组合性

## Development Standards

### 质量标准

- 所有代码必须通过静态分析检查
- 编码风格严格遵循 Google C++ Style Guide
- 所有公共接口必须有完整文档
- 性能关键路径必须有基准测试

### 版本管理

- 严格遵循 Semantic Versioning 2.0.0
- MAJOR 版本：不兼容的 API 变更
- MINOR 版本：向后兼容的功能新增
- PATCH 版本：向后兼容的问题修正

## Governance

本宪法凌驾于所有其他实践之上；修订需要文档记录、批准、迁移计划

所有 PR/审查必须验证合规性；复杂性必须合理化；使用运行时开发指南文件

**版本**: 1.0.0 | **制定**: 2025-10-14 | **最后修订**: 2025-10-14
