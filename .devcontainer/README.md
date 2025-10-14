# BLE Foo DevContainer

这个 devcontainer 配置为 BLE Foo 项目提供了完整的开发环境，基于 `armforge` Docker image 并添加了开发工具。

## 功能特性

- 基于 `armforge` image (ROS2 Humble)
- 支持 Linux ARM64 平台
- 包含蓝牙开发所需的库和工具
- 预配置的 C++ 开发环境
- 代码质量工具（clang-format, cpplint）
- VS Code 扩展和设置
- 开发工具（Git, CMake, Make 等）

## 使用方法

1. 在 VS Code 中打开项目
2. 按 `Ctrl+Shift+P` (或 `Cmd+Shift+P` on macOS)
3. 选择 "Dev Containers: Reopen in Container"
4. 等待容器构建完成

## 构建项目

在容器内，你可以使用以下命令构建项目：

```bash
# 使用 CMake
cmake -S . -B build
cmake --build build

# 或者使用 Makefile
make build
```

## 开发工具

容器内预装了以下开发工具：
- CMake
- Make
- clang-format
- cpplint
- Git
- Python 3 和 pip

## 蓝牙权限

容器以特权模式运行，并挂载了必要的设备文件，以支持蓝牙开发。
