#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "HWpilotLLM/HWpilotLLM.h"
#include "HWpilotLLM/Prompts.h"

namespace {
constexpr int FeedbackRole = Qt::UserRole + 1;
constexpr int NoteRole = Qt::UserRole + 2;

QString htmlEscape(const QString& text) {
    QString escaped = text.toHtmlEscaped();
    return escaped.replace('\n', "<br>");
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    applyStyle();
    refreshOverview();
}

void MainWindow::buildUi() {
    setWindowTitle("HWpilot - AI 作业辅助系统");
    resize(1320, 820);

    buildToolBar();

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
    leftLayout->addWidget(m_projectTree);
    leftLayout->addWidget(m_versionTree, 1);

    auto* centerPanel = new QWidget(splitter);
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    buildCenterPanel();
    centerLayout->addWidget(m_tabs);

    auto* aiPanel = new QWidget(splitter);
    auto* aiLayout = new QVBoxLayout(aiPanel);
    aiLayout->setContentsMargins(0, 0, 0, 0);
    buildAiPanel();
    aiLayout->addWidget(m_modeCombo);
    aiLayout->addWidget(m_taskEdit);
    aiLayout->addWidget(m_questionEdit);
    aiLayout->addWidget(m_includeCodeCheck);
    aiLayout->addWidget(m_includeHistoryCheck);
    aiLayout->addWidget(m_analyzeButton);
    aiLayout->addWidget(m_responseView, 1);
    aiLayout->addWidget(m_saveFeedbackButton);

    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->addWidget(aiPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({260, 680, 380});

    rootLayout->addWidget(splitter);
    setCentralWidget(root);

    m_statusLabel = new QLabel("请选择一个作业项目文件夹", this);
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::buildToolBar() {
    auto* toolbar = addToolBar("主工具栏");
    toolbar->setMovable(false);

    auto* openAction = toolbar->addAction("打开项目");
    connect(openAction, &QAction::triggered, this, &MainWindow::openProjectFolder);

    auto* scanAction = toolbar->addAction("扫描文件");
    connect(scanAction, &QAction::triggered, this, &MainWindow::scanCurrentProject);

    auto* submitAction = toolbar->addAction("提交版本");
    connect(submitAction, &QAction::triggered, this, &MainWindow::submitVersion);

    auto* aiAction = toolbar->addAction("AI 分析");
    connect(aiAction, &QAction::triggered, this, &MainWindow::startAiAnalysis);
}

void MainWindow::buildLeftPanel() {
    m_projectTree = new QTreeWidget(this);
    m_projectTree->setHeaderLabel("作业项目");
    m_projectRoot = new QTreeWidgetItem(QStringList() << "尚未打开项目");
    m_projectTree->addTopLevelItem(m_projectRoot);
    m_projectTree->expandAll();

    m_versionTree = new QTreeWidget(this);
    m_versionTree->setHeaderLabel("版本树");
    m_versionRoot = new QTreeWidgetItem(QStringList() << "当前作业");
    m_versionRoot->setData(0, NoteRole, "版本控制入口占位，后续可接入真实 git/树接口。");
    m_versionTree->addTopLevelItem(m_versionRoot);
    appendVersionNode("v1 初始版本", "打开项目后可以从这里查看和提交版本。");
    m_versionTree->expandAll();
    m_versionTree->setCurrentItem(m_versionRoot->child(0));
    connect(m_versionTree, &QTreeWidget::currentItemChanged, this, &MainWindow::updateCurrentVersionPanel);
}

void MainWindow::buildCenterPanel() {
    m_tabs = new QTabWidget(this);

    m_overview = new QTextBrowser(this);
    m_fileList = new QListWidget(this);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_changeSummary = new QTextBrowser(this);
    m_history = new QTextBrowser(this);
    m_reviewReport = new QTextBrowser(this);

    m_tabs->addTab(m_overview, "概览");
    m_tabs->addTab(m_fileList, "文件");
    m_tabs->addTab(m_changeSummary, "变更");
    m_tabs->addTab(m_history, "提交记录");
    m_tabs->addTab(m_reviewReport, "复习报告");
}

void MainWindow::buildAiPanel() {
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItems({"代码批改", "Bug 检查", "启发式引导", "学习模式", "复习总结"});

    m_taskEdit = new QTextEdit(this);
    m_taskEdit->setPlaceholderText("作业要求 / 评分标准 / 课程资料摘要");
    m_taskEdit->setFixedHeight(118);

    m_questionEdit = new QTextEdit(this);
    m_questionEdit->setPlaceholderText("想问 AI 的具体问题，可留空");
    m_questionEdit->setFixedHeight(86);

    m_includeCodeCheck = new QCheckBox("包含勾选的代码文件", this);
    m_includeCodeCheck->setChecked(true);

    m_includeHistoryCheck = new QCheckBox("包含当前版本的历史反馈", this);
    m_includeHistoryCheck->setChecked(true);

    m_analyzeButton = new QPushButton("开始分析", this);
    connect(m_analyzeButton, &QPushButton::clicked, this, &MainWindow::startAiAnalysis);

    m_responseView = new QTextEdit(this);
    m_responseView->setReadOnly(true);
    m_responseView->setPlaceholderText("AI 回复会显示在这里。");

    m_saveFeedbackButton = new QPushButton("保存反馈到当前版本", this);
    connect(m_saveFeedbackButton, &QPushButton::clicked, this, &MainWindow::saveFeedbackToVersion);
}

void MainWindow::applyStyle() {
    qApp->setStyleSheet(
        "QMainWindow, QWidget { background: #f6f7f9; color: #1f2933; font-size: 14px; }"
        "QToolBar { background: #ffffff; border-bottom: 1px solid #d9dee7; spacing: 8px; padding: 6px; }"
        "QToolButton, QPushButton { background: #ffffff; border: 1px solid #cbd3df; border-radius: 6px; padding: 7px 12px; }"
        "QToolButton:hover, QPushButton:hover { background: #eef4ff; border-color: #8fb5ff; }"
        "QPushButton:disabled { color: #8b97a7; background: #eef1f5; }"
        "QTreeWidget, QListWidget, QTextEdit, QTextBrowser, QComboBox { background: #ffffff; border: 1px solid #d9dee7; border-radius: "
        "6px; }"
        "QTreeWidget::item, QListWidget::item { padding: 5px; }"
        "QTreeWidget::item:selected, QListWidget::item:selected { background: #dbeafe; color: #102a43; }"
        "QTabWidget::pane { border: 1px solid #d9dee7; background: #ffffff; border-radius: 6px; }"
        "QTabBar::tab { background: #eef1f5; padding: 8px 14px; border: 1px solid #d9dee7; border-bottom: none; }"
        "QTabBar::tab:selected { background: #ffffff; }"
        "QStatusBar { background: #ffffff; border-top: 1px solid #d9dee7; }");
}

void MainWindow::openProjectFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, "选择作业项目文件夹", m_projectDir);
    if (folder.isEmpty())
        return;

    setProjectFolder(folder);
    scanCurrentProject();
}

void MainWindow::setProjectFolder(const QString& folderPath) {
    m_projectDir = QDir(folderPath).absolutePath();
    const QString name = QFileInfo(m_projectDir).fileName();

    m_projectRoot->setText(0, name.isEmpty() ? m_projectDir : name);
    m_projectRoot->takeChildren();
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << "文件"));
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << "AI 反馈"));
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << "复习报告"));
    m_projectTree->expandAll();

    m_versionRoot->setText(0, name.isEmpty() ? "当前作业" : name);
    m_versionRoot->takeChildren();
    appendVersionNode("v1 初始扫描", "打开项目并建立第一版视图。");
    m_versionTree->expandAll();
    m_versionTree->setCurrentItem(m_versionRoot->child(0));
}

void MainWindow::scanCurrentProject() {
    if (m_projectDir.isEmpty()) {
        openProjectFolder();
        return;
    }

    m_statusLabel->setText("正在扫描项目文件...");
    m_files = HWFileScanner::scanDirectory(m_projectDir);
    populateFileList();
    refreshOverview();
    refreshChangeSummary();
    refreshVersionHistory();
    m_statusLabel->setText(QString("已扫描 %1 个代码/文本文件").arg(m_files.size()));

    if (m_files.isEmpty()) {
        QMessageBox::warning(this, "未找到文件", "没有扫描到支持的代码文件，请检查项目目录。");
    }
}

void MainWindow::populateFileList() {
    m_fileList->clear();
    for (const CodeFile& file : m_files) {
        auto* item = new QListWidgetItem(file.relativePath, m_fileList);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, file.absolutePath);
        item->setToolTip(file.absolutePath);
    }
}

void MainWindow::refreshOverview() {
    const QString projectName = m_projectDir.isEmpty() ? "未打开项目" : QFileInfo(m_projectDir).fileName();
    QTreeWidgetItem* version = m_versionTree ? m_versionTree->currentItem() : nullptr;
    const QString versionName = version ? version->text(0) : "无";

    QString html;
    html += "<h2>项目概览</h2>";
    html += QString("<p><b>当前项目：</b>%1</p>").arg(htmlEscape(projectName));
    html += QString("<p><b>项目路径：</b>%1</p>").arg(htmlEscape(m_projectDir.isEmpty() ? "尚未选择" : m_projectDir));
    html += QString("<p><b>当前版本：</b>%1</p>").arg(htmlEscape(versionName));
    html += QString("<p><b>已扫描文件：</b>%1 个</p>").arg(m_files.size());
    html += "<hr>";
    html += "<p>建议工作流：打开项目 -> 扫描文件 -> 勾选参与分析的文件 -> AI 分析 -> 修改代码 -> 提交新版本。</p>";
    html += "<p>左侧版本树用于承载你队友的版本控制功能；右侧 AI 反馈可以保存到当前版本节点。</p>";
    m_overview->setHtml(html);
}

void MainWindow::refreshChangeSummary() {
    QString html = "<h2>变更视图</h2>";
    html += "<p>这里预留给版本控制模块展示当前版本与上一版本的差异。</p>";
    html += "<ul>";
    for (const CodeFile& file : m_files) {
        html += QString("<li><b>%1</b> - 已纳入项目文件视图</li>").arg(htmlEscape(file.relativePath));
    }
    html += "</ul>";
    m_changeSummary->setHtml(html);
}

void MainWindow::refreshVersionHistory() {
    QString html = "<h2>提交记录</h2><ul>";
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        html += QString("<li><b>%1</b><br>%2</li>").arg(htmlEscape(item->text(0))).arg(htmlEscape(item->data(0, NoteRole).toString()));
    }
    html += "</ul>";
    m_history->setHtml(html);
}

void MainWindow::appendVersionNode(const QString& title, const QString& note) {
    auto* item = new QTreeWidgetItem(QStringList() << title);
    item->setData(0, NoteRole, note);
    m_versionRoot->addChild(item);
}

void MainWindow::submitVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const int next = m_versionRoot->childCount() + 1;
    const QString title = QString("v%1 %2").arg(next).arg(QDateTime::currentDateTime().toString("MM-dd HH:mm"));
    const QString note = QString("手动提交，当前扫描文件 %1 个。").arg(m_files.size());
    appendVersionNode(title, note);
    m_versionTree->expandAll();
    m_versionTree->setCurrentItem(m_versionRoot->child(m_versionRoot->childCount() - 1));
    refreshOverview();
    refreshVersionHistory();
    m_statusLabel->setText("已在版本树中新增一个版本节点");
}

QList<CodeFile> MainWindow::selectedFiles() const {
    QList<CodeFile> files;
    for (int i = 0; i < m_fileList->count(); ++i) {
        QListWidgetItem* item = m_fileList->item(i);
        if (item->checkState() != Qt::Checked)
            continue;

        const QString absolutePath = item->data(Qt::UserRole).toString();
        for (const CodeFile& file : m_files) {
            if (file.absolutePath == absolutePath) {
                files.append(file);
                break;
            }
        }
    }
    return files;
}

QString MainWindow::currentModePrompt() const {
    const QString mode = currentModeName();
    if (mode == "Bug 检查")
        return Prompts::BUG_CHECK;
    if (mode == "启发式引导")
        return Prompts::HEURISTIC;
    if (mode == "学习模式")
        return Prompts::STUDY_MODE;
    if (mode == "复习总结")
        return Prompts::REVIEW_SUMMARY;
    return Prompts::CODE_REVIEW;
}

QString MainWindow::currentModeName() const {
    return m_modeCombo->currentText();
}

double MainWindow::currentTemperature() const {
    const QString mode = currentModeName();
    return (mode == "启发式引导" || mode == "学习模式") ? 0.7 : 0.3;
}

void MainWindow::startAiAnalysis() {
    if (m_includeCodeCheck->isChecked() && selectedFiles().isEmpty()) {
        QMessageBox::information(this, "没有可分析的文件", "请先打开项目并勾选至少一个文件，或取消“包含勾选的代码文件”。");
        return;
    }

    const QString apiKey = qEnvironmentVariable("DEEPSEEK_API_KEY");
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "缺少 API Key", "请先设置环境变量 DEEPSEEK_API_KEY，再使用 AI 分析功能。");
        return;
    }

    QString userContent;
    const QString task = m_taskEdit->toPlainText().trimmed();
    const QString question = m_questionEdit->toPlainText().trimmed();
    if (!question.isEmpty()) {
        userContent += "【用户问题】\n" + question + "\n\n";
    }

    if (m_includeHistoryCheck->isChecked() && m_versionTree->currentItem()) {
        const QString feedback = m_versionTree->currentItem()->data(0, FeedbackRole).toString();
        if (!feedback.isEmpty()) {
            userContent += "【当前版本历史 AI 反馈】\n" + feedback + "\n\n";
        }
    }

    if (m_includeCodeCheck->isChecked()) {
        userContent += HWFileScanner::formatFilesForLLM(selectedFiles(), task);
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
    systemMsg["content"] = currentModePrompt();
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
        m_statusLabel->setText("AI 分析完成");
    });
    connect(m_llm, &HWpilotLLM::errorOccurred, this, [this](const QString& errorString) {
        m_responseView->setPlainText("网络/API 错误：\n" + errorString);
        setBusy(false);
        m_statusLabel->setText("AI 分析失败");
    });

    setBusy(true);
    m_responseView->setPlainText("正在向 DeepSeek 发送请求，请稍候...");
    m_llm->sendChatRequest(messages, currentTemperature());
}

void MainWindow::saveFeedbackToVersion() {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item || item == m_versionRoot) {
        QMessageBox::information(this, "请选择版本", "请先在左侧版本树中选择一个具体版本。");
        return;
    }

    const QString feedback = m_responseView->toPlainText().trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(this, "没有反馈", "当前没有可保存的 AI 回复。");
        return;
    }

    item->setData(0, FeedbackRole, feedback);
    item->setText(0, item->text(0).contains("[AI]") ? item->text(0) : item->text(0) + " [AI]");
    updateCurrentVersionPanel();
    m_statusLabel->setText("已将 AI 反馈保存到当前版本节点");
}

void MainWindow::updateCurrentVersionPanel() {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item)
        return;

    const QString feedback = item->data(0, FeedbackRole).toString();
    QString html;
    html += QString("<h2>%1</h2>").arg(htmlEscape(item->text(0)));
    html += QString("<p>%1</p>").arg(htmlEscape(item->data(0, NoteRole).toString()));
    html += "<hr>";
    if (feedback.isEmpty()) {
        html += "<p>当前版本还没有保存 AI 反馈。</p>";
    } else {
        html += "<h3>已保存的 AI 反馈</h3>";
        html += QString("<p>%1</p>").arg(htmlEscape(feedback));
    }
    m_reviewReport->setHtml(html);
    refreshOverview();
}

void MainWindow::setBusy(bool busy) {
    m_analyzeButton->setDisabled(busy);
    m_saveFeedbackButton->setDisabled(busy);
    m_statusLabel->setText(busy ? "AI 正在分析..." : "就绪");
}
