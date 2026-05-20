#include "../MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

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
    projectActionsLayout->addWidget(m_refreshProjectButton);
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

    m_refreshProjectButton = new QPushButton("Refresh", this);
    connect(m_refreshProjectButton, &QPushButton::clicked, this, &MainWindow::refreshCurrentProject);

    m_commitButton = new QPushButton("Commit", this);
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

