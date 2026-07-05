#include "AppText.h"

#include <QHash>
#include <QSettings>

namespace {
struct Entry {
    const char* key;
    const char* zh;
    const char* en;
};

const Entry kEntries[] = {
    {"app.title", "HWpilot", "HWpilot"},
    {"menu.file", "文件", "File"},
    {"menu.openProject", "打开项目文件夹...", "Open Folder..."},
    {"menu.recentProjects", "最近打开", "Recent Projects"},
    {"menu.noRecentProjects", "暂无最近项目", "No recent projects"},
    {"menu.settings", "设置", "Settings"},
    {"menu.language", "语言", "Language"},
    {"menu.language.zh", "中文", "Chinese"},
    {"menu.language.en", "English", "English"},
    {"menu.theme", "主题", "Theme"},
    {"menu.theme.light", "浅色", "Light"},
    {"menu.theme.dark", "深色", "Dark"},
    {"menu.temperature", "大模型温度", "Model Temperature"},
    {"menu.temperature.strict", "严谨 0.2", "Strict 0.2"},
    {"menu.temperature.balanced", "平衡 0.3", "Balanced 0.3"},
    {"menu.temperature.creative", "发散 0.7", "Creative 0.7"},
    {"menu.apiKey", "设置 API-key", "Set API Key"},
    {"menu.about", "关于", "About"},
    {"menu.aboutHwPilot", "关于 HWpilot", "About HWpilot"},
    {"menu.rename", "重命名", "Rename"},
    {"about.body", "HWpilot\n版本：1.0.0\n\nHWpilot 是一个面向程序设计作业的本地辅助工具，用于管理项目版本、查看代码变更、生成 AI 反馈并追踪反馈状态。\n\n主要功能：\n- 项目文件扫描与 AI 反馈生成\n- Git 版本快照与变更查看\n- 反馈记录保存、复查与状态管理\n- 启发式问题生成\n\nAI 功能由用户配置的 DeepSeek API Key 提供。使用 AI 功能时，所选代码内容和问题描述会发送给模型服务，请勿提交敏感信息。\n\nCopyright © 2026 HWpilot.",
     "HWpilot\nVersion: 1.0.0\n\nHWpilot is a local assistant for programming assignments. It helps manage project versions, review code changes, generate AI feedback, and track feedback status.\n\nMain features:\n- Project file scanning and AI feedback generation\n- Git version snapshots and change review\n- Feedback record saving, review, and status tracking\n- Guiding question generation\n\nAI features are powered by the user-configured DeepSeek API Key. When using AI features, selected code content and problem descriptions are sent to the model service. Do not submit sensitive information.\n\nCopyright © 2026 HWpilot."},

    {"button.openFolder", "打开文件夹", "Open Folder"},
    {"button.refresh", "刷新", "Refresh"},
    {"button.commit", "提交", "Commit"},
    {"button.selectAll", "全选", "Select All"},
    {"button.chooseCodeFiles", "勾选代码文件", "Choose Code Files"},
    {"button.chooseFeedbackRecords", "勾选反馈记录", "Choose Feedback Records"},
    {"button.aiAssistant", "生成 AI 反馈", "Generate AI Feedback"},
    {"button.reviewSelectedFeedback", "复查已选反馈", "Review Selected Feedback"},
    {"button.openReviewRecord", "查看复查记录", "Open Review Record"},
    {"button.generateHeuristic", "生成启发问题", "Generate Guiding Questions"},
    {"button.saveFeedback", "保存为反馈记录", "Save as Feedback"},
    {"button.cancel", "取消", "Cancel"},

    {"label.noProject", "尚未打开项目", "No project opened"},
    {"label.chooseProjectStart", "请选择一个作业文件夹", "Choose an assignment folder to start"},
    {"label.filesCount", "文件：%1", "Files: %1"},
    {"label.feedbackCount", "反馈记录：%1", "Feedback records: %1"},
    {"label.commits", "所有版本", "Commits"},
    {"label.projectFiles", "项目文件", "Project Files"},
    {"label.feedbackRecords", "反馈记录", "Feedback Records"},
    {"label.feedbackIssue", "反馈问题", "Issue"},
    {"label.severity", "严重程度", "Severity"},
    {"label.location", "位置", "Location"},
    {"label.status", "状态", "Status"},
    {"label.summary", "摘要", "Summary"},
    {"label.attributes", "属性", "Attributes"},
    {"label.fullFeedback", "完整反馈", "Full Feedback"},
    {"label.fullSuggestion", "完整建议", "Full Suggestion"},
    {"label.versionOverview", "版本概览", "Overview"},
    {"label.fileAnalysis", "AI反馈", "AI Helper"},
    {"label.aiAnalysis", "问题描述", "Problem Description"},
    {"label.feedbackReview", "反馈复查", "Feedback Review"},
    {"label.heuristicQuestions", "启发式问题", "Guiding Questions"},

    {"placeholder.task", "请输入作业要求，或在左侧勾选说明文档...", "Enter assignment requirements, or select the requirement document on the left..."},
    {"placeholder.question", "补充说明的问题...", "Additional questions for AI..."},
    {"placeholder.aiReply", "AI 回复会显示在这里...", "AI replies will appear here..."},
    {"placeholder.heuristicReply", "启发式问题会显示在这里...", "Guiding questions will appear here..."},
    {"placeholder.cancelledReply", "已取消保存本次 AI 反馈记录。", "This AI reply was cancelled. New analysis replies will appear here."},
    {"placeholder.savedFeedback", "已保存为反馈记录！", "Saved as feedback. New analysis replies will appear here."},

    {"dialog.noProject.title", "尚未打开项目", "No Project Opened"},
    {"dialog.noProject.body", "请先打开一个作业项目文件夹。", "Open an assignment project folder first."},
    {"dialog.noFiles.title", "没有可分析的文件", "No Files to Analyze"},
    {"dialog.noFiles.body", "请先打开项目并勾选至少一个代码文件。", "Open a project and select at least one code file."},
    {"dialog.emptyFiles.title", "文件内容为空", "File Content Is Empty"},
    {"dialog.emptyFiles.body", "已勾选文件，但没有读取到任何文件内容。请重新扫描项目，或切换到其它提交后再试。", "Files are selected, but no readable content was found. Rescan the project or switch to another commit and try again."},
    {"dialog.noApiKey.title", "缺少 API Key", "Missing API Key"},
    {"dialog.noApiKey.body", "当前项目还没有填写 API Key。请重新打开项目并填写，或在 .hwpilot/project.json 中设置 apiKey 后再使用 AI 功能。", "This project does not have an API key yet. Reopen the project and enter one, or set apiKey in .hwpilot/project.json before using AI features."},
    {"dialog.apiKey.title", "填写 API Key", "Enter API Key"},
    {"dialog.apiKey.body", "当前项目还没有保存 API Key。API Key 会保存到当前项目的 .hwpilot/project.json。", "This project does not have a saved API key. The API key will be saved to this project's .hwpilot/project.json."},
    {"dialog.apiKey.label", "API Key：", "API Key:"},
    {"dialog.apiKey.confirm", "确认", "Confirm"},
    {"dialog.apiKey.skip", "暂不填写", "Not Now"},
    {"dialog.emptyContent.title", "内容为空", "Empty Content"},
    {"dialog.emptyContent.body", "请填写问题、作业要求，或勾选代码文件。", "Enter a question, assignment requirements, or select code files."},
    {"dialog.noChanges.title", "没有可提交的变更", "No Changes to Commit"},
    {"dialog.noChanges.body", "当前工作区没有 Git 可提交的变更。", "There are no Git changes to commit in the current workspace."},
    {"dialog.commitMessage.title", "Commit message", "Commit Message"},
    {"dialog.commitMessage.label", "请输入 commit message：", "Enter commit message:"},
    {"dialog.emptyCommit.title", "Commit message 为空", "Commit Message Is Empty"},
    {"dialog.emptyCommit.body", "请输入 commit message 后再提交。", "Enter a commit message before committing."},
    {"dialog.gitAddFailed", "Git add 失败", "Git add failed"},
    {"dialog.gitCommitFailed", "Git commit 失败", "Git commit failed"},
    {"dialog.saveFailed", "保存失败", "Save Failed"},
    {"dialog.noFeedback.title", "没有反馈", "No Feedback"},
    {"dialog.noFeedback.body", "当前没有可保存的 AI 回复。", "There is no AI reply to save."},
    {"dialog.renameFeedback.title", "重命名反馈记录", "Rename Feedback Record"},
    {"dialog.renameFeedback.label", "请输入新的记录名称：", "Enter a new record name:"},
    {"dialog.markReviewedResolved.title", "更新原反馈状态？", "Update Original Feedback Status?"},
    {"dialog.markReviewedResolved.body", "是否将本次复查的所有原反馈条目标记为“已解决”？", "Mark all original feedback items reviewed this time as Resolved?"},
    {"dialog.gitInitFailed", "Git 初始化失败", "Git Initialization Failed"},
    {"dialog.projectInitFailed", "项目初始化失败", "Project Initialization Failed"},
    {"dialog.feedbackStoreInitFailed", "反馈仓库初始化失败", "Feedback Store Initialization Failed"},
    {"dialog.noFilesFound.title", "未找到文件", "No Files Found"},
    {"dialog.noFilesFound.body", "没有扫描到支持的代码文件，请检查项目目录。", "No supported code files were found. Check the project folder."},
    {"error.projectDirMissing", "项目目录不存在。", "Project folder does not exist."},
    {"error.projectDirMissingFeedback", "项目目录不存在，无法打开反馈仓库。", "Project folder does not exist. Cannot open feedback store."},
    {"error.metadataDirCreate", "无法创建 .hwpilot 元数据目录。", "Could not create the .hwpilot metadata folder."},
    {"error.feedbackDirCreate", "无法创建 .hwpilot 目录。", "Could not create the .hwpilot folder."},
    {"error.metadataRead", "无法读取项目元数据。", "Could not read project metadata."},
    {"error.metadataInvalid", "项目元数据不是有效 JSON。", "Project metadata is not valid JSON."},
    {"error.metadataSaveNoDir", "项目目录不存在，无法保存元数据。", "Project folder does not exist. Cannot save metadata."},
    {"error.metadataWrite", "无法写入项目元数据。", "Could not write project metadata."},
    {"error.feedbackRead", "无法读取反馈数据。", "Could not read feedback data."},
    {"error.feedbackInvalid", "反馈数据不是有效 JSON。", "Feedback data is not valid JSON."},
    {"error.feedbackWrite", "无法写入反馈数据。", "Could not write feedback data."},
    {"error.feedbackItemMissing", "没有找到对应的问题条目。", "Could not find the matching issue item."},
    {"error.feedbackRecordMissing", "没有找到对应的反馈记录。", "Could not find the matching feedback record."},
    {"error.gitNoWorkdir", "Git 工作目录为空。", "Git working directory is empty."},
    {"error.gitTimeout", "Git 命令执行超时。", "Git command timed out."},

    {"status.ready", "就绪", "Ready"},
    {"status.aiRunning", "正在进行文件分析...", "Running file analysis..."},
    {"status.aiDone", "文件分析完成", "File analysis complete"},
    {"status.aiFailed", "文件分析失败", "File analysis failed"},
    {"status.aiSending", "正在向 DeepSeek 发送请求，请稍候...", "Sending request to DeepSeek, please wait..."},
    {"status.cancelledFeedback", "已取消本次反馈", "Feedback cancelled"},
    {"status.savedFeedback", "已保存为一条新的反馈记录", "Saved as a new feedback record"},
    {"status.refreshDone", "已刷新，当前扫描到 %1 个代码/文本文件", "Refreshed. Found %1 code/text files"},
    {"status.scanning", "正在扫描项目文件...", "Scanning project files..."},
    {"status.scanDone", "已扫描 %1 个代码/文本文件", "Scanned %1 code/text files"},
    {"status.committing", "正在提交当前版本快照...", "Committing current snapshot..."},
    {"status.commitFailed", "提交失败", "Commit failed"},
    {"status.commitDone", "已提交当前版本呢快照", "Current snapshot committed"},
    {"status.feedbackStatusUpdated", "反馈状态已更新为：%1", "Feedback status updated to: %1"},

    {"feedback.record", "反馈记录", "Feedback Record"},
    {"feedback.noRecordsForVersion", "当前版本还没有保存反馈记录。", "No feedback records have been saved for this version."},
    {"feedback.noRecordsForProject", "当前项目还没有反馈记录", "This project has no feedback records"},
    {"feedback.issueCount", "%1 项", "%1 items"},
    {"feedback.unnamedIssue", "未命名问题", "Untitled Issue"},
    {"feedback.unresolved", "未解决", "Unresolved"},
    {"feedback.resolved", "已解决", "Resolved"},
    {"feedback.uncertain", "无法确定", "Uncertain"},
    {"feedback.ignored", "忽略", "Ignored"},
    {"feedback.reviewState", "复查状态", "Review State"},
    {"feedback.reviewed", "已复查", "Reviewed"},
    {"feedback.notReviewed", "未复查", "Not Reviewed"},
    {"feedback.statusUpdateFailed", "状态更新失败", "Status Update Failed"},
    {"feedback.currentWorkspace", "当前工作区", "Current Working Directory"},
    {"feedback.untitledCommit", "未命名提交", "Untitled Commit"},
    {"feedback.unlinkedVersion", "未关联版本", "Unlinked Version"},
    {"feedback.unknownVersion", "未知版本 %1", "Unknown Version %1"},
    {"feedback.issueNumber", "问题 %1", "Issue %1"},
    {"feedback.noLocation", "未定位到具体文件", "No specific file location"},
    {"feedback.noSuggestion", "无具体建议", "No specific suggestion"},
    {"feedback.issueTemplate", "问题：%1\n严重程度：%2\n类别：%3\n位置：%4\n建议：%5", "Issue: %1\nSeverity: %2\nCategory: %3\nLocation: %4\nSuggestion: %5"},
    {"feedback.parseFailed", "未能解析结构化 JSON，已保存原始回复。", "Could not parse structured JSON. Saved the raw reply."},
    {"feedback.defaultSummary", "AI反馈", "AI Feedback"},
    {"feedback.viewIssues", "反馈问题", "Feedback Issues"},
    {"feedback.viewReviews", "反馈复查", "Feedback Reviews"},
    {"feedback.viewHeuristic", "启发式问题", "Guiding Questions"},

    {"version.currentWorkspace", "当前工作区", "Current Workspace"},
    {"version.currentWorkspaceNote", "当前磁盘上的项目文件和未提交变更。", "Project files and uncommitted changes on disk."},
    {"version.noProjectNode", "尚未打开项目", "No project opened"},
    {"version.noProjectNote", "请选择一个作业项目文件夹。", "Choose an assignment project folder."},
    {"version.dateCommit", "日期：%1\nCommit：%2", "Date: %1\nCommit: %2"},
    {"version.info", "信息概览", "Overview"},
    {"version.projectName", "项目名称", "Project"},
    {"version.projectFileCount", "项目文件总数", "Project Files"},
    {"version.projectFeedbackCount", "项目反馈总数", "Project Feedback"},
    {"version.version", "版本", "Version"},
    {"version.recordsUnit", "%1 条", "%1 records"},
    {"version.type", "类型", "Type"},
    {"version.historyCommit", "历史提交", "Historical Commit"},
    {"version.commitMessage", "提交信息", "Commit Message"},
    {"version.commitDate", "提交日期", "Commit Date"},
    {"version.branch", "所在分支", "Branch"},
    {"version.noBranch", "未找到本地分支引用", "No local branch reference found"},
    {"version.changeStat", "变更统计", "Change Stat"},
    {"version.diffTitle", "相对上一版本的变更", "Changes Since Previous Version"},
    {"diff.rootFolder", "根目录", "Root"},
    {"diff.lines", "%1（%2 行）", "%1 (%2 lines)"},
    {"diff.addedContent", "新增内容", "Added Content"},
    {"diff.deletedContent", "删除内容", "Deleted Content"},
    {"diff.binaryChange", "这个文件有元数据或二进制变化，没有可展示的文本行。", "This file has metadata or binary changes, with no displayable text lines."},
    {"diff.noContent", "没有可显示的内容变更。", "No displayable content changes."},
    {"diff.unknownFile", "未知文件", "Unknown File"},
    {"diff.filesTitle", "%1（%2 个文件）", "%1 (%2 files)"},
    {"diff.addedFiles", "新增文件", "Added Files"},
    {"diff.deletedFiles", "删除文件", "Deleted Files"},
    {"diff.modifiedFiles", "修改文件", "Modified Files"},
    {"diff.unparsed", "检测到了 diff，但没有解析出可展示的文本变更。", "A diff was detected, but no displayable text changes could be parsed."},
    {"diff.noStat", "暂无文件变更统计。", "No file change statistics."},
    {"diff.fileCount", "%1 个文件", "%1 files"},
    {"diff.file", "文件", "File"},
    {"diff.added", "新增", "Added"},
    {"diff.deleted", "删除", "Deleted"},
    {"diff.change", "变化", "Change"},

    {"prompt.system", "你是一位严谨、耐心的程序设计课助教。用户会提供作业要求、代码文件和补充问题。请聚焦于这次作业代码本身，分析可能的 Bug、边界条件、代码质量和实现建议。如果信息不足，请明确说明需要补充什么。", "You are a careful and patient programming-course teaching assistant. The user will provide assignment requirements, code files, and additional questions. Focus on the assignment code itself, and analyze possible bugs, edge cases, code quality, and implementation suggestions. If information is insufficient, clearly say what else is needed."},
    {"prompt.feedbackReviewSystem", "你是一位严谨、耐心的程序设计课助教。用户会提供当前代码和一组已选历史反馈。你的首要任务不是重新全面批改代码，而是逐条判断这些历史反馈指出的问题在当前代码中是否已经真的解决。判断必须基于当前代码证据；如果信息不足，请标记为无法确定。对于未解决的问题，只围绕对应历史反馈给出进一步反馈和下一步建议。", "You are a careful and patient programming-course teaching assistant. The user will provide current code and selected historical feedback items. Your primary task is not to do a broad new review, but to judge item by item whether the problem described by each selected historical feedback item has truly been fixed in the current code. Base the judgment on current-code evidence. If the evidence is insufficient, mark it as uncertain. For unresolved items, give follow-up feedback and next-step suggestions only for that selected historical feedback item."},
    {"prompt.heuristicSystem", "你是一位温和、耐心、擅长启发式教学的程序设计课助教。请根据作业要求和用户提供的材料，生成能引导学生独立思考的问题。语气要友善、克制、鼓励式，避免批评、命令或下结论；不要直接给完整答案。问题应覆盖需求理解、边界情况、数据结构、算法设计、测试策略和代码组织。", "You are a gentle, patient programming-course teaching assistant skilled in heuristic teaching. Based on the assignment requirements and provided materials, generate questions that guide students to think independently. Keep the tone friendly, restrained, and encouraging. Avoid criticism, commands, or definitive judgments, and do not give complete answers directly. Cover requirement understanding, edge cases, data structures, algorithm design, testing strategy, and code organization."},
    {"prompt.userQuestion", "【用户问题】\n", "[User Question]\n"},
    {"prompt.selectedFeedback", "【反馈记录】\n", "[Selected Feedback Records]\n"},
    {"prompt.assignment", "【作业要求】\n", "[Assignment Requirements / Task Description]\n"},
    {"prompt.submittedCode", "【学生提交的代码，共 %1 个文件】\n\n", "[Student Submitted Code, %1 files]\n\n"},
    {"prompt.fileHeader", "### 文件：`%1`\n", "### File: `%1`\n"},
    {"prompt.structuredFeedback", "请严格输出一个 JSON 对象，不要在 JSON 外添加解释文字。JSON 结构如下：\n{\n  \"summary\": \"反馈记录标题，4-10 个汉字或中文短语，最多 12 个中文字符，例如：编译阻塞、空实现过多、接口缺失。不要写完整句子，不要包含逗号、分号或多项展开\",\n  \"items\": [\n    {\n      \"severity\": \"high|medium|low\",\n      \"filePath\": \"相关文件路径，无法判断则留空\",\n      \"line\": 具体行号，无法判断则为 -1,\n      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n      \"title\": \"问题标题，尽量控制在 6-16 个汉字\",\n      \"suggestion\": \"具体修改或学习建议\"\n    }\n  ],\n  \"rawReport\": \"完整自然语言反馈报告\"\n}\nsummary 只用于反馈记录列表展示，必须短；详细原因、多个问题和完整说明都写入 rawReport 或 items。如果没有发现具体问题，items 输出空数组，并在 summary 中使用类似“暂无问题”的短标题，在 rawReport 中说明。", "Strictly output one JSON object. Do not add explanatory text outside JSON. Use this structure:\n{\n  \"summary\": \"A short feedback-record title, 3-8 English words, no full sentence, no comma/semicolon/list expansion, e.g. Build Blocked, Missing Interfaces, Empty Implementation\",\n  \"items\": [\n    {\n      \"severity\": \"high|medium|low\",\n      \"filePath\": \"Related file path, or empty if unknown\",\n      \"line\": line number, or -1 if unknown,\n      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n      \"title\": \"Issue title, preferably 3-10 words\",\n      \"suggestion\": \"Concrete fix or learning suggestion\"\n    }\n  ],\n  \"rawReport\": \"Complete natural-language feedback report\"\n}\nsummary is only for list display and must be short. Put detailed reasoning, multiple issues, and complete explanations in rawReport or items. If no concrete issue is found, output an empty items array, use a short summary such as No Issues, and explain in rawReport."},
    {"prompt.feedbackReviewStructured", "请严格输出一个 JSON 对象，不要在 JSON 外添加解释文字。JSON 结构如下：\n{\n  \"summary\": \"复查记录标题，4-10 个汉字或中文短语，最多 12 个中文字符，例如：复查未完成、反馈已解决、部分待处理。不要写完整句子，不要包含逗号、分号或多项展开\",\n  \"items\": [\n    {\n      \"sourceFeedbackId\": \"被复查的反馈条目ID，必须原样返回用户提供的反馈条目ID\",\n      \"severity\": \"high|medium|low\",\n      \"filePath\": \"判断依据对应的文件路径，无法判断则留空\",\n      \"line\": 具体行号，无法判断则为 -1,\n      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n      \"status\": \"resolved|unresolved|uncertain\",\n      \"title\": \"被复查反馈的短标题，尽量控制在 6-16 个汉字\",\n      \"suggestion\": \"先写判断结论：已解决/未解决/无法确定；再说明当前代码证据。如果未解决，继续针对这条原反馈给出进一步修改建议；如果已解决，简要说明依据；如果无法确定，说明缺少什么信息。\"\n    }\n  ],\n  \"rawReport\": \"完整自然语言复查报告\"\n}\n必须逐条覆盖用户勾选的反馈，并原样带回每条反馈的 sourceFeedbackId。不要泛泛批改没有被勾选的新问题；只有当它直接影响所选反馈是否解决时，才可以补充说明。", "Strictly output one JSON object. Do not add explanatory text outside JSON. Use this structure:\n{\n  \"summary\": \"A short review-record title, 3-8 English words, no full sentence, no comma/semicolon/list expansion, e.g. Still Open, Fixed Feedback, Partly Addressed\",\n  \"items\": [\n    {\n      \"sourceFeedbackId\": \"The reviewed feedback item ID. Return the feedback item ID provided by the user exactly as-is\",\n      \"severity\": \"high|medium|low\",\n      \"filePath\": \"Evidence file path, or empty if unknown\",\n      \"line\": line number, or -1 if unknown,\n      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n      \"status\": \"resolved|unresolved|uncertain\",\n      \"title\": \"Short title of the reviewed feedback item\",\n      \"suggestion\": \"Start with the judgment: resolved/unresolved/uncertain. Then explain the current-code evidence. If unresolved, provide follow-up advice for this original feedback item. If resolved, briefly explain why. If uncertain, state what evidence is missing.\"\n    }\n  ],\n  \"rawReport\": \"Complete natural-language feedback-review report\"\n}\nYou must cover every selected feedback item and return each sourceFeedbackId exactly. Do not broadly review unrelated new issues unless they directly affect whether the selected feedback has been fixed."},
    {"prompt.heuristicStructuredFeedback", "请严格输出一个 JSON 对象，不要在 JSON 外添加解释文字。JSON 结构如下：\n{\n  \"summary\": \"启发式问题记录标题，4-10 个汉字或中文短语，最多 12 个中文字符，例如：边界思考、算法选择、测试追问。不要写完整句子，不要包含逗号、分号或多项展开\",\n  \"items\": [\n    {\n      \"severity\": \"low\",\n      \"filePath\": \"相关文件路径，无法判断则留空\",\n      \"line\": 具体行号，无法判断则为 -1,\n      \"category\": \"learning\",\n      \"title\": \"启发式问题标题，尽量控制在 6-16 个汉字\",\n      \"suggestion\": \"完整的启发式问题。请用温和、鼓励、开放式的语气提问，例如：你可以试着想一想……；如果输入变成……会发生什么？避免直接指出错误或给出答案。\"\n    }\n  ],\n  \"rawReport\": \"完整自然语言问题列表\"\n}\n启发式问题只用于引导学生继续思考，不要给完整答案，不要使用批评性语气。每个 item 的 title 用于列表展示，suggestion 必须写成一个可以直接展示给学生的完整问题。", "Strictly output one JSON object. Do not add explanatory text outside JSON. Use this structure:\n{\n  \"summary\": \"A short guiding-question record title, 3-8 English words, no full sentence, no comma/semicolon/list expansion, e.g. Edge Cases, Algorithm Choice, Testing Prompts\",\n  \"items\": [\n    {\n      \"severity\": \"low\",\n      \"filePath\": \"Related file path, or empty if unknown\",\n      \"line\": line number, or -1 if unknown,\n      \"category\": \"learning\",\n      \"title\": \"Short guiding-question title, preferably 3-10 words\",\n      \"suggestion\": \"The complete guiding question. Use a gentle, encouraging, open-ended tone, such as: You might try thinking about...; What would happen if the input became...? Avoid directly pointing out mistakes or giving the answer.\"\n    }\n  ],\n  \"rawReport\": \"Complete natural-language list of guiding questions\"\n}\nGuiding questions are only for helping the student keep thinking. Do not provide full answers or use a critical tone. Each item.title is for the list, and item.suggestion must be a complete question suitable for direct display to the student."},
};
}  // namespace

namespace AppText {

QString language() {
    return QSettings("HWpilot", "HWpilot").value("ui/language", "zh").toString();
}

void setLanguage(const QString& language) {
    QSettings("HWpilot", "HWpilot").setValue("ui/language", language == "en" ? "en" : "zh");
}

QString get(const char* key) {
    static QHash<QString, QPair<QString, QString>> table;
    if (table.isEmpty()) {
        for (const Entry& entry : kEntries)
            table.insert(QString::fromUtf8(entry.key), {QString::fromUtf8(entry.zh), QString::fromUtf8(entry.en)});
    }

    const auto item = table.constFind(QString::fromUtf8(key));
    if (item == table.constEnd())
        return QString::fromUtf8(key);

    return language() == "en" ? item.value().second : item.value().first;
}

}  // namespace AppText
