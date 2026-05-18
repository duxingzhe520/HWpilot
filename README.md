# HWpilot

HWpilot 是一个面向程设课程的桌面辅助工具，集成了作业项目管理、文件扫描、版本节点记录与 Deepseek AI 分析能力。

## 已实现功能

- 项目文件扫描：递归读取目标目录中常见的代码与文档文件（如 `cpp`、`h`、`py`、`md` 等）。
- 版本节点管理：支持手动提交版本节点，并在版本树中显示提交记录与 AI 反馈。
- AI 分析模块：支持“代码批改”、“Bug 检查”、“启发式引导”、“学习模式”、“复习总结”等多种模式。
- AI 反馈保存：将当前 AI 分析结果保存到选中的版本节点，并在右侧展示。
- 完整 Qt 桌面界面：包含项目树、版本树、文件列表、变更视图、历史与复习报告面板。

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

1. 点击“打开项目”选择一个作业目录。
2. 点击“扫描文件”加载项目中的代码与文档文件。
3. 勾选想要参与 AI 分析的文件。
4. 在右侧选择 AI 模式，填写作业要求或问题。
5. 点击“开始分析”，等待 AI 回复。
6. 若满意，将 AI 反馈保存到当前版本节点。
7. 点击“提交版本”创建一个新的版本节点。

## 提示

- 当前 AI 模块使用 Deepseek 兼容接口，需通过 `DEEPSEEK_API_KEY` 提供 API Key。
- 版本树目前为手动记录型节点示意，并未接入完整 git 历史回滚功能。
- `HWFileScanner` 会跳过隐藏目录和单文件超过 200KB 的文件。

## 目录结构

- `main.cpp`：应用入口。
- `MainWindow.cpp/.h`：主界面与交互逻辑。
- `HWFileScanner/`：项目文件扫描与 LLM 内容格式化。
- `HWpilotLLM/`：Deepseek 聊天请求封装与 Prompts 定义。
- `CMakeLists.txt`：项目构建配置。
