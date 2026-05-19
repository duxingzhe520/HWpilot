#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "HWpilotLLM/HWpilotLLM.h"

namespace {
constexpr int NoteRole = Qt::UserRole + 2;
constexpr int CommitHashRole = Qt::UserRole + 3;
constexpr int FilePathRole = Qt::UserRole + 4;
constexpr int IsDirectoryRole = Qt::UserRole + 5;
constexpr int DetailHtmlRole = Qt::UserRole + 6;

struct DiffFileBlock {
    QString path;
    QString folder;
    QStringList lines;
};

struct DiffStatEntry {
    QString path;
    int additions = 0;
    int deletions = 0;
    QString detail;
};

QString htmlEscape(const QString& text) {
    QString escaped = text.toHtmlEscaped();
    return escaped.replace('\n', "<br>");
}

QString extractJsonObjectText(const QString& text) {
    QString trimmed = text.trimmed();
    if (trimmed.startsWith("```")) {
        const int firstNewline = trimmed.indexOf('\n');
        const int lastFence = trimmed.lastIndexOf("```");
        if (firstNewline >= 0 && lastFence > firstNewline)
            trimmed = trimmed.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
    }

    const int firstBrace = trimmed.indexOf('{');
    const int lastBrace = trimmed.lastIndexOf('}');
    if (firstBrace >= 0 && lastBrace > firstBrace)
        return trimmed.mid(firstBrace, lastBrace - firstBrace + 1);

    return trimmed;
}

QString structuredFeedbackInstruction() {
    return
        "请严格输出一个 JSON 对象，不要在 JSON 外添加解释文字。JSON 结构如下：\n"
        "{\n"
        "  \"summary\": \"一句话总结本次反馈\",\n"
        "  \"items\": [\n"
        "    {\n"
        "      \"severity\": \"high|medium|low\",\n"
        "      \"filePath\": \"相关文件路径，无法判断则留空\",\n"
        "      \"line\": 具体行号，无法判断则为 -1,\n"
        "      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n"
        "      \"title\": \"问题标题\",\n"
        "      \"suggestion\": \"具体修改或学习建议\"\n"
        "    }\n"
        "  ],\n"
        "  \"rawReport\": \"完整自然语言反馈报告\"\n"
        "}\n"
        "如果没有发现具体问题，items 输出空数组，并在 summary 与 rawReport 中说明。";
}

bool isSupportedFilePath(const QString& path) {
    const QFileInfo info(path);
    if (info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0)
        return true;

    return HWFileScanner::DEFAULT_CODE_EXTENSIONS.contains(info.suffix().toLower());
}

QString extensionForPath(const QString& path) {
    const QFileInfo info(path);
    if (info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0)
        return "cmake";

    return info.suffix().toLower();
}

QString folderForPath(const QString& path) {
    const int slash = path.lastIndexOf('/');
    if (slash <= 0)
        return "根目录";
    return path.left(slash);
}

QString pathFromDiffHeader(const QString& line) {
    const QString prefix = "diff --git ";
    if (!line.startsWith(prefix))
        return QString();

    const QStringList parts = line.mid(prefix.size()).split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return QString();

    QString path = parts.at(1);
    if (path.startsWith("b/"))
        path = path.mid(2);
    return path;
}

QString diffLineHtml(const QString& line) {
    QString color = "#475569";
    QString background = "#ffffff";
    QString border = "#e5e7eb";
    QString prefix = "&nbsp;";

    if (line.startsWith("@@")) {
        color = "#1d4ed8";
        background = "#eff6ff";
        border = "#bfdbfe";
        prefix = "@";
    } else if (line.startsWith('+') && !line.startsWith("+++")) {
        color = "#166534";
        background = "#ecfdf3";
        border = "#bbf7d0";
        prefix = "+";
    } else if (line.startsWith('-') && !line.startsWith("---")) {
        color = "#991b1b";
        background = "#fef2f2";
        border = "#fecaca";
        prefix = "-";
    } else if (line.startsWith("diff --git") || line.startsWith("index ") || line.startsWith("---") || line.startsWith("+++")) {
        color = "#64748b";
        background = "#f8fafc";
        border = "#e2e8f0";
        prefix = " ";
    }

    QString text = line.toHtmlEscaped();
    text.replace(" ", "&nbsp;");
    return QString("<div style=\"font-family: Menlo, Consolas, monospace; font-size: 12px; line-height: 1.45; color: %1; background: %2; border-left: 3px solid %3; padding: 1px 8px;\"><span style=\"display:inline-block; width:18px; color:%1;\">%4</span>%5</div>")
        .arg(color, background, border, prefix, text);
}

QString renderReadableDiff(const QString& diff) {
    if (diff.trimmed().isEmpty())
        return "<p>没有可显示的内容变更。</p>";

    QList<DiffFileBlock> blocks;
    DiffFileBlock current;
    const QStringList lines = diff.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("diff --git ")) {
            if (!current.path.isEmpty())
                blocks.append(current);

            current = DiffFileBlock();
            current.path = pathFromDiffHeader(line);
            if (current.path.isEmpty())
                current.path = "未知文件";
            current.folder = folderForPath(current.path);
        }

        if (current.path.isEmpty()) {
            current.path = "变更内容";
            current.folder = "根目录";
        }
        current.lines.append(line);
    }

    if (!current.path.isEmpty())
        blocks.append(current);

    QMap<QString, QList<DiffFileBlock>> groups;
    for (const DiffFileBlock& block : blocks)
        groups[block.folder].append(block);

    QString html;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        html += QString("<h4 style=\"margin:18px 0 8px 0; color:#0f172a;\">%1</h4>").arg(htmlEscape(it.key()));
        for (const DiffFileBlock& block : it.value()) {
            html += QString("<div style=\"border:1px solid #d9dee7; border-radius: 0px; overflow:hidden; margin-bottom:10px; background:#ffffff;\">"
                            "<div style=\"background:#f1f5f9; padding:8px 10px; font-weight:600; color:#1f2937;\">%1</div>")
                        .arg(htmlEscape(block.path));
            for (const QString& line : block.lines)
                html += diffLineHtml(line);
            html += "</div>";
        }
    }

    return html;
}

QString renderDiffStat(const QString& diffStat) {
    if (diffStat.trimmed().isEmpty())
        return "<p style=\"margin:0; color:#64748b;\">暂无文件变更统计。</p>";

    QList<DiffStatEntry> entries;
    QString summary;
    const QStringList lines = diffStat.split('\n', Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        const int pipeIndex = line.indexOf('|');
        if (pipeIndex < 0) {
            summary = line;
            continue;
        }

        DiffStatEntry entry;
        entry.path = line.left(pipeIndex).trimmed();
        entry.detail = line.mid(pipeIndex + 1).trimmed();
        for (const QChar ch : entry.detail) {
            if (ch == '+')
                ++entry.additions;
            else if (ch == '-')
                ++entry.deletions;
        }
        entries.append(entry);
    }

    if (entries.isEmpty())
        return QString("<p style=\"margin:0; color:#64748b;\">%1</p>").arg(htmlEscape(summary.isEmpty() ? diffStat.trimmed() : summary));

    int totalAdditions = 0;
    int totalDeletions = 0;
    for (const DiffStatEntry& entry : entries) {
        totalAdditions += entry.additions;
        totalDeletions += entry.deletions;
    }

    QString html;
    html += "<div style=\"display:flex; gap:8px; margin:2px 0 10px 0; flex-wrap:wrap;\">";
    html += QString("<span style=\"background:#f1f5f9; border:1px solid #d9dee7; border-radius: 0px; padding:5px 9px; color:#334155;\">%1 个文件</span>").arg(entries.size());
    html += QString("<span style=\"background:#ecfdf3; border:1px solid #bbf7d0; border-radius: 0px; padding:5px 9px; color:#166534;\">+%1</span>").arg(totalAdditions);
    html += QString("<span style=\"background:#fef2f2; border:1px solid #fecaca; border-radius: 0px; padding:5px 9px; color:#991b1b;\">-%1</span>").arg(totalDeletions);
    if (!summary.isEmpty())
        html += QString("<span style=\"color:#64748b; padding:5px 0;\">%1</span>").arg(htmlEscape(summary));
    html += "</div>";

    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; border:1px solid #e2e8f0; border-radius: 0px; overflow:hidden;\">";
    html += "<tr style=\"background:#f8fafc;\"><th align=\"left\" style=\"color:#475569; font-weight:600;\">文件</th><th align=\"right\" style=\"color:#166534; font-weight:600; width:70px;\">新增</th><th align=\"right\" style=\"color:#991b1b; font-weight:600; width:70px;\">删除</th><th align=\"left\" style=\"color:#64748b; font-weight:600; width:160px;\">变化</th></tr>";
    for (const DiffStatEntry& entry : entries) {
        html += QString("<tr style=\"border-top:1px solid #e2e8f0;\"><td>%1</td><td align=\"right\" style=\"color:#166534;\">+%2</td><td align=\"right\" style=\"color:#991b1b;\">-%3</td><td style=\"font-family:Menlo, Consolas, monospace; color:#64748b;\">%4</td></tr>")
                    .arg(htmlEscape(entry.path))
                    .arg(entry.additions)
                    .arg(entry.deletions)
                    .arg(htmlEscape(entry.detail));
    }
    html += "</table>";
    return html;
}

QString pillHtml(const QString& text, const QString& color, const QString& background, const QString& border) {
    return QString("<span style=\"border:1px solid %1; border-radius: 0px; padding:2px 7px; color:%2; background:%3; font-size:12px;\">%4</span>")
        .arg(border, color, background, htmlEscape(text));
}

QString severityPill(const QString& severity) {
    if (severity == "high")
        return pillHtml("high", "#991b1b", "#fef2f2", "#fecaca");
    if (severity == "low")
        return pillHtml("low", "#166534", "#ecfdf3", "#bbf7d0");
    return pillHtml(severity.isEmpty() ? "medium" : severity, "#92400e", "#fffbeb", "#fde68a");
}

QString renderFeedbackRecordDetail(const FeedbackRecord& feedback) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(feedback.summary.isEmpty() ? "反馈记录" : feedback.summary));
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">时间</td><td>%1</td></tr>").arg(htmlEscape(feedback.createdAt));
    html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>%1&nbsp;&nbsp;%2&nbsp;&nbsp;%3</td></tr>")
                .arg(pillHtml(feedback.mode.isEmpty() ? "文件分析" : feedback.mode, "#1d4ed8", "#eff6ff", "#bfdbfe"),
                     pillHtml(feedback.parseStatus.isEmpty() ? "unknown" : feedback.parseStatus, "#475569", "#f8fafc", "#d9dee7"),
                     pillHtml(QString("%1 个问题").arg(feedback.items.size()), "#334155", "#f1f5f9", "#d9dee7"));
    html += QString("<tr><td style=\"color:#64748b;\">摘要</td><td>%1</td></tr>").arg(htmlEscape(feedback.summary.isEmpty() ? "无摘要" : feedback.summary));
    html += "</table>";
    html += "<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">完整反馈</h3>";
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(htmlEscape(feedback.rawContent.isEmpty() ? "这次反馈没有保存正文。" : feedback.rawContent));
    return html;
}

QString renderFeedbackItemDetail(const FeedbackItem& item) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(item.title.isEmpty() ? "问题详情" : item.title));
    const QString location = item.filePath.isEmpty()
                                 ? "未定位到具体文件"
                                 : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">属性</td><td>%1&nbsp;&nbsp;%2&nbsp;&nbsp;%3</td></tr>")
                .arg(severityPill(item.severity),
                     pillHtml(item.category.isEmpty() ? "other" : item.category, "#475569", "#f8fafc", "#d9dee7"),
                     pillHtml(item.status.isEmpty() ? "open" : item.status, "#1d4ed8", "#eff6ff", "#bfdbfe"));
    html += QString("<tr><td style=\"color:#64748b;\">位置</td><td>%1</td></tr>").arg(htmlEscape(location));
    html += "</table>";
    html += "<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">完整建议</h3>";
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(htmlEscape(item.suggestion.isEmpty() ? "这条问题没有保存建议。" : item.suggestion));
    return html;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    applyStyle();
}

void MainWindow::buildUi() {
    setWindowTitle("HWpilot - proj finished by YANG R.C. and SONG R.Y.");
    resize(1280, 820);

    auto* root = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    auto* splitter = new QSplitter(Qt::Horizontal, root);

    auto* leftPanel = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);
    buildLeftPanel();
    leftLayout->addWidget(m_projectNameLabel);
    leftLayout->addWidget(m_projectPathLabel);
    leftLayout->addWidget(m_fileCountLabel);
    leftLayout->addWidget(m_feedbackCountLabel);
    auto* projectActionsLayout = new QHBoxLayout();
    projectActionsLayout->setContentsMargins(0, 0, 0, 0);
    projectActionsLayout->setSpacing(8);
    projectActionsLayout->addWidget(m_openProjectButton);
    projectActionsLayout->addWidget(m_commitButton);
    leftLayout->addLayout(projectActionsLayout);
    leftLayout->addWidget(m_versionTree, 1);

    buildAiPanel();
    buildCenterPanel();

    auto* centerPanel = new QWidget(splitter);
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->addWidget(m_tabs);

    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 1000});

    rootLayout->addWidget(splitter);
    setCentralWidget(root);

    m_statusLabel = new QLabel("Please select a project folder", this);
    statusBar()->addWidget(m_statusLabel, 1);
    refreshProjectPanel();
}

void MainWindow::buildLeftPanel() {
    m_projectNameLabel = new QLabel(this);
    m_projectNameLabel->setObjectName("ProjectName");
    m_projectPathLabel = new QLabel(this);
    m_projectPathLabel->setObjectName("MetaLabel");
    m_projectPathLabel->setWordWrap(true);
    m_fileCountLabel = new QLabel(this);
    m_fileCountLabel->setObjectName("MetaLabel");
    m_feedbackCountLabel = new QLabel(this);
    m_feedbackCountLabel->setObjectName("MetaLabel");

    m_openProjectButton = new QPushButton("Open Folder", this);
    connect(m_openProjectButton, &QPushButton::clicked, this, &MainWindow::openProjectFolder);

    m_commitButton = new QPushButton("commit", this);
    connect(m_commitButton, &QPushButton::clicked, this, &MainWindow::commitCurrentSnapshot);

    m_versionTree = new QTreeWidget(this);
    m_versionTree->setHeaderLabel("Commits");
    m_versionTree->setAlternatingRowColors(true);
    m_versionRoot = m_versionTree->invisibleRootItem();
    appendVersionNode("No folder opened.", "Please select a project folder.");
    m_versionTree->expandAll();
    m_versionTree->setCurrentItem(m_versionRoot->child(0));
    connect(m_versionTree, &QTreeWidget::currentItemChanged, this, &MainWindow::updateCurrentVersionPanel);
}

void MainWindow::buildCenterPanel() {
    m_tabs = new QTabWidget(this);

    m_selectAllFilesButton = new QPushButton("Select All", this);
    connect(m_selectAllFilesButton, &QPushButton::clicked, this, &MainWindow::selectAllFiles);

    m_fileTree = new QTreeWidget(this);
    m_fileTree->setHeaderLabel("项目文件");
    m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileTree->setAlternatingRowColors(true);
    connect(m_fileTree, &QTreeWidget::itemChanged, this, &MainWindow::handleFileItemChanged);

    m_changeSummary = new QTextBrowser(this);
    m_reviewReport = new QTextBrowser(this);
    m_feedbackTree = new QTreeWidget(this);
    m_feedbackTree->setHeaderLabels({"反馈问题", "严重程度", "位置", "状态"});
    m_feedbackTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_feedbackTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_feedbackTree->setAlternatingRowColors(true);
    m_feedbackTree->header()->setStretchLastSection(false);
    m_feedbackTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(m_feedbackTree, &QTreeWidget::currentItemChanged, this, &MainWindow::handleFeedbackTreeSelection);

    auto* versionPanel = new QWidget(this);
    auto* versionLayout = new QVBoxLayout(versionPanel);
    versionLayout->setContentsMargins(0, 0, 0, 0);
    versionLayout->addWidget(m_changeSummary, 1);

    auto* fileAnalysisPanel = new QWidget(this);
    auto* fileAnalysisLayout = new QVBoxLayout(fileAnalysisPanel);
    fileAnalysisLayout->setContentsMargins(0, 0, 0, 0);

    auto* fileAnalysisSplitter = new QSplitter(Qt::Horizontal, fileAnalysisPanel);
    auto* filePickerPanel = new QWidget(fileAnalysisSplitter);
    auto* filePickerLayout = new QVBoxLayout(filePickerPanel);
    filePickerLayout->setContentsMargins(0, 0, 0, 0);
    filePickerLayout->setSpacing(8);
    filePickerLayout->addWidget(m_selectAllFilesButton);
    filePickerLayout->addWidget(m_fileTree, 1);

    auto* aiPanel = new QWidget(fileAnalysisSplitter);
    auto* aiLayout = new QVBoxLayout(aiPanel);
    aiLayout->setContentsMargins(0, 0, 0, 0);
    aiLayout->setSpacing(8);
    aiLayout->addWidget(m_aiTitleLabel);
    aiLayout->addWidget(m_taskEdit);
    aiLayout->addWidget(m_questionEdit);
    aiLayout->addWidget(m_includeCodeCheck);
    aiLayout->addWidget(m_includeHistoryCheck);
    aiLayout->addWidget(m_analyzeButton);
    aiLayout->addWidget(m_responseView, 1);
    aiLayout->addWidget(m_saveFeedbackButton);

    fileAnalysisSplitter->addWidget(filePickerPanel);
    fileAnalysisSplitter->addWidget(aiPanel);
    fileAnalysisSplitter->setStretchFactor(0, 1);
    fileAnalysisSplitter->setStretchFactor(1, 1);
    fileAnalysisSplitter->setSizes({360, 540});
    fileAnalysisLayout->addWidget(fileAnalysisSplitter, 1);

    auto* feedbackPanel = new QWidget(this);
    auto* feedbackLayout = new QVBoxLayout(feedbackPanel);
    feedbackLayout->setContentsMargins(0, 0, 0, 0);
    feedbackLayout->setSpacing(8);
    feedbackLayout->addWidget(m_reviewReport, 1);
    feedbackLayout->addWidget(m_feedbackTree, 1);

    m_tabs->addTab(versionPanel, "版本概览");
    m_tabs->addTab(fileAnalysisPanel, "智能文件分析");
    m_tabs->addTab(feedbackPanel, "反馈记录");
}

void MainWindow::buildAiPanel() {
    m_aiTitleLabel = new QLabel("智能分析", this);
    m_aiTitleLabel->setObjectName("PanelTitle");

    m_taskEdit = new QTextEdit(this);
    m_taskEdit->setPlaceholderText("Please enter assignment requirements or tick the requirement document on the left...");
    m_taskEdit->setFixedHeight(118);

    m_questionEdit = new QTextEdit(this);
    m_questionEdit->setPlaceholderText("Additional remarks on the assignment...");
    m_questionEdit->setFixedHeight(86);

    m_includeCodeCheck = new QCheckBox("包含勾选的代码文件", this);
    m_includeCodeCheck->setChecked(true);

    m_includeHistoryCheck = new QCheckBox("包含当前版本的历史反馈", this);
    m_includeHistoryCheck->setChecked(true);

    m_analyzeButton = new QPushButton("智能分析", this);
    m_analyzeButton->setObjectName("PrimaryButton");
    connect(m_analyzeButton, &QPushButton::clicked, this, &MainWindow::startAiAnalysis);

    m_responseView = new QTextEdit(this);
    m_responseView->setReadOnly(true);
    m_responseView->setPlaceholderText("AI 回复会显示在这里。");

    m_saveFeedbackButton = new QPushButton("保存为一条反馈记录", this);
    connect(m_saveFeedbackButton, &QPushButton::clicked, this, &MainWindow::saveFeedbackToVersion);
}

void MainWindow::applyStyle() {
    qApp->setStyleSheet(
        "QMainWindow, QWidget { background: #eef2f6; color: #1f2937; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Arial, sans-serif; font-size: 14px; line-height: 1.45; }"
        "QSplitter::handle { background: #e4eaf1; }"
        "QSplitter::handle:hover { background: #d5dee9; }"
        "QToolButton, QPushButton { background: #ffffff; border: 0; border-radius: 0px; padding: 8px 13px; color: #243244; line-height: 1.45; }"
        "QToolButton:hover, QPushButton:hover { background: #f3f8ff; }"
        "QToolButton:pressed, QPushButton:pressed { background: #e8f1ff; }"
        "QPushButton#PrimaryButton { background: #2563eb; color: white; font-weight: 600; }"
        "QPushButton#PrimaryButton:hover { background: #1d4ed8; }"
        "QPushButton:disabled { color: #94a3b8; background: #e8edf3; }"
        "QTreeWidget, QTextEdit, QTextBrowser, QComboBox { background: #ffffff; border: 0; border-radius: 0px; padding: 5px; selection-background-color: #dbeafe; selection-color: #0f172a; line-height: 1.45; }"
        "QTextEdit[readOnly=\"true\"] { background: #fbfcfe; }"
        "QHeaderView::section { background: #f3f6fa; color: #475569; border: 0; padding: 7px 8px; font-weight: 600; line-height: 1.45; }"
        "QLabel#PanelTitle { background: transparent; color: #0f172a; font-size: 16px; font-weight: 700; padding: 2px 0 6px 0; line-height: 1.45; }"
        "QLabel#ProjectName { background: transparent; color: #0f172a; font-size: 18px; font-weight: 700; padding: 2px 0 3px 0; line-height: 1.45; }"
        "QLabel#MetaLabel { background: transparent; color: #5b6878; padding: 1px 0; line-height: 1.45; }"
        "QCheckBox { background: transparent; color: #334155; spacing: 8px; padding: 2px 0; line-height: 1.45; }"
        "QCheckBox::indicator { width: 15px; height: 15px; border-radius: 0px; border: 0; background: #ffffff; }"
        "QCheckBox::indicator:checked { background: #2563eb; }"
        "QTreeWidget { alternate-background-color: #f8fafc; }"
        "QTreeWidget::item { padding: 6px 5px; border-radius: 0px; }"
        "QTreeWidget::item:hover { background: #eef6ff; }"
        "QTreeWidget::item:selected { background: #dbeafe; color: #102a43; }"
        "QTabWidget::pane { border: 0; background: #ffffff; border-radius: 0px; top: -1px; }"
        "QTabBar::tab { background: #e4eaf2; color: #516071; padding: 9px 18px; border: 0; border-radius: 0px; margin-right: 4px; line-height: 1.45; }"
        "QTabBar::tab:hover { background: #eef5ff; color: #1f2937; }"
        "QTabBar::tab:selected { background: #ffffff; color: #0f172a; font-weight: 700; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: #c7d1df; border-radius: 0px; min-height: 28px; }"
        "QScrollBar::handle:vertical:hover { background: #9fb0c4; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }"
        "QScrollBar::handle:horizontal { background: #c7d1df; border-radius: 0px; min-width: 28px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QStatusBar { background: #ffffff; color: #475569; border: 0; line-height: 1.45; }");
}

void MainWindow::openProjectFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, "选择作业项目文件夹", m_projectDir);
    if (folder.isEmpty())
        return;

    setProjectFolder(folder);
    scanCurrentProject();
}

void MainWindow::commitCurrentSnapshot() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const QStringList paths = m_gitService.changedPaths();
    if (paths.isEmpty()) {
        QMessageBox::information(this, "没有可提交的变更", "当前工作区没有 Git 可提交的变更。");
        return;
    }

    bool ok = false;
    const QString defaultMessage = QString("保存作业进度：%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    const QString message = QInputDialog::getText(this, "Commit message", "请输入 commit message：", QLineEdit::Normal, defaultMessage, &ok).trimmed();
    if (!ok)
        return;
    if (message.isEmpty()) {
        QMessageBox::information(this, "Commit message 为空", "请输入 commit message 后再提交。");
        return;
    }

    m_statusLabel->setText("正在提交当前快照...");
    const GitCommandResult addResult = m_gitService.addPaths(paths);
    if (!addResult.success) {
        QMessageBox::warning(this, "Git add 失败", addResult.stderrText.trimmed().isEmpty() ? addResult.stdoutText.trimmed() : addResult.stderrText.trimmed());
        m_statusLabel->setText("提交失败");
        return;
    }

    const GitCommandResult commitResult = m_gitService.commit(message);
    if (!commitResult.success) {
        QMessageBox::warning(this, "Git commit 失败", commitResult.stderrText.trimmed().isEmpty() ? commitResult.stdoutText.trimmed() : commitResult.stderrText.trimmed());
        m_statusLabel->setText("提交失败");
        return;
    }

    const QString newHead = m_gitService.currentHead();
    refreshGitState();
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        if (item->data(0, CommitHashRole).toString() == newHead) {
            m_versionTree->setCurrentItem(item);
            break;
        }
    }
    m_statusLabel->setText("已提交当前快照");
}

void MainWindow::setProjectFolder(const QString& folderPath) {
    m_projectDir = QDir(folderPath).absolutePath();
    QString errorMessage;
    if (!m_projectManager.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(this, "项目初始化失败", errorMessage);
        return;
    }
    if (!m_feedbackStore.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(this, "反馈仓库初始化失败", errorMessage);
        return;
    }
    if (m_feedbackStore.isEmpty() && !m_projectManager.data().feedbacks.isEmpty()) {
        m_feedbackStore.importFeedbacks(m_projectManager.data().feedbacks);
        m_feedbackStore.save(&errorMessage);
    }

    m_gitService.setWorkingDirectory(m_projectDir);
    if (!m_gitService.isGitRepo()) {
        const GitCommandResult initResult = m_gitService.initRepo();
        if (!initResult.success) {
            QMessageBox::warning(this, "Git 初始化失败", initResult.stderrText.trimmed());
        }
    }

    m_taskEdit->setPlainText(m_projectManager.data().assignmentText);
    refreshProjectPanel();
    refreshGitState();
}

void MainWindow::scanCurrentProject() {
    if (m_projectDir.isEmpty()) {
        openProjectFolder();
        return;
    }

    m_statusLabel->setText("正在扫描项目文件...");
    m_files = HWFileScanner::scanDirectory(m_projectDir);
    m_projectManager.data().assignmentText = m_taskEdit->toPlainText().trimmed();
    QString errorMessage;
    m_projectManager.save(&errorMessage);
    populateFileTree();
    refreshProjectPanel();
    refreshChangeSummary();
    m_statusLabel->setText(QString("已扫描 %1 个代码/文本文件").arg(m_files.size()));

    if (m_files.isEmpty()) {
        QMessageBox::warning(this, "未找到文件", "没有扫描到支持的代码文件，请检查项目目录。");
    }
}

void MainWindow::refreshProjectPanel() {
    const QString name = m_projectDir.isEmpty() ? "尚未打开项目" : QFileInfo(m_projectDir).fileName();
    m_projectNameLabel->setText(name);
    m_projectPathLabel->setText(m_projectDir.isEmpty() ? "请选择一个作业文件夹开始分析" : m_projectDir);
    m_fileCountLabel->setText(QString("文件：%1").arg(m_files.size()));
    m_feedbackCountLabel->setText(QString("反馈记录：%1").arg(m_feedbackStore.allFeedbacks().size()));
}

void MainWindow::populateFileTree() {
    QStringList paths;
    const QString commitHash = selectedCommitHash();
    if (commitHash.isEmpty()) {
        for (const CodeFile& file : m_files)
            paths.append(file.relativePath);
    } else {
        const QStringList committedPaths = m_gitService.filesAtCommit(commitHash);
        for (const QString& path : committedPaths) {
            if (isSupportedFilePath(path))
                paths.append(path);
        }
    }

    populateFileTreeForPaths(paths);
}

void MainWindow::populateFileTreeForPaths(const QStringList& paths) {
    m_updatingFileTree = true;
    m_fileTree->clear();

    QStringList sortedPaths = paths;
    sortedPaths.removeDuplicates();
    sortedPaths.sort(Qt::CaseInsensitive);

    for (const QString& path : sortedPaths) {
        QTreeWidgetItem* parent = m_fileTree->invisibleRootItem();
        QString currentPath;
        const QStringList parts = path.split('/', Qt::SkipEmptyParts);
        for (int i = 0; i < parts.size(); ++i) {
            const bool isLast = i == parts.size() - 1;
            currentPath = currentPath.isEmpty() ? parts.at(i) : currentPath + "/" + parts.at(i);

            QTreeWidgetItem* child = nullptr;
            for (int childIndex = 0; childIndex < parent->childCount(); ++childIndex) {
                QTreeWidgetItem* existing = parent->child(childIndex);
                if (existing->text(0) == parts.at(i)) {
                    child = existing;
                    break;
                }
            }

            if (!child) {
                child = new QTreeWidgetItem(parent, QStringList() << parts.at(i));
                child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
                child->setCheckState(0, Qt::Checked);
            }

            child->setData(0, FilePathRole, currentPath);
            child->setData(0, IsDirectoryRole, !isLast);
            child->setToolTip(0, currentPath);
            parent = child;
        }
    }

    m_fileTree->expandAll();
    m_updatingFileTree = false;
}

void MainWindow::setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state) {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        child->setCheckState(0, state);
        setTreeChildrenCheckState(child, state);
    }
}

void MainWindow::updateParentCheckState(QTreeWidgetItem* item) {
    QTreeWidgetItem* parent = item->parent();
    while (parent) {
        int checkedCount = 0;
        int partialCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            const Qt::CheckState state = parent->child(i)->checkState(0);
            if (state == Qt::Checked)
                ++checkedCount;
            else if (state == Qt::PartiallyChecked)
                ++partialCount;
        }

        if (checkedCount == parent->childCount()) {
            parent->setCheckState(0, Qt::Checked);
        } else if (checkedCount == 0 && partialCount == 0) {
            parent->setCheckState(0, Qt::Unchecked);
        } else {
            parent->setCheckState(0, Qt::PartiallyChecked);
        }

        parent = parent->parent();
    }
}

void MainWindow::refreshChangeSummary() {
    QString html = "<h2>版本概览</h2>";
    if (m_projectDir.isEmpty()) {
        html += "<p>尚未打开项目。</p>";
        m_changeSummary->setHtml(html);
        return;
    }

    const QString commitHash = selectedCommitHash();
    const QString versionTitle = m_versionTree->currentItem() ? m_versionTree->currentItem()->text(0) : QString("当前工作区");
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.feedbacksForCommit(feedbackContextHash());
    const int fileCount = commitHash.isEmpty() ? m_files.size() : m_gitService.filesAtCommit(commitHash).size();

    html += "<h3>信息概览</h3>";
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%;\">";
    html += QString("<tr><td style=\"color:#64748b; width:110px;\">版本</td><td><b>%1</b></td></tr>").arg(htmlEscape(versionTitle));
    html += QString("<tr><td style=\"color:#64748b;\">文件数量</td><td>%1</td></tr>").arg(fileCount);
    html += QString("<tr><td style=\"color:#64748b;\">反馈记录</td><td>%1 条</td></tr>").arg(feedbacks.size());

    if (commitHash.isEmpty()) {
        const QString status = m_gitService.statusPorcelain().trimmed();
        const QString diffStat = m_gitService.diffStat().trimmed();
        const QString diff = m_gitService.diff().trimmed();

        html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>当前工作区</td></tr>");
        const QString head = m_gitService.currentHead();
        html += QString("<tr><td style=\"color:#64748b;\">当前 HEAD</td><td>%1</td></tr>")
                    .arg(head.isEmpty() ? "尚无提交" : QString("<code>%1</code>").arg(htmlEscape(head)));
        html += QString("<tr><td style=\"color:#64748b;\">Git 状态</td><td>%1</td></tr>")
                    .arg(htmlEscape(status.isEmpty() ? "工作区干净，没有未提交修改。" : status));
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">变更统计</td><td>%1</td></tr>")
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += "<h3>相对上一版本的变更</h3>";
        html += renderReadableDiff(diff);
    } else {
        GitCommit selectedCommit;
        for (const GitCommit& commit : m_commits) {
            if (commit.hash == commitHash) {
                selectedCommit = commit;
                break;
            }
        }
        const QString diffStat = m_gitService.diffStatForCommit(commitHash).trimmed();
        const QString diff = m_gitService.diffForCommit(commitHash).trimmed();

        html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>历史提交</td></tr>");
        html += QString("<tr><td style=\"color:#64748b;\">提交信息</td><td>%1</td></tr>").arg(htmlEscape(selectedCommit.subject));
        html += QString("<tr><td style=\"color:#64748b;\">提交日期</td><td>%1</td></tr>").arg(htmlEscape(selectedCommit.date));
        const QStringList branches = m_gitService.branchesContainingCommit(commitHash);
        html += QString("<tr><td style=\"color:#64748b;\">所在分支</td><td>%1</td></tr>")
                    .arg(htmlEscape(branches.isEmpty() ? "未找到本地分支引用" : branches.join(", ")));
        html += QString("<tr><td style=\"color:#64748b;\">Commit Hash</td><td><code>%1</code></td></tr>").arg(htmlEscape(commitHash));
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">变更统计</td><td>%1</td></tr>")
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += "<h3>相对上一版本的变更</h3>";
        html += renderReadableDiff(diff);
    }
    m_changeSummary->setHtml(html);
}

void MainWindow::refreshGitState() {
    m_commits = m_gitService.log();
    rebuildVersionTree();
    populateFileTree();
    refreshChangeSummary();
}

void MainWindow::rebuildVersionTree() {
    m_versionRoot->takeChildren();
    if (m_projectDir.isEmpty()) {
        appendVersionNode("尚未打开项目", "请选择一个作业项目文件夹。");
    } else {
        appendVersionNode("当前工作区", "当前磁盘上的项目文件和未提交变更。");

        for (const GitCommit& commit : m_commits) {
            const QString baseTitle = commit.subject.trimmed().isEmpty() ? "未命名提交" : commit.subject.trimmed();
            const QStringList branches = m_gitService.branchesContainingCommit(commit.hash);
            const QString branchHint = branches.isEmpty() ? "无分支引用" : branches.first();
            const QString title = QString("%1  |  %2  |  %3").arg(baseTitle, commit.date, branchHint);

            QString note = QString("日期：%1\nCommit：%2").arg(commit.date, commit.hash);
            if (!branches.isEmpty())
                note += QString("\n所在分支：%1").arg(branches.join(", "));
            appendVersionNode(title, note, commit.hash);
        }
    }

    m_versionTree->expandAll();
    if (m_versionRoot->childCount() > 0)
        m_versionTree->setCurrentItem(m_versionRoot->child(0));
}

void MainWindow::appendVersionNode(const QString& title, const QString& note, const QString& commitHash) {
    auto* item = new QTreeWidgetItem(QStringList() << title);
    item->setData(0, NoteRole, note);
    item->setData(0, CommitHashRole, commitHash);
    m_versionRoot->addChild(item);
}

QList<CodeFile> MainWindow::selectedFiles() const {
    QList<CodeFile> files;
    const QString commitHash = selectedCommitHash();
    const QStringList paths = checkedFilePaths();
    for (const QString& path : paths) {
        if (commitHash.isEmpty()) {
            const QString absolutePath = QDir(m_projectDir).filePath(path);
            for (const CodeFile& file : m_files) {
                if (file.absolutePath == absolutePath || file.relativePath == path) {
                    files.append(file);
                    break;
                }
            }
            continue;
        }

        CodeFile file;
        file.relativePath = path;
        file.absolutePath = QDir(m_projectDir).filePath(path);
        file.extension = extensionForPath(path);
        file.content = m_gitService.fileContentAtCommit(commitHash, path);
        files.append(file);
    }
    return files;
}

QStringList MainWindow::checkedFilePaths() const {
    QStringList paths;
    auto collect = [&](auto&& self, QTreeWidgetItem* item) -> void {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child->data(0, IsDirectoryRole).toBool()) {
                self(self, child);
            } else if (child->checkState(0) == Qt::Checked) {
                paths.append(child->data(0, FilePathRole).toString());
            }
        }
    };

    collect(collect, m_fileTree->invisibleRootItem());
    return paths;
}

QString MainWindow::selectedCommitHash() const {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item || item == m_versionRoot)
        return QString();
    return item->data(0, CommitHashRole).toString();
}

QString MainWindow::feedbackContextHash() const {
    const QString commitHash = selectedCommitHash();
    return commitHash.isEmpty() ? currentWorkContextHash() : commitHash;
}

void MainWindow::selectAllFiles() {
    if (!m_fileTree)
        return;

    m_updatingFileTree = true;
    QTreeWidgetItem* root = m_fileTree->invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* item = root->child(i);
        item->setCheckState(0, Qt::Checked);
        setTreeChildrenCheckState(item, Qt::Checked);
    }
    m_updatingFileTree = false;
}

void MainWindow::handleFileItemChanged(QTreeWidgetItem* item, int column) {
    if (!item || column != 0 || m_updatingFileTree)
        return;

    m_updatingFileTree = true;
    if (item->data(0, IsDirectoryRole).toBool())
        setTreeChildrenCheckState(item, item->checkState(0));
    updateParentCheckState(item);
    m_updatingFileTree = false;
}

QString MainWindow::currentWorkContextHash() const {
    const QString head = m_gitService.currentHead();
    if (head.isEmpty() || !m_gitService.statusPorcelain().trimmed().isEmpty())
        return "working-tree";
    return head;
}

QString MainWindow::currentFeedbackHistory() const {
    QString history;
    const QString commitHash = feedbackContextHash();
    const QList<FeedbackRecord> records = m_feedbackStore.feedbacksForCommit(commitHash);
    for (const FeedbackRecord& record : records) {
        history += QString("### %1 %2\n%3\n%4\n\n").arg(record.createdAt, record.mode, record.summary, record.rawContent);
    }
    return history.trimmed();
}

FeedbackRecord MainWindow::buildFeedbackRecord(const QString& replyText) const {
    FeedbackRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.commitHash = feedbackContextHash();
    record.mode = currentModeName();
    record.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    record.rawContent = replyText;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(extractJsonObjectText(replyText).toUtf8(), &parseError);
    if (!document.isObject()) {
        record.parseStatus = "raw";
        record.summary = "未能解析结构化 JSON，已保存原始回复。";
        return record;
    }

    const QJsonObject object = document.object();
    record.parseStatus = "parsed";
    record.summary = object["summary"].toString("AI 已返回结构化反馈。");
    record.rawContent = object["rawReport"].toString(replyText);

    const QJsonArray items = object["items"].toArray();
    int index = 1;
    for (const QJsonValue& value : items) {
        if (!value.isObject())
            continue;

        FeedbackItem item = FeedbackItem::fromJson(value.toObject());
        item.id = QString("%1-item-%2").arg(record.id).arg(index++);
        if (item.status.isEmpty())
            item.status = "open";
        if (item.severity.isEmpty())
            item.severity = "medium";
        if (item.category.isEmpty())
            item.category = "other";
        record.items.append(item);
    }

    return record;
}

void MainWindow::populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks) {
    m_feedbackTree->clear();

    if (feedbacks.isEmpty()) {
        m_reviewReport->setHtml("<h2 style=\"margin:0 0 8px 0; color:#111827;\">反馈记录</h2><p style=\"color:#64748b;\">当前版本还没有保存反馈记录。</p>");
        return;
    }

    for (const FeedbackRecord& feedback : feedbacks) {
        const QString title = QString("%1  |  %2").arg(feedback.createdAt, feedback.summary.isEmpty() ? "反馈记录" : feedback.summary);
        auto* recordItem = new QTreeWidgetItem(m_feedbackTree, QStringList() << title << QString("%1 项").arg(feedback.items.size()) << feedback.mode << feedback.parseStatus);
        recordItem->setData(0, DetailHtmlRole, renderFeedbackRecordDetail(feedback));

        for (const FeedbackItem& item : feedback.items) {
            const QString location = item.filePath.isEmpty()
                                         ? QString()
                                         : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
            auto* itemNode = new QTreeWidgetItem(recordItem, QStringList() << (item.title.isEmpty() ? "未命名问题" : item.title) << item.severity << location << item.status);
            itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
        }
    }

    m_feedbackTree->expandAll();
    for (int column = 1; column < m_feedbackTree->columnCount(); ++column)
        m_feedbackTree->resizeColumnToContents(column);

    if (m_feedbackTree->topLevelItemCount() > 0)
        m_feedbackTree->setCurrentItem(m_feedbackTree->topLevelItem(0));
}

void MainWindow::handleFeedbackTreeSelection(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current)
        return;

    const QString detailHtml = current->data(0, DetailHtmlRole).toString();
    if (!detailHtml.isEmpty())
        m_reviewReport->setHtml(detailHtml);
}

QString MainWindow::currentModePrompt() const {
    return
        "你是一位严谨、耐心的程序设计课助教。"
        "用户会提供作业要求、代码文件、当前 Git 变更和补充问题。"
        "请聚焦于这次作业代码本身，分析可能的 Bug、边界条件、代码质量和实现建议。"
        "如果信息不足，请明确说明需要补充什么。";
}

QString MainWindow::currentModeName() const {
    return "文件分析";
}

double MainWindow::currentTemperature() const {
    return 0.3;
}

void MainWindow::startAiAnalysis() {
    QList<CodeFile> filesForAnalysis;
    if (m_includeCodeCheck->isChecked()) {
        filesForAnalysis = selectedFiles();
        if (filesForAnalysis.isEmpty()) {
            QMessageBox::information(this, "没有可分析的文件", "请先打开项目并勾选至少一个文件，或取消“包含勾选的代码文件”。");
            return;
        }

        bool hasReadableContent = false;
        for (const CodeFile& file : filesForAnalysis) {
            if (!file.content.isEmpty()) {
                hasReadableContent = true;
                break;
            }
        }
        if (!hasReadableContent) {
            QMessageBox::warning(this, "文件内容为空", "已勾选文件，但没有读取到任何文件内容。请重新扫描项目，或切换到其它提交后再试。");
            return;
        }
    }

    const QString apiKey = qEnvironmentVariable("DEEPSEEK_API_KEY");
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "缺少 API Key", "请先设置环境变量 DEEPSEEK_API_KEY，再使用文件分析功能。");
        return;
    }

    QString userContent;
    const QString task = m_taskEdit->toPlainText().trimmed();
    const QString question = m_questionEdit->toPlainText().trimmed();
    if (!question.isEmpty()) {
        userContent += "【用户问题】\n" + question + "\n\n";
    }

    m_projectManager.data().assignmentText = task;
    QString errorMessage;
    m_projectManager.save(&errorMessage);

    if (m_includeHistoryCheck->isChecked()) {
        const QString feedback = currentFeedbackHistory();
        if (!feedback.isEmpty()) {
            userContent += "【当前版本历史反馈记录】\n" + feedback + "\n\n";
        }
    }

    const QString selectedCommit = selectedCommitHash();
    const QString gitDiff = selectedCommit.isEmpty() ? m_gitService.diff().trimmed() : m_gitService.diffForCommit(selectedCommit).trimmed();
    if (!gitDiff.isEmpty()) {
        userContent += selectedCommit.isEmpty() ? "【当前 Git 变更 diff】\n" : "【当前提交相对于上一个提交的 diff】\n";
        userContent += gitDiff + "\n\n";
    }

    if (selectedCommit.isEmpty()) {
        const QString gitStatus = m_gitService.statusPorcelain().trimmed();
        if (!gitStatus.isEmpty()) {
            userContent += "【当前 Git status】\n";
            userContent += gitStatus + "\n\n";
        }
    }

    if (m_includeCodeCheck->isChecked()) {
        userContent += HWFileScanner::formatFilesForLLM(filesForAnalysis, task);
    } else if (!task.isEmpty()) {
        userContent += "【作业要求 / 任务描述】\n" + task + "\n\n";
    }

    if (userContent.trimmed().isEmpty()) {
        QMessageBox::information(this, "内容为空", "请填写问题、作业要求，或勾选代码文件。");
        return;
    }

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = currentModePrompt() + "\n\n" + structuredFeedbackInstruction();
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userContent;
    messages.append(userMsg);

    if (m_llm)
        m_llm->deleteLater();
    m_llm = new HWpilotLLM(apiKey, this);

    connect(m_llm, &HWpilotLLM::responseReceived, this, [this](const QString& replyText) {
        m_responseView->setPlainText(replyText);
        setBusy(false);
        m_statusLabel->setText("文件分析完成");
    });
    connect(m_llm, &HWpilotLLM::errorOccurred, this, [this](const QString& errorString) {
        m_responseView->setPlainText("网络/API 错误：\n" + errorString);
        setBusy(false);
        m_statusLabel->setText("文件分析失败");
    });

    setBusy(true);
    m_responseView->setPlainText("正在向 DeepSeek 发送请求，请稍候...");
    m_llm->sendChatRequest(messages, currentTemperature());
}

void MainWindow::saveFeedbackToVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const QString feedback = m_responseView->toPlainText().trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(this, "没有反馈", "当前没有可保存的 AI 回复。");
        return;
    }

    FeedbackRecord record = buildFeedbackRecord(feedback);

    QString errorMessage;
    if (!m_feedbackStore.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(this, "保存失败", errorMessage);
        return;
    }

    m_statusLabel->setText("已保存为一条新的反馈记录");
    m_responseView->clear();
    m_questionEdit->clear();
    m_responseView->setPlaceholderText("已保存为反馈记录。新的文件分析回复会显示在这里。");

    refreshProjectPanel();
    updateCurrentVersionPanel();
}

void MainWindow::updateCurrentVersionPanel() {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item)
        return;

    populateFileTree();
    refreshChangeSummary();

    const QString commitHash = feedbackContextHash();
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.feedbacksForCommit(commitHash);
    QString html;
    html += QString("<h2>%1</h2>").arg(htmlEscape(item->text(0)));
    html += QString("<p>%1</p>").arg(htmlEscape(item->data(0, NoteRole).toString()));
    html += "<hr>";
    m_reviewReport->setHtml(html);
    populateFeedbackPanel(feedbacks);
}

void MainWindow::setBusy(bool busy) {
    m_analyzeButton->setDisabled(busy);
    m_saveFeedbackButton->setDisabled(busy);
    m_commitButton->setDisabled(busy);
    m_statusLabel->setText(busy ? "正在进行文件分析..." : "就绪");
}
