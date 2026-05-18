# HWpilot

HWpilot 是一个面向单次程设作业的 AI 代码分析工具。它负责扫描作业文件、整理当前代码变更、调用 DeepSeek 生成 AI 分析结果，并将反馈保存到本地记录中。

## 已实现功能

- 项目文件扫描：递归读取目标目录中常见的代码与文档文件（如 `cpp`、`h`、`py`、`md` 等）。
- Git 变更查看：显示当前项目的 `git status`、`git diff --stat` 和 `git diff`，作为 AI 分析的辅助上下文。
- AI 分析：将作业要求、补充问题、勾选文件内容和当前 Git 变更发送给 DeepSeek。
- AI 反馈保存：将分析结果保存到 `.hwpilot/feedbacks.json`，并在界面中展示历史反馈。
- 保存与提交：保存 AI 反馈后，可在同一入口提交当前项目状态作为代码存档。
- Qt 桌面界面：包含当前项目、提交存档、文件列表、变更视图和 AI 反馈记录。

## 运行要求

- Qt 6 及其开发组件（Widgets、Network）
- CMake 3.16+
- 已设置环境变量 `DEEPSEEK_API_KEY`

## 构建与运行

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./HWpilot
```

如果你使用的是 Qt 5，请自行修改 `CMakeLists.txt` 中的 `find_package(Qt6 ...)` 为 `find_package(Qt5 ...)` 并调整链接库。

## 使用说明

1. 点击“打开项目并扫描”选择一个作业目录。
2. 程序会自动加载项目中的代码与文档文件。
3. 勾选想要参与 AI 分析的文件。
4. 在右侧填写作业要求或补充问题。
5. 点击“AI 分析”，等待 AI 回复。
6. 若满意，点击“保存反馈并提交存档”。

## 提示

- 当前 AI 模块使用 Deepseek 兼容接口，需通过 `DEEPSEEK_API_KEY` 提供 API Key。
- Git 仅用于辅助展示变更和提交存档，不提供分支管理、历史恢复等完整版本控制功能。
- `HWFileScanner` 会跳过隐藏目录和单文件超过 200KB 的文件。

## 目录结构

- `main.cpp`：应用入口。
- `MainWindow.cpp/.h`：主界面与交互逻辑。
- `ProjectManager/`：项目元数据读写。
- `FeedbackStore/`：AI 反馈记录读写。
- `GitService/`：Git 状态、diff、log 与提交封装。
- `HWFileScanner/`：项目文件扫描与 LLM 内容格式化。
- `HWpilotLLM/`：Deepseek 聊天请求封装与 Prompts 定义。
- `CMakeLists.txt`：项目构建配置。
