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
#include <QInputDialog>
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
constexpr int CommitHashRole = Qt::UserRole + 3;

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
    m_versionTree->setHeaderLabel("Git 版本");
    m_versionRoot = new QTreeWidgetItem(QStringList() << "当前作业");
    m_versionRoot->setData(0, NoteRole, "打开项目后会显示真实 Git 提交历史。");
    m_versionTree->addTopLevelItem(m_versionRoot);
    appendVersionNode("尚未打开项目", "请选择一个作业项目文件夹。");
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
    m_tabs->addTab(m_reviewReport, "AI 反馈");
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
    QString errorMessage;
    if (!m_projectManager.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(this, "项目初始化失败", errorMessage);
        return;
    }

    m_gitService.setWorkingDirectory(m_projectDir);
    if (!m_gitService.isGitRepo()) {
        const GitCommandResult initResult = m_gitService.initRepo();
        if (!initResult.success) {
            QMessageBox::warning(this, "Git 初始化失败", initResult.stderrText.trimmed());
        }
    }

    const QString name = m_projectManager.data().projectName.isEmpty() ? QFileInfo(m_projectDir).fileName() : m_projectManager.data().projectName;
    m_taskEdit->setPlainText(m_projectManager.data().assignmentText);

    m_projectRoot->setText(0, name.isEmpty() ? m_projectDir : name);
    m_projectRoot->takeChildren();
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << "文件"));
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << "AI 反馈"));
    m_projectRoot->addChild(new QTreeWidgetItem(QStringList() << ".hwpilot 数据"));
    m_projectTree->expandAll();

    m_versionRoot->setText(0, name.isEmpty() ? "当前作业" : name);
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
    html += QString("<p><b>Git 仓库：</b>%1</p>").arg(m_gitService.isGitRepo() ? "已连接" : "未初始化");
    const QString status = m_gitService.statusPorcelain().trimmed();
    html += QString("<p><b>工作区状态：</b>%1</p>").arg(status.isEmpty() ? "干净" : "有未提交修改");
    html += "<hr>";
    html += "<p>建议工作流：打开项目 -> 扫描文件 -> 查看 Git 变更 -> AI 分析 -> 保存反馈 -> 提交真实版本。</p>";
    html += "<p>AI 反馈会保存到项目的 .hwpilot/project.json，并按当前 Git 版本关联。</p>";
    m_overview->setHtml(html);
}

void MainWindow::refreshChangeSummary() {
    QString html = "<h2>变更视图</h2>";
    if (m_projectDir.isEmpty()) {
        html += "<p>尚未打开项目。</p>";
        m_changeSummary->setHtml(html);
        return;
    }

    const QString status = m_gitService.statusPorcelain().trimmed();
    const QString diffStat = m_gitService.diffStat().trimmed();
    const QString diff = m_gitService.diff().trimmed();

    html += "<h3>Git Status</h3>";
    html += QString("<pre>%1</pre>").arg(htmlEscape(status.isEmpty() ? "工作区干净，没有未提交修改。" : status));
    html += "<h3>Diff Stat</h3>";
    html += QString("<pre>%1</pre>").arg(htmlEscape(diffStat.isEmpty() ? "暂无已跟踪文件变更统计。" : diffStat));
    html += "<h3>Diff</h3>";
    html += QString("<pre>%1</pre>").arg(htmlEscape(diff.isEmpty() ? "暂无已跟踪文件内容变更。" : diff));
    m_changeSummary->setHtml(html);
}

void MainWindow::refreshVersionHistory() {
    QString html = "<h2>Git 提交记录</h2>";
    if (m_commits.isEmpty()) {
        html += "<p>当前项目还没有提交。请修改文件后点击“提交版本”。</p>";
        m_history->setHtml(html);
        return;
    }

    html += "<ul>";
    for (const GitCommit& commit : m_commits) {
        const int feedbackCount = m_projectManager.data().feedbacksForCommit(commit.hash).size();
        html += QString("<li><b>%1</b> %2<br>%3<br>AI 反馈：%4 条</li>")
                    .arg(htmlEscape(commit.shortHash))
                    .arg(htmlEscape(commit.subject))
                    .arg(htmlEscape(commit.date))
                    .arg(feedbackCount);
    }
    html += "</ul>";
    m_history->setHtml(html);
}

void MainWindow::refreshGitState() {
    m_commits = m_gitService.log();
    rebuildVersionTree();
    refreshOverview();
    refreshChangeSummary();
    refreshVersionHistory();
}

void MainWindow::rebuildVersionTree() {
    m_versionRoot->takeChildren();
    if (m_projectDir.isEmpty()) {
        appendVersionNode("尚未打开项目", "请选择一个作业项目文件夹。");
    } else if (m_commits.isEmpty()) {
        appendVersionNode("尚无提交", "当前项目已经连接 Git，但还没有真实提交。");
    } else {
        for (const GitCommit& commit : m_commits) {
            const int feedbackCount = m_projectManager.data().feedbacksForCommit(commit.hash).size();
            const QString title = QString("%1  %2%3")
                                      .arg(commit.shortHash)
                                      .arg(commit.subject)
                                      .arg(feedbackCount > 0 ? QString(" [AI:%1]").arg(feedbackCount) : QString());
            const QString note = QString("%1\n%2").arg(commit.date, commit.hash);
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

void MainWindow::submitVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const QString status = m_gitService.statusPorcelain().trimmed();
    if (status.isEmpty()) {
        QMessageBox::information(this, "没有可提交的修改", "当前 Git 工作区是干净的，不需要提交新版本。");
        return;
    }

    bool ok = false;
    const QString defaultMessage = QString("HWpilot checkpoint %1").arg(QDateTime::currentDateTime().toString("MM-dd HH:mm"));
    const QString message = QInputDialog::getText(this, "提交版本", "提交说明：", QLineEdit::Normal, defaultMessage, &ok).trimmed();
    if (!ok || message.isEmpty())
        return;

    m_projectManager.data().assignmentText = m_taskEdit->toPlainText().trimmed();
    QString errorMessage;
    m_projectManager.save(&errorMessage);

    const GitCommandResult addResult = m_gitService.addAll();
    if (!addResult.success) {
        QMessageBox::warning(this, "Git add 失败", addResult.stderrText.trimmed());
        return;
    }

    const GitCommandResult commitResult = m_gitService.commit(message);
    if (!commitResult.success) {
        const QString detail = commitResult.stderrText.trimmed().isEmpty() ? commitResult.stdoutText.trimmed() : commitResult.stderrText.trimmed();
        QMessageBox::warning(this, "Git commit 失败", detail);
        return;
    }

    const QString newHead = m_gitService.currentHead();
    if (!newHead.isEmpty()) {
        for (FeedbackRecord& record : m_projectManager.data().feedbacks) {
            if (record.commitHash == "working-tree")
                record.commitHash = newHead;
        }
        m_projectManager.save(&errorMessage);
    }

    scanCurrentProject();
    refreshGitState();
    m_statusLabel->setText("已提交一个真实 Git 版本");
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

QString MainWindow::selectedCommitHash() const {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item || item == m_versionRoot)
        return m_gitService.currentHead().isEmpty() ? "working-tree" : m_gitService.currentHead();

    const QString selectedHash = item->data(0, CommitHashRole).toString();
    const QString head = m_gitService.currentHead();
    if (!selectedHash.isEmpty())
        return selectedHash;
    return head.isEmpty() ? "working-tree" : head;
}

QString MainWindow::currentWorkContextHash() const {
    const QString head = m_gitService.currentHead();
    if (head.isEmpty() || !m_gitService.statusPorcelain().trimmed().isEmpty())
        return "working-tree";
    return head;
}

QString MainWindow::currentFeedbackHistory() const {
    QString history;
    const QString commitHash = currentWorkContextHash();
    const QList<FeedbackRecord> records = m_projectManager.data().feedbacksForCommit(commitHash);
    for (const FeedbackRecord& record : records) {
        history += QString("### %1 %2\n%3\n\n").arg(record.createdAt, record.mode, record.content);
    }
    return history.trimmed();
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

    m_projectManager.data().assignmentText = task;
    QString errorMessage;
    m_projectManager.save(&errorMessage);

    if (m_includeHistoryCheck->isChecked()) {
        const QString feedback = currentFeedbackHistory();
        if (!feedback.isEmpty()) {
            userContent += "【当前版本历史 AI 反馈】\n" + feedback + "\n\n";
        }
    }

    const QString gitDiff = m_gitService.diff().trimmed();
    if (!gitDiff.isEmpty()) {
        userContent += "【当前 Git 变更 diff】\n";
        userContent += gitDiff + "\n\n";
    }

    const QString gitStatus = m_gitService.statusPorcelain().trimmed();
    if (!gitStatus.isEmpty()) {
        userContent += "【当前 Git status】\n";
        userContent += gitStatus + "\n\n";
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

    FeedbackRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.commitHash = currentWorkContextHash();
    record.mode = currentModeName();
    record.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    record.content = feedback;

    QString errorMessage;
    if (!m_projectManager.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(this, "保存失败", errorMessage);
        return;
    }

    item->setData(0, FeedbackRole, feedback);
    refreshGitState();
    updateCurrentVersionPanel();
    m_statusLabel->setText("已将 AI 反馈保存到 .hwpilot/project.json");
}

void MainWindow::updateCurrentVersionPanel() {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item)
        return;

    const QString commitHash = item->data(0, CommitHashRole).toString().isEmpty() ? "working-tree" : item->data(0, CommitHashRole).toString();
    const QList<FeedbackRecord> feedbacks = m_projectManager.data().feedbacksForCommit(commitHash);
    QString html;
    html += QString("<h2>%1</h2>").arg(htmlEscape(item->text(0)));
    html += QString("<p>%1</p>").arg(htmlEscape(item->data(0, NoteRole).toString()));
    html += "<hr>";
    if (feedbacks.isEmpty()) {
        html += "<p>当前版本还没有保存 AI 反馈。</p>";
    } else {
        html += "<h3>已保存的 AI 反馈</h3>";
        for (const FeedbackRecord& feedback : feedbacks) {
            html += QString("<h4>%1 - %2</h4>").arg(htmlEscape(feedback.createdAt), htmlEscape(feedback.mode));
            html += QString("<p>%1</p>").arg(htmlEscape(feedback.content));
        }
    }
    m_reviewReport->setHtml(html);
    refreshOverview();
}

void MainWindow::setBusy(bool busy) {
    m_analyzeButton->setDisabled(busy);
    m_saveFeedbackButton->setDisabled(busy);
    m_statusLabel->setText(busy ? "AI 正在分析..." : "就绪");
}
