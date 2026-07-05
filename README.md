# HWpilot

HWpilot 是一个基于 Qt 6 的本地桌面工具，用于扫描程序设计作业项目、调用 DeepSeek 生成 AI 反馈、保存反馈记录，并结合 Git 快照追踪作业修改过程。

## 验收前必须配置 DeepSeek API Key

AI 反馈、反馈复查和启发式问题功能都需要 DeepSeek API Key。真实 Key 不要提交到 GitHub。

配置步骤：

1. 编译并启动 HWpilot。
2. 点击“打开文件夹”，选择一个需要分析的作业项目目录。
3. 首次打开该目录时，程序会弹出“填写 API Key”窗口。
4. 粘贴 DeepSeek API Key，点击“确认”。
5. 程序会把 Key 保存到被分析作业目录的 `.hwpilot/project.json` 中。
6. 如果跳过了弹窗，可通过菜单“设置 -> 设置 API-key”重新填写。

注意：当前程序不依赖根目录的 `api-key.txt`。不要把包含真实 Key 的文件提交到 GitHub。

## 环境要求

- CMake 3.20 或更高版本
- 支持 C++17 的编译器
- Qt 6，至少包含 `Widgets` 和 `Network` 组件
- Git
- 可用的 DeepSeek API Key

## 构建与运行

```bash
cmake -S . -B build
cmake --build build
./build/HWpilot
```

## 基本使用流程

1. 打开 HWpilot。
2. 点击“打开文件夹”，选择要分析的作业项目。
3. 填写或确认 DeepSeek API Key。
4. 在“AI反馈”页勾选需要分析的文件。
5. 填写作业要求和补充问题。
6. 点击“生成 AI 反馈”。
7. 查看结果后，点击“保存为反馈记录”。
8. 在“反馈记录”页查看历史反馈，并可将条目标记为未解决、已解决、无法确定或忽略。
9. 修改代码后，可点击“提交”保存 Git 快照。
10. 如需复查历史问题，可在“AI反馈”页切换到“勾选反馈记录”，选择历史反馈后点击“复查已选反馈”。

## 主要功能

- 递归扫描作业目录中的常见代码和文本文件。
- 跳过隐藏目录、隐藏文件和过大的单文件。
- 调用 DeepSeek `deepseek-chat` 生成结构化 AI 反馈。
- 保存反馈记录到作业目录下的 `.hwpilot/feedbacks.json`。
- 支持反馈条目状态管理和历史反馈复查。
- 支持启发式问题生成，帮助学生继续思考。
- 自动初始化 Git 仓库，并支持提交当前作业快照。
- 展示当前工作区或历史 commit 的变更统计与可读 diff。
- 支持中文/英文界面和模型温度设置。

## 本地数据说明

打开某个作业目录后，HWpilot 会在该目录下创建 `.hwpilot` 文件夹：

- `.hwpilot/project.json`：保存项目路径、作业要求、最近打开时间和 API Key。
- `.hwpilot/feedbacks.json`：保存 AI 反馈、复查记录和启发式问题记录。

这些文件包含本地配置和可能的敏感信息，不建议提交到公开仓库。
