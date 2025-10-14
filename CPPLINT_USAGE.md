# cpplint 集成使用说明

## 概述

本项目已正确集成 cpplint 代码风格检查工具，用于确保代码质量和一致性。

## 使用方法

### 1. 运行代码风格检查

```bash
# 在项目根目录下
make b          # 正常构建
make -C build cpplint  # 运行代码风格检查
```

### 2. 自动修复格式问题

```bash
# 生成修复命令（不直接应用）
make -C build cpplint-fix

# 应用自动修复（需要手动执行）
make -C build cpplint-fix | sh
```

### 3. 直接使用 cpplint

```bash
# 检查整个 src 目录
cpplint --recursive src/

# 检查特定文件
cpplint src/cli/ble_paired.cpp
```

## 配置说明

### CPPLINT.cfg 配置文件

项目根目录的 `CPPLINT.cfg` 文件包含以下配置：

- `set noparent`: 防止向上查找父目录配置
- `filter`: 过滤规则，忽略不重要的警告
- `linelength=120`: 行长度限制为120字符
- `root=src`: 设置项目根目录为 src
- `headers`: 头文件扩展名配置

### 过滤的警告类型

- `whitespace/line_length`: 行长度警告
- `build/include_order`: 头文件包含顺序
- `legal/copyright`: 版权声明
- `build/c++11`: C++11 头文件警告

## 集成方式

### CMake 集成

使用自定义目标方式集成，不会影响正常构建：

```cmake
# 创建 cpplint 目标
add_custom_target(cpplint
    COMMAND ${CPPLINT} --recursive ${CMAKE_SOURCE_DIR}/src
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cpplint on source files"
)

# 创建 cpplint-fix 目标
add_custom_target(cpplint-fix
    COMMAND ${CPPLINT} --recursive --output=sed ${CMAKE_SOURCE_DIR}/src
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating cpplint fixes"
)
```

## 优势

1. **不影响构建**: 使用自定义目标，不会在正常编译时运行
2. **灵活配置**: 通过 CPPLINT.cfg 文件集中管理配置
3. **自动修复**: 支持生成和应用自动修复命令
4. **递归检查**: 自动检查整个 src 目录
5. **过滤噪音**: 过滤掉不重要的警告，专注于真正的问题

## 注意事项

- cpplint 是可选的，如果未安装会自动禁用
- 修复命令需要手动确认后再应用
- 配置文件会覆盖命令行参数
- 建议在提交代码前运行 cpplint 检查
