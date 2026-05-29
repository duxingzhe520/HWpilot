#include "MainWindowPrivate.h"

#include "../AppText.h"

#include <QApplication>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QtGlobal>
#include <QVBoxLayout>

MainWindowPrivate::MainWindowPrivate(MainWindow* window) : QObject(window), q(window) {}

void MainWindowPrivate::buildUi() {
    q->setWindowTitle(AppText::get("app.title"));
    q->resize(520, 840);

    auto* root = new QWidget(q);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    buildLeftPanel();
    buildAiPanel();
    buildCenterPanel();

    auto* leftPanel = new QWidget(q);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(8);
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

    auto* centerPanel = new QWidget(q);
    auto* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(18, 18, 18, 18);
    centerLayout->addWidget(m_tabs);

    m_rootStack = new QStackedWidget(root);
    m_copilotPanel = buildCopilotPanel();
    m_deepAnalysisPanel = buildDeepAnalysisPanel(leftPanel, centerPanel);
    m_rootStack->addWidget(m_copilotPanel);
    m_rootStack->addWidget(m_deepAnalysisPanel);
    m_rootStack->setCurrentWidget(m_copilotPanel);
    auto* escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), q);
    connect(escapeShortcut, &QShortcut::activated, this, [this]() {
        if (m_rootStack && m_rootStack->currentWidget() == m_deepAnalysisPanel)
            showCopilotPanel();
    });
    rootLayout->addWidget(m_rootStack, 1);
    q->setCentralWidget(root);

    m_statusLabel = new QLabel(AppText::get("label.chooseProjectStart"), q);
    q->statusBar()->addWidget(m_statusLabel, 1);
    refreshProjectPanel();
}

QWidget* MainWindowPrivate::buildCopilotPanel() {
    auto* panel = new QWidget(q);
    panel->setObjectName("CopilotPanel");
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    auto* logo = new QLabel("▱", panel);
    logo->setObjectName("CopilotLogo");
    auto* title = new QLabel("HWpilot 副驾", panel);
    title->setObjectName("CopilotTitle");
    m_copilotProjectLabel = new QLabel(AppText::get("label.chooseProjectStart"), panel);
    m_copilotProjectLabel->setObjectName("CopilotMeta");
    m_copilotProjectLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(logo);
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(m_copilotProjectLabel);
    layout->addLayout(header);

    auto* summaryCard = new QFrame(panel);
    summaryCard->setObjectName("CopilotSummaryCard");
    auto* summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(22, 20, 22, 20);
    summaryLayout->setSpacing(8);
    m_copilotSummaryLabel = new QLabel(panel);
    m_copilotSummaryLabel->setObjectName("CopilotSummaryText");
    m_copilotSummaryLabel->setWordWrap(true);
    m_copilotMetaLabel = new QLabel(panel);
    m_copilotMetaLabel->setObjectName("CopilotMeta");
    m_copilotMetaLabel->setWordWrap(true);
    summaryLayout->addWidget(m_copilotSummaryLabel);
    summaryLayout->addWidget(m_copilotMetaLabel);
    layout->addWidget(summaryCard);

    auto* scrollArea = new QScrollArea(panel);
    scrollArea->setObjectName("CopilotScroll");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto* scrollContent = new QWidget(scrollArea);
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(16);

    auto* todoTitle = new QLabel("行动清单 (TO-DO)", scrollContent);
    todoTitle->setObjectName("CopilotSectionTitle");
    scrollLayout->addWidget(todoTitle);
    m_copilotTodoList = new QWidget(scrollContent);
    m_copilotTodoLayout = new QVBoxLayout(m_copilotTodoList);
    m_copilotTodoLayout->setContentsMargins(0, 0, 0, 0);
    m_copilotTodoLayout->setSpacing(12);
    scrollLayout->addWidget(m_copilotTodoList);

    auto* questionTitle = new QLabel("启发式问题", scrollContent);
    questionTitle->setObjectName("CopilotSectionTitle");
    scrollLayout->addWidget(questionTitle);
    m_copilotQuestionList = new QWidget(scrollContent);
    m_copilotQuestionLayout = new QVBoxLayout(m_copilotQuestionList);
    m_copilotQuestionLayout->setContentsMargins(0, 0, 0, 0);
    m_copilotQuestionLayout->setSpacing(12);
    scrollLayout->addWidget(m_copilotQuestionList);
    scrollLayout->addStretch(1);
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea, 1);

    m_copilotAnalyzeButton = new QPushButton("分析当前版本", panel);
    m_copilotAnalyzeButton->setObjectName("CopilotPrimaryButton");
    connect(m_copilotAnalyzeButton, &QPushButton::clicked, this, &MainWindowPrivate::showAnalysisDialog);
    m_copilotReviewButton = new QPushButton("复查已选反馈", panel);
    m_copilotReviewButton->setObjectName("CopilotSecondaryButton");
    connect(m_copilotReviewButton, &QPushButton::clicked, this, &MainWindowPrivate::showReviewDialog);
    m_copilotHeuristicButton = new QPushButton("生成启发式问题", panel);
    m_copilotHeuristicButton->setObjectName("CopilotSecondaryButton");
    connect(m_copilotHeuristicButton, &QPushButton::clicked, this, &MainWindowPrivate::showHeuristicDialog);
    m_copilotDeepButton = new QPushButton("展开深度分析", panel);
    m_copilotDeepButton->setObjectName("CopilotPrimaryButton");
    connect(m_copilotDeepButton, &QPushButton::clicked, this, &MainWindowPrivate::showDeepAnalysisPanel);
    m_copilotReturnButton = new QPushButton("返回修改", panel);
    m_copilotReturnButton->setObjectName("CopilotSecondaryButton");
    connect(m_copilotReturnButton, &QPushButton::clicked, q, &QWidget::showMinimized);
    m_copilotOpenProjectButton = new QPushButton(AppText::get("button.openFolder"), panel);
    m_copilotOpenProjectButton->setObjectName("CopilotSecondaryButton");
    connect(m_copilotOpenProjectButton, &QPushButton::clicked, this, &MainWindowPrivate::openProjectFolder);

    layout->addWidget(m_copilotAnalyzeButton);
    layout->addWidget(m_copilotReviewButton);
    layout->addWidget(m_copilotHeuristicButton);
    layout->addWidget(m_copilotDeepButton);
    layout->addWidget(m_copilotReturnButton);
    layout->addWidget(m_copilotOpenProjectButton);
    return panel;
}

QWidget* MainWindowPrivate::buildDeepAnalysisPanel(QWidget* leftPanel, QWidget* centerPanel) {
    auto* panel = new QWidget(q);
    panel->setObjectName("DeepAnalysisPanel");
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QWidget(panel);
    header->setObjectName("DeepHeader");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 16, 24, 16);
    auto* logo = new QLabel("▱", header);
    logo->setObjectName("CopilotLogo");
    auto* title = new QLabel("HWpilot 分析工作台", header);
    title->setObjectName("CopilotTitle");
    auto* versionButton = new QPushButton("查看版本记录", header);
    versionButton->setObjectName("CopilotSecondaryButton");
    connect(versionButton, &QPushButton::clicked, this, [this]() {
        if (m_tabs)
            m_tabs->setCurrentIndex(0);
    });
    m_backToCopilotButton = new QPushButton("返回副驾 (Esc)", header);
    m_backToCopilotButton->setObjectName("CopilotSecondaryButton");
    connect(m_backToCopilotButton, &QPushButton::clicked, this, &MainWindowPrivate::showCopilotPanel);
    headerLayout->addWidget(logo);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);
    headerLayout->addWidget(versionButton);
    headerLayout->addWidget(m_backToCopilotButton);
    layout->addWidget(header);

    auto* splitter = new QSplitter(Qt::Horizontal, panel);
    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({300, 980});
    layout->addWidget(splitter, 1);
    return panel;
}

void MainWindowPrivate::buildLeftPanel() {
    m_projectNameLabel = new QLabel(q);
    m_projectNameLabel->setObjectName("ProjectName");
    m_projectPathLabel = new QLabel(q);
    m_projectPathLabel->setObjectName("MetaLabel");
    m_projectPathLabel->setWordWrap(true);
    m_fileCountLabel = new QLabel(q);
    m_fileCountLabel->setObjectName("MetaLabel");
    m_feedbackCountLabel = new QLabel(q);
    m_feedbackCountLabel->setObjectName("MetaLabel");

    m_openProjectButton = new QPushButton(AppText::get("button.openFolder"), q);
    connect(m_openProjectButton, &QPushButton::clicked, this, &MainWindowPrivate::openProjectFolder);

    m_refreshProjectButton = new QPushButton(AppText::get("button.refresh"), q);
    connect(m_refreshProjectButton, &QPushButton::clicked, this, &MainWindowPrivate::refreshCurrentProject);

    m_commitButton = new QPushButton(AppText::get("button.commit"), q);
    connect(m_commitButton, &QPushButton::clicked, this, &MainWindowPrivate::commitCurrentSnapshot);

    m_versionTree = new QTreeWidget(q);
    m_versionTree->setHeaderLabel(AppText::get("label.commits"));
    m_versionTree->setAlternatingRowColors(true);
    m_versionRoot = m_versionTree->invisibleRootItem();
    appendVersionNode(AppText::get("label.noProject"), AppText::get("label.chooseProjectStart"));
    m_versionTree->expandAll();
    m_versionTree->setCurrentItem(m_versionRoot->child(0));
    connect(m_versionTree, &QTreeWidget::currentItemChanged, this, &MainWindowPrivate::updateCurrentVersionPanel);
}

void MainWindowPrivate::buildCenterPanel() {
    m_tabs = new QTabWidget(q);

    m_selectAllFilesButton = new QPushButton(AppText::get("button.selectAll"), q);
    connect(m_selectAllFilesButton, &QPushButton::clicked, this, &MainWindowPrivate::selectAllFiles);

    m_selectAllFeedbackRecordsButton = new QPushButton(AppText::get("button.selectAll"), q);
    connect(m_selectAllFeedbackRecordsButton, &QPushButton::clicked, this, &MainWindowPrivate::selectAllAiFeedbackRecords);

    m_selectAllHeuristicFilesButton = new QPushButton(AppText::get("button.selectAll"), q);
    connect(m_selectAllHeuristicFilesButton, &QPushButton::clicked, this, &MainWindowPrivate::selectAllHeuristicFiles);

    m_fileTree = new QTreeWidget(q);
    m_fileTree->setHeaderLabel(AppText::get("label.projectFiles"));
    m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileTree->setAlternatingRowColors(true);
    connect(m_fileTree, &QTreeWidget::itemChanged, this, &MainWindowPrivate::handleFileItemChanged);

    m_heuristicFileTree = new QTreeWidget(q);
    m_heuristicFileTree->setHeaderLabel(AppText::get("label.projectFiles"));
    m_heuristicFileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_heuristicFileTree->setAlternatingRowColors(true);
    connect(m_heuristicFileTree, &QTreeWidget::itemChanged, this, &MainWindowPrivate::handleHeuristicFileItemChanged);

    m_aiFeedbackTree = new QTreeWidget(q);
    m_aiFeedbackTree->setHeaderLabel(AppText::get("label.feedbackRecords"));
    m_aiFeedbackTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_aiFeedbackTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_aiFeedbackTree->setAlternatingRowColors(true);
    m_aiFeedbackTree->header()->setStretchLastSection(false);
    m_aiFeedbackTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(m_aiFeedbackTree, &QTreeWidget::itemChanged, this, &MainWindowPrivate::handleAiFeedbackItemChanged);

    m_changeSummary = new QTextBrowser(q);
    m_reviewReport = new QTextBrowser(q);
    m_feedbackTree = new QTreeWidget(q);
    m_feedbackTree->setHeaderLabels({AppText::get("label.feedbackIssue"), AppText::get("label.severity"), AppText::get("label.location"), AppText::get("label.status")});
    m_feedbackTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_feedbackTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_feedbackTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_feedbackTree->setAlternatingRowColors(true);
    m_feedbackTree->header()->setStretchLastSection(false);
    m_feedbackTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_feedbackTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_feedbackTree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_feedbackTree->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_feedbackTree->setColumnWidth(1, 110);
    m_feedbackTree->setColumnWidth(2, 240);
    m_feedbackTree->setColumnWidth(3, 126);
    connect(m_feedbackTree, &QTreeWidget::currentItemChanged, this, &MainWindowPrivate::handleFeedbackTreeSelection);
    connect(m_feedbackTree, &QTreeWidget::customContextMenuRequested, this, &MainWindowPrivate::showFeedbackTreeContextMenu);

    auto* versionPanel = new QWidget(q);
    auto* versionLayout = new QVBoxLayout(versionPanel);
    versionLayout->setContentsMargins(0, 0, 0, 0);
    versionLayout->addWidget(m_changeSummary, 1);

    auto* fileAnalysisPanel = new QWidget(q);
    auto* fileAnalysisLayout = new QVBoxLayout(fileAnalysisPanel);
    fileAnalysisLayout->setContentsMargins(0, 0, 0, 0);

    auto* fileAnalysisSplitter = new QSplitter(Qt::Horizontal, fileAnalysisPanel);
    m_aiPickerStack = new QStackedWidget(fileAnalysisSplitter);

    auto* filePickerPanel = new QWidget(m_aiPickerStack);
    auto* filePickerLayout = new QVBoxLayout(filePickerPanel);
    filePickerLayout->setContentsMargins(0, 0, 0, 0);
    filePickerLayout->setSpacing(8);
    filePickerLayout->addWidget(m_selectAllFilesButton);
    filePickerLayout->addWidget(m_fileTree, 1);

    auto* feedbackPickerPanel = new QWidget(m_aiPickerStack);
    auto* feedbackPickerLayout = new QVBoxLayout(feedbackPickerPanel);
    feedbackPickerLayout->setContentsMargins(0, 0, 0, 0);
    feedbackPickerLayout->setSpacing(8);
    feedbackPickerLayout->addWidget(m_selectAllFeedbackRecordsButton);
    feedbackPickerLayout->addWidget(m_aiFeedbackTree, 1);

    m_aiPickerStack->addWidget(filePickerPanel);
    m_aiPickerStack->addWidget(feedbackPickerPanel);
    m_aiPickerStack->setCurrentWidget(filePickerPanel);

    auto* aiPanel = new QWidget(fileAnalysisSplitter);
    auto* aiLayout = new QVBoxLayout(aiPanel);
    aiLayout->setContentsMargins(0, 0, 0, 0);
    aiLayout->setSpacing(8);
    aiLayout->addWidget(m_aiTitleLabel);
    aiLayout->addWidget(m_taskEdit);
    aiLayout->addWidget(m_questionEdit);
    auto* pickerActionsLayout = new QHBoxLayout();
    pickerActionsLayout->setContentsMargins(0, 0, 0, 0);
    pickerActionsLayout->setSpacing(8);
    pickerActionsLayout->addWidget(m_chooseCodeFilesButton);
    pickerActionsLayout->addWidget(m_chooseFeedbackRecordsButton);
    aiLayout->addLayout(pickerActionsLayout);
    aiLayout->addWidget(m_analyzeButton);
    aiLayout->addWidget(m_responseView, 1);
    auto* feedbackActionsLayout = new QHBoxLayout();
    feedbackActionsLayout->setContentsMargins(0, 0, 0, 0);
    feedbackActionsLayout->setSpacing(8);
    feedbackActionsLayout->addWidget(m_saveFeedbackButton);
    feedbackActionsLayout->addWidget(m_cancelFeedbackButton);
    aiLayout->addLayout(feedbackActionsLayout);

    fileAnalysisSplitter->addWidget(m_aiPickerStack);
    fileAnalysisSplitter->addWidget(aiPanel);
    fileAnalysisSplitter->setStretchFactor(0, 1);
    fileAnalysisSplitter->setStretchFactor(1, 1);
    fileAnalysisSplitter->setSizes({360, 540});
    fileAnalysisLayout->addWidget(fileAnalysisSplitter, 1);

    auto* feedbackPanel = new QWidget(q);
    auto* feedbackLayout = new QVBoxLayout(feedbackPanel);
    feedbackLayout->setContentsMargins(0, 0, 0, 0);
    feedbackLayout->setSpacing(8);
    feedbackLayout->addWidget(m_reviewReport, 1);
    m_openReviewRecordButton = new QPushButton(AppText::get("button.openReviewRecord"), q);
    m_openReviewRecordButton->setVisible(false);
    connect(m_openReviewRecordButton, &QPushButton::clicked, this, [this]() {
        selectFeedbackRecordById(m_openReviewRecordButton->property("reviewRecordId").toString());
    });
    feedbackLayout->addWidget(m_openReviewRecordButton);
    auto* feedbackFilterLayout = new QHBoxLayout();
    feedbackFilterLayout->setContentsMargins(0, 0, 0, 0);
    feedbackFilterLayout->setSpacing(8);
    m_feedbackIssuesButton = new QPushButton(AppText::get("feedback.viewIssues"), q);
    m_feedbackIssuesButton->setCheckable(true);
    m_feedbackIssuesButton->setChecked(true);
    connect(m_feedbackIssuesButton, &QPushButton::clicked, this, &MainWindowPrivate::showFeedbackIssueRecords);
    m_feedbackReviewButton = new QPushButton(AppText::get("feedback.viewReviews"), q);
    m_feedbackReviewButton->setCheckable(true);
    connect(m_feedbackReviewButton, &QPushButton::clicked, this, &MainWindowPrivate::showFeedbackReviewRecords);
    m_heuristicQuestionsButton = new QPushButton(AppText::get("feedback.viewHeuristic"), q);
    m_heuristicQuestionsButton->setCheckable(true);
    connect(m_heuristicQuestionsButton, &QPushButton::clicked, this, &MainWindowPrivate::showHeuristicQuestionRecords);
    feedbackFilterLayout->addWidget(m_feedbackIssuesButton);
    feedbackFilterLayout->addWidget(m_feedbackReviewButton);
    feedbackFilterLayout->addWidget(m_heuristicQuestionsButton);
    feedbackLayout->addLayout(feedbackFilterLayout);
    feedbackLayout->addWidget(m_feedbackTree, 1);

    auto* heuristicPanel = new QWidget(q);
    auto* heuristicLayout = new QVBoxLayout(heuristicPanel);
    heuristicLayout->setContentsMargins(0, 0, 0, 0);
    auto* heuristicSplitter = new QSplitter(Qt::Horizontal, heuristicPanel);
    auto* heuristicPickerPanel = new QWidget(heuristicSplitter);
    auto* heuristicPickerLayout = new QVBoxLayout(heuristicPickerPanel);
    heuristicPickerLayout->setContentsMargins(0, 0, 0, 0);
    heuristicPickerLayout->setSpacing(8);
    heuristicPickerLayout->addWidget(m_selectAllHeuristicFilesButton);
    heuristicPickerLayout->addWidget(m_heuristicFileTree, 1);
    auto* heuristicAiPanel = new QWidget(heuristicSplitter);
    auto* heuristicAiLayout = new QVBoxLayout(heuristicAiPanel);
    heuristicAiLayout->setContentsMargins(0, 0, 0, 0);
    heuristicAiLayout->setSpacing(8);
    m_heuristicTitleLabel = new QLabel(AppText::get("label.heuristicQuestions"), q);
    m_heuristicTitleLabel->setObjectName("PanelTitle");
    m_heuristicTaskEdit = new QTextEdit(q);
    m_heuristicTaskEdit->setPlaceholderText(AppText::get("placeholder.task"));
    m_heuristicTaskEdit->setFixedHeight(118);
    m_heuristicQuestionEdit = new QTextEdit(q);
    m_heuristicQuestionEdit->setPlaceholderText(AppText::get("placeholder.question"));
    m_heuristicQuestionEdit->setFixedHeight(86);
    m_generateHeuristicButton = new QPushButton(AppText::get("button.generateHeuristic"), q);
    m_generateHeuristicButton->setObjectName("PrimaryButton");
    connect(m_generateHeuristicButton, &QPushButton::clicked, this, &MainWindowPrivate::startHeuristicQuestions);
    m_heuristicResponseView = new QTextEdit(q);
    m_heuristicResponseView->setReadOnly(true);
    m_heuristicResponseView->setPlaceholderText(AppText::get("placeholder.heuristicReply"));
    m_saveHeuristicButton = new QPushButton(AppText::get("button.saveFeedback"), q);
    connect(m_saveHeuristicButton, &QPushButton::clicked, this, &MainWindowPrivate::saveHeuristicToVersion);
    m_cancelHeuristicButton = new QPushButton(AppText::get("button.cancel"), q);
    connect(m_cancelHeuristicButton, &QPushButton::clicked, this, [this]() {
        m_lastHeuristicReply.clear();
        m_heuristicResponseView->clear();
        m_heuristicResponseView->setPlaceholderText(AppText::get("placeholder.cancelledReply"));
        m_statusLabel->setText(AppText::get("status.cancelledFeedback"));
    });
    heuristicAiLayout->addWidget(m_heuristicTitleLabel);
    heuristicAiLayout->addWidget(m_heuristicTaskEdit);
    heuristicAiLayout->addWidget(m_heuristicQuestionEdit);
    heuristicAiLayout->addWidget(m_generateHeuristicButton);
    heuristicAiLayout->addWidget(m_heuristicResponseView, 1);
    auto* heuristicActionsLayout = new QHBoxLayout();
    heuristicActionsLayout->setContentsMargins(0, 0, 0, 0);
    heuristicActionsLayout->setSpacing(8);
    heuristicActionsLayout->addWidget(m_saveHeuristicButton);
    heuristicActionsLayout->addWidget(m_cancelHeuristicButton);
    heuristicAiLayout->addLayout(heuristicActionsLayout);
    heuristicSplitter->addWidget(heuristicPickerPanel);
    heuristicSplitter->addWidget(heuristicAiPanel);
    heuristicSplitter->setStretchFactor(0, 1);
    heuristicSplitter->setStretchFactor(1, 1);
    heuristicSplitter->setSizes({360, 540});
    heuristicLayout->addWidget(heuristicSplitter, 1);

    m_tabs->addTab(versionPanel, AppText::get("label.versionOverview"));
    m_tabs->addTab(fileAnalysisPanel, AppText::get("label.fileAnalysis"));
    m_tabs->addTab(heuristicPanel, AppText::get("label.heuristicQuestions"));
    m_tabs->addTab(feedbackPanel, AppText::get("label.feedbackRecords"));
}

void MainWindowPrivate::buildAiPanel() {
    m_aiTitleLabel = new QLabel(AppText::get("label.aiAnalysis"), q);
    m_aiTitleLabel->setObjectName("PanelTitle");

    m_taskEdit = new QTextEdit(q);
    m_taskEdit->setPlaceholderText(AppText::get("placeholder.task"));
    m_taskEdit->setFixedHeight(118);

    m_questionEdit = new QTextEdit(q);
    m_questionEdit->setPlaceholderText(AppText::get("placeholder.question"));
    m_questionEdit->setFixedHeight(86);

    m_chooseCodeFilesButton = new QPushButton(AppText::get("button.chooseCodeFiles"), q);
    m_chooseCodeFilesButton->setCheckable(true);
    m_chooseCodeFilesButton->setChecked(true);
    connect(m_chooseCodeFilesButton, &QPushButton::clicked, this, &MainWindowPrivate::showAiCodeFilePicker);

    m_chooseFeedbackRecordsButton = new QPushButton(AppText::get("button.chooseFeedbackRecords"), q);
    m_chooseFeedbackRecordsButton->setCheckable(true);
    connect(m_chooseFeedbackRecordsButton, &QPushButton::clicked, this, &MainWindowPrivate::showAiFeedbackRecordPicker);

    m_analyzeButton = new QPushButton(AppText::get("button.aiAssistant"), q);
    m_analyzeButton->setObjectName("PrimaryButton");
    connect(m_analyzeButton, &QPushButton::clicked, this, &MainWindowPrivate::startAiAnalysis);

    m_responseView = new QTextEdit(q);
    m_responseView->setReadOnly(true);
    m_responseView->setPlaceholderText(AppText::get("placeholder.aiReply"));

    m_saveFeedbackButton = new QPushButton(AppText::get("button.saveFeedback"), q);
    connect(m_saveFeedbackButton, &QPushButton::clicked, this, &MainWindowPrivate::saveFeedbackToVersion);

    m_cancelFeedbackButton = new QPushButton(AppText::get("button.cancel"), q);
    connect(m_cancelFeedbackButton, &QPushButton::clicked, this, [this]() {
        m_lastAiReply.clear();
        m_lastAiModeName.clear();
        m_responseView->clear();
        m_responseView->setPlaceholderText(AppText::get("placeholder.cancelledReply"));
        m_statusLabel->setText(AppText::get("status.cancelledFeedback"));
    });
}

void MainWindowPrivate::buildMenuBar() {
    QSettings settings("HWpilot", "HWpilot");
    m_temperature = settings.value("llm/temperature", 0.3).toDouble();

    q->menuBar()->clear();
    QMenu* fileMenu = q->menuBar()->addMenu(AppText::get("menu.file"));
    fileMenu->addAction(AppText::get("menu.openProject"), this, &MainWindowPrivate::openProjectFolder);
    m_recentProjectsMenu = fileMenu->addMenu(AppText::get("menu.recentProjects"));
    updateRecentProjectsMenu();

    QMenu* settingsMenu = q->menuBar()->addMenu(AppText::get("menu.settings"));

    QMenu* languageMenu = settingsMenu->addMenu(AppText::get("menu.language"));
    auto* languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);
    QAction* chineseAction = languageMenu->addAction(AppText::get("menu.language.zh"));
    QAction* englishAction = languageMenu->addAction(AppText::get("menu.language.en"));
    chineseAction->setCheckable(true);
    englishAction->setCheckable(true);
    languageGroup->addAction(chineseAction);
    languageGroup->addAction(englishAction);
    const QString language = settings.value("ui/language", "zh").toString();
    (language == "en" ? englishAction : chineseAction)->setChecked(true);
    connect(chineseAction, &QAction::triggered, this, [this]() {
        AppText::setLanguage("zh");
        buildMenuBar();
        applyLanguage();
    });
    connect(englishAction, &QAction::triggered, this, [this]() {
        AppText::setLanguage("en");
        buildMenuBar();
        applyLanguage();
    });

    QMenu* themeMenu = settingsMenu->addMenu(AppText::get("menu.theme"));
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    QAction* lightAction = themeMenu->addAction(AppText::get("menu.theme.light"));
    QAction* darkAction = themeMenu->addAction(AppText::get("menu.theme.dark"));
    lightAction->setCheckable(true);
    darkAction->setCheckable(true);
    themeGroup->addAction(lightAction);
    themeGroup->addAction(darkAction);
    const QString theme = settings.value("ui/theme", "light").toString();
    (theme == "dark" ? darkAction : lightAction)->setChecked(true);
    connect(lightAction, &QAction::triggered, this, []() {
        QSettings("HWpilot", "HWpilot").setValue("ui/theme", "light");
    });
    connect(darkAction, &QAction::triggered, this, []() {
        QSettings("HWpilot", "HWpilot").setValue("ui/theme", "dark");
    });

    QMenu* temperatureMenu = settingsMenu->addMenu(AppText::get("menu.temperature"));
    auto* temperatureGroup = new QActionGroup(this);
    temperatureGroup->setExclusive(true);
    const QList<QPair<QString, double>> temperatures = {
        {AppText::get("menu.temperature.strict"), 0.2},
        {AppText::get("menu.temperature.balanced"), 0.3},
        {AppText::get("menu.temperature.creative"), 0.7},
    };
    for (const auto& option : temperatures) {
        QAction* action = temperatureMenu->addAction(option.first);
        action->setCheckable(true);
        action->setData(option.second);
        temperatureGroup->addAction(action);
        if (qAbs(m_temperature - option.second) < 0.001)
            action->setChecked(true);
        connect(action, &QAction::triggered, this, [this, action]() {
            m_temperature = action->data().toDouble();
            QSettings("HWpilot", "HWpilot").setValue("llm/temperature", m_temperature);
        });
    }
    if (!temperatureGroup->checkedAction())
        temperatureGroup->actions().value(1)->setChecked(true);

    const QString aboutMenuTitle = AppText::language() == "en" ? QString("About%1").arg(QChar(0x200B)) : AppText::get("menu.about");
    const QString aboutActionTitle = AppText::language() == "en" ? QString("About HWpilot%1").arg(QChar(0x200B)) : AppText::get("menu.aboutHwPilot");
    QMenu* aboutMenu = q->menuBar()->addMenu(aboutMenuTitle);
    aboutMenu->menuAction()->setMenuRole(QAction::NoRole);
    QAction* aboutAction = aboutMenu->addAction(aboutActionTitle, this, [this]() {
        QDialog dialog(q);
        dialog.setWindowTitle(AppText::get("menu.aboutHwPilot"));
        dialog.setMinimumSize(420, 260);
        dialog.setStyleSheet("QDialog { background: #f8fafc; color: #1f2937; }");
        dialog.exec();
    });
    aboutAction->setMenuRole(QAction::NoRole);
}

void MainWindowPrivate::applyLanguage() {
    q->setWindowTitle(AppText::get("app.title"));
    if (m_openProjectButton)
        m_openProjectButton->setText(AppText::get("button.openFolder"));
    if (m_refreshProjectButton)
        m_refreshProjectButton->setText(AppText::get("button.refresh"));
    if (m_commitButton)
        m_commitButton->setText(AppText::get("button.commit"));
    if (m_selectAllFilesButton)
        m_selectAllFilesButton->setText(AppText::get("button.selectAll"));
    if (m_selectAllFeedbackRecordsButton)
        m_selectAllFeedbackRecordsButton->setText(AppText::get("button.selectAll"));
    if (m_selectAllHeuristicFilesButton)
        m_selectAllHeuristicFilesButton->setText(AppText::get("button.selectAll"));
    if (m_chooseCodeFilesButton)
        m_chooseCodeFilesButton->setText(AppText::get("button.chooseCodeFiles"));
    if (m_chooseFeedbackRecordsButton)
        m_chooseFeedbackRecordsButton->setText(AppText::get("button.chooseFeedbackRecords"));
    if (m_analyzeButton)
        updateAiActionText();
    if (m_saveFeedbackButton)
        m_saveFeedbackButton->setText(AppText::get("button.saveFeedback"));
    if (m_cancelFeedbackButton)
        m_cancelFeedbackButton->setText(AppText::get("button.cancel"));
    if (m_generateHeuristicButton)
        m_generateHeuristicButton->setText(AppText::get("button.generateHeuristic"));
    if (m_saveHeuristicButton)
        m_saveHeuristicButton->setText(AppText::get("button.saveFeedback"));
    if (m_cancelHeuristicButton)
        m_cancelHeuristicButton->setText(AppText::get("button.cancel"));
    if (m_feedbackIssuesButton)
        m_feedbackIssuesButton->setText(AppText::get("feedback.viewIssues"));
    if (m_feedbackReviewButton)
        m_feedbackReviewButton->setText(AppText::get("feedback.viewReviews"));
    if (m_heuristicQuestionsButton)
        m_heuristicQuestionsButton->setText(AppText::get("feedback.viewHeuristic"));
    if (m_openReviewRecordButton)
        m_openReviewRecordButton->setText(AppText::get("button.openReviewRecord"));
    if (m_aiTitleLabel)
        m_aiTitleLabel->setText(AppText::get("label.aiAnalysis"));
    if (m_heuristicTitleLabel)
        m_heuristicTitleLabel->setText(AppText::get("label.heuristicQuestions"));

    if (m_versionTree)
        m_versionTree->setHeaderLabel(AppText::get("label.commits"));
    if (m_fileTree)
        m_fileTree->setHeaderLabel(AppText::get("label.projectFiles"));
    if (m_heuristicFileTree)
        m_heuristicFileTree->setHeaderLabel(AppText::get("label.projectFiles"));
    if (m_aiFeedbackTree)
        m_aiFeedbackTree->setHeaderLabel(AppText::get("label.feedbackRecords"));
    if (m_feedbackTree) {
        if (m_showingHeuristicRecords)
            m_feedbackTree->setHeaderLabels({AppText::get("label.heuristicQuestions")});
        else
            m_feedbackTree->setHeaderLabels({AppText::get("label.feedbackIssue"), AppText::get("label.severity"), AppText::get("label.location"), AppText::get("label.status")});
    }

    if (m_tabs) {
        m_tabs->setTabText(0, AppText::get("label.versionOverview"));
        m_tabs->setTabText(1, AppText::get("label.fileAnalysis"));
        m_tabs->setTabText(2, AppText::get("label.heuristicQuestions"));
        m_tabs->setTabText(3, AppText::get("label.feedbackRecords"));
    }

    if (m_taskEdit)
        m_taskEdit->setPlaceholderText(AppText::get("placeholder.task"));
    if (m_questionEdit)
        m_questionEdit->setPlaceholderText(AppText::get("placeholder.question"));
    if (m_responseView && m_responseView->toPlainText().trimmed().isEmpty())
        m_responseView->setPlaceholderText(AppText::get("placeholder.aiReply"));
    if (m_heuristicTaskEdit)
        m_heuristicTaskEdit->setPlaceholderText(AppText::get("placeholder.task"));
    if (m_heuristicQuestionEdit)
        m_heuristicQuestionEdit->setPlaceholderText(AppText::get("placeholder.question"));
    if (m_heuristicResponseView && m_heuristicResponseView->toPlainText().trimmed().isEmpty())
        m_heuristicResponseView->setPlaceholderText(AppText::get("placeholder.heuristicReply"));

    refreshProjectPanel();
    refreshChangeSummary();
    if (m_aiPickerStack && m_aiPickerStack->currentIndex() == 1)
        populateAiFeedbackPicker();
}

void MainWindowPrivate::updateRecentProjectsMenu() {
    if (!m_recentProjectsMenu)
        return;

    m_recentProjectsMenu->clear();
    const QStringList projects = QSettings("HWpilot", "HWpilot").value("recentProjects").toStringList();
    if (projects.isEmpty()) {
        QAction* emptyAction = m_recentProjectsMenu->addAction(AppText::get("menu.noRecentProjects"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString& project : projects) {
        QAction* action = m_recentProjectsMenu->addAction(project);
        connect(action, &QAction::triggered, this, [this, project]() {
            setProjectFolder(project);
            scanCurrentProject();
        });
    }
}

void MainWindowPrivate::showAiCodeFilePicker() {
    if (m_aiPickerStack)
        m_aiPickerStack->setCurrentIndex(0);
    m_chooseCodeFilesButton->setChecked(true);
    m_chooseFeedbackRecordsButton->setChecked(false);
    updateAiActionText();
}

void MainWindowPrivate::showAiFeedbackRecordPicker() {
    populateAiFeedbackPicker();
    if (m_aiPickerStack)
        m_aiPickerStack->setCurrentIndex(1);
    m_chooseCodeFilesButton->setChecked(false);
    m_chooseFeedbackRecordsButton->setChecked(true);
    updateAiActionText();
}

void MainWindowPrivate::showCopilotPanel() {
    if (m_rootStack && m_copilotPanel)
        m_rootStack->setCurrentWidget(m_copilotPanel);
    q->resize(520, 840);
    refreshCopilotPanel();
}

void MainWindowPrivate::showDeepAnalysisPanel() {
    if (m_rootStack && m_deepAnalysisPanel)
        m_rootStack->setCurrentWidget(m_deepAnalysisPanel);
    q->resize(1320, 860);
    refreshCurrentProject();
}

void MainWindowPrivate::showAnalysisDialog() {
    if (m_projectDir.isEmpty()) {
        openProjectFolder();
        if (m_projectDir.isEmpty())
            return;
    }

    scanCurrentProject(false);

    QDialog dialog(q);
    dialog.setWindowTitle("分析当前版本");
    dialog.setMinimumSize(620, 560);
    dialog.setObjectName("CopilotDialog");
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* title = new QLabel("分析当前版本", &dialog);
    title->setObjectName("DialogTitle");
    layout->addWidget(title);

    auto* scope = new QComboBox(&dialog);
    scope->addItem("智能选择推荐文件", "recommended");
    scope->addItem("分析全部代码文件", "all");
    scope->addItem("手动选择文件", "manual");
    layout->addWidget(scope);

    auto* manualTree = new QTreeWidget(&dialog);
    manualTree->setHeaderLabel(AppText::get("label.projectFiles"));
    manualTree->setVisible(false);
    for (const CodeFile& file : m_files) {
        auto* item = new QTreeWidgetItem(manualTree, QStringList() << file.relativePath);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Checked);
        item->setData(0, FilePathRole, file.relativePath);
    }
    layout->addWidget(manualTree, 1);
    connect(scope, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [scope, manualTree]() {
        manualTree->setVisible(scope->currentData().toString() == "manual");
    });

    auto* task = new QTextEdit(&dialog);
    task->setPlaceholderText(AppText::get("placeholder.task"));
    task->setFixedHeight(110);
    task->setPlainText(m_taskEdit ? m_taskEdit->toPlainText() : QString());
    layout->addWidget(task);

    auto* question = new QTextEdit(&dialog);
    question->setPlaceholderText(AppText::get("placeholder.question"));
    question->setFixedHeight(90);
    layout->addWidget(question);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto* startButton = buttons->addButton("开始分析", QDialogButtonBox::AcceptRole);
    startButton->setObjectName("CopilotPrimaryButton");
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(startButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    QStringList paths;
    if (scope->currentData().toString() == "manual") {
        for (int i = 0; i < manualTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = manualTree->topLevelItem(i);
            if (item->checkState(0) == Qt::Checked)
                paths.append(item->data(0, FilePathRole).toString());
        }
    } else {
        for (const CodeFile& file : m_files)
            paths.append(file.relativePath);
    }

    setCheckedFilePaths(m_fileTree, paths, m_updatingFileTree);
    if (m_aiPickerStack)
        m_aiPickerStack->setCurrentIndex(0);
    if (m_chooseCodeFilesButton)
        m_chooseCodeFilesButton->setChecked(true);
    if (m_chooseFeedbackRecordsButton)
        m_chooseFeedbackRecordsButton->setChecked(false);
    if (m_taskEdit)
        m_taskEdit->setPlainText(task->toPlainText());
    if (m_questionEdit)
        m_questionEdit->setPlainText(question->toPlainText());

    m_autoSaveAiAfterResponse = true;
    startAiAnalysis();
}

void MainWindowPrivate::showReviewDialog() {
    if (m_projectDir.isEmpty()) {
        openProjectFolder();
        if (m_projectDir.isEmpty())
            return;
    }

    const QStringList itemIds = checkedCopilotTodoItemIds();
    if (itemIds.isEmpty()) {
        QMessageBox::information(q, "未选择待办", "请先在行动清单中勾选要复查的反馈。");
        return;
    }

    QDialog dialog(q);
    dialog.setWindowTitle("复查已选反馈");
    dialog.setMinimumSize(560, 360);
    dialog.setObjectName("CopilotDialog");
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);
    auto* title = new QLabel(QString("复查 %1 条反馈").arg(itemIds.size()), &dialog);
    title->setObjectName("DialogTitle");
    layout->addWidget(title);
    auto* hint = new QLabel("系统会自动刷新当前代码，并读取已选反馈对应的上下文。", &dialog);
    hint->setObjectName("CopilotMeta");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* notes = new QTextEdit(&dialog);
    notes->setPlaceholderText("补充说明，例如：我已经修改了析构函数和测试用例...");
    notes->setFixedHeight(120);
    layout->addWidget(notes);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto* startButton = buttons->addButton("开始复查", QDialogButtonBox::AcceptRole);
    startButton->setObjectName("CopilotPrimaryButton");
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(startButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    scanCurrentProject(false);
    QStringList paths;
    for (const CodeFile& file : m_files)
        paths.append(file.relativePath);
    setCheckedFilePaths(m_fileTree, paths, m_updatingFileTree);
    populateAiFeedbackPicker();
    applyFeedbackPickerSelection(itemIds);
    if (m_questionEdit)
        m_questionEdit->setPlainText(notes->toPlainText());
    m_autoSaveAiAfterResponse = true;
    startAiAnalysis();
}

void MainWindowPrivate::showHeuristicDialog() {
    if (m_projectDir.isEmpty()) {
        openProjectFolder();
        if (m_projectDir.isEmpty())
            return;
    }

    scanCurrentProject(false);

    QDialog dialog(q);
    dialog.setWindowTitle("生成启发式问题");
    dialog.setMinimumSize(560, 430);
    dialog.setObjectName("CopilotDialog");
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);
    auto* title = new QLabel("生成启发式问题", &dialog);
    title->setObjectName("DialogTitle");
    layout->addWidget(title);
    auto* task = new QTextEdit(&dialog);
    task->setPlaceholderText(AppText::get("placeholder.task"));
    task->setFixedHeight(110);
    task->setPlainText(m_taskEdit ? m_taskEdit->toPlainText() : QString());
    layout->addWidget(task);
    auto* question = new QTextEdit(&dialog);
    question->setPlaceholderText("额外要求，例如：更偏向引导学生自己发现内存问题...");
    question->setFixedHeight(100);
    layout->addWidget(question);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto* startButton = buttons->addButton("生成问题", QDialogButtonBox::AcceptRole);
    startButton->setObjectName("CopilotPrimaryButton");
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(startButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    QStringList paths;
    for (const CodeFile& file : m_files)
        paths.append(file.relativePath);
    setCheckedFilePaths(m_heuristicFileTree, paths, m_updatingHeuristicFileTree);
    if (m_heuristicTaskEdit)
        m_heuristicTaskEdit->setPlainText(task->toPlainText());
    if (m_heuristicQuestionEdit)
        m_heuristicQuestionEdit->setPlainText(question->toPlainText());
    m_autoSaveHeuristicAfterResponse = true;
    startHeuristicQuestions();
}

void MainWindowPrivate::applyStyle() {
    qApp->setStyleSheet(
        "QMainWindow, QWidget { background: #1c1c1d; color: #e8e8ea; font-family: \"PingFang SC\", \"Helvetica Neue\", Arial, sans-serif; font-size: 14px; line-height: 1.45; }"
        "QWidget#CopilotPanel, QWidget#DeepAnalysisPanel { background: #1c1c1d; }"
        "QWidget#DeepHeader { background: #202123; border-bottom: 1px solid #3a3b3f; }"
        "QLabel#CopilotLogo { color: #0b8bdc; font-size: 28px; font-weight: 800; padding-right: 4px; }"
        "QLabel#CopilotTitle { color: #f4f4f5; font-size: 24px; font-weight: 800; }"
        "QLabel#CopilotMeta, QLabel#MetaLabel { color: #a1a1aa; background: transparent; font-weight: 600; }"
        "QLabel#CopilotSummaryText { color: #e5e7eb; background: transparent; font-size: 21px; font-weight: 800; line-height: 1.45; }"
        "QLabel#CopilotSectionTitle { color: #9b9ba1; background: transparent; font-size: 17px; font-weight: 800; padding: 4px 0; }"
        "QLabel#CopilotEmptyText { color: #a1a1aa; background: #222326; border: 1px solid #3f4045; border-radius: 8px; padding: 14px; }"
        "QFrame#CopilotSummaryCard { background: #28282a; border: 1px solid #44454a; border-radius: 10px; }"
        "QFrame#TodoCardHigh { background: #222224; border: 1px solid #57352e; border-left: 5px solid #ff806d; border-radius: 8px; }"
        "QFrame#TodoCardMedium { background: #222224; border: 1px solid #4e4725; border-left: 5px solid #d8b800; border-radius: 8px; }"
        "QFrame#QuestionCard { background: #1f262d; border: 1px solid #0b84d8; border-left: 5px solid #0b8bdc; border-radius: 8px; }"
        "QLabel#TodoTitle { color: #ffffff; background: transparent; font-size: 17px; font-weight: 800; }"
        "QLabel#TodoBody { color: #b8b8bd; background: transparent; font-size: 15px; font-weight: 600; }"
        "QLabel#TodoTagHigh { color: #ff9a8a; background: #5a3933; border-radius: 5px; padding: 4px 8px; font-weight: 800; }"
        "QLabel#TodoTagMedium { color: #d7d7dc; background: #45484c; border-radius: 5px; padding: 4px 8px; font-weight: 800; }"
        "QLabel#QuestionTitle { color: #0b98ee; background: transparent; font-size: 16px; font-weight: 800; }"
        "QLabel#QuestionBody { color: #e8eef7; background: transparent; font-size: 16px; font-weight: 650; line-height: 1.55; }"
        "QLabel#DialogTitle, QLabel#PanelTitle, QLabel#ProjectName { background: transparent; color: #f4f4f5; font-size: 18px; font-weight: 800; padding: 2px 0 6px 0; }"
        "QPushButton, QToolButton { background: #26272a; border: 1px solid #46474d; border-radius: 8px; padding: 10px 14px; color: #d9d9dd; font-weight: 700; }"
        "QPushButton:hover, QToolButton:hover { background: #303137; border-color: #5b5c63; }"
        "QPushButton:pressed, QToolButton:pressed { background: #18191b; }"
        "QPushButton#PrimaryButton, QPushButton#CopilotPrimaryButton { background: #0b8bdc; color: white; border: 1px solid #0b8bdc; font-size: 16px; font-weight: 850; padding: 14px 16px; }"
        "QPushButton#PrimaryButton:hover, QPushButton#CopilotPrimaryButton:hover { background: #0a7bc2; }"
        "QPushButton#CopilotSecondaryButton { background: #1d1e20; color: #d7d7dc; border: 1px solid #414247; font-size: 15px; font-weight: 800; padding: 12px 16px; }"
        "QPushButton#CopilotSecondaryButton:hover { background: #27282c; }"
        "QPushButton:checked { background: #16364f; color: #64c3ff; border-color: #0b8bdc; }"
        "QPushButton:disabled { color: #696a70; background: #242528; border-color: #33343a; }"
        "QCheckBox { background: transparent; color: #e5e7eb; spacing: 10px; }"
        "QCheckBox::indicator { width: 24px; height: 24px; border: 3px solid #6a6b70; border-radius: 6px; background: transparent; }"
        "QCheckBox::indicator:checked { background: #0b8bdc; border-color: #0b8bdc; }"
        "QTreeWidget, QTextEdit, QTextBrowser, QComboBox { background: #202123; border: 1px solid #3e3f44; border-radius: 8px; padding: 6px; color: #e5e7eb; selection-background-color: #16364f; selection-color: #ffffff; }"
        "QTextEdit[readOnly=\"true\"] { background: #202123; }"
        "QHeaderView::section { background: #28292d; color: #a7a7ad; border: 0; padding: 8px; font-weight: 800; }"
        "QTreeWidget { alternate-background-color: #242529; }"
        "QTreeWidget::item { padding: 7px 5px; border-radius: 4px; }"
        "QTreeWidget::item:hover { background: #2b3036; }"
        "QTreeWidget::item:selected { background: #16364f; color: #ffffff; }"
        "QTabWidget::pane { border: 1px solid #35363a; background: #202123; border-radius: 8px; top: -1px; }"
        "QTabBar::tab { background: #292a2f; color: #b2b2b8; padding: 10px 18px; border: 1px solid #383940; border-bottom: 0; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 4px; }"
        "QTabBar::tab:hover { background: #303139; color: #ffffff; }"
        "QTabBar::tab:selected { background: #202123; color: #ffffff; font-weight: 800; }"
        "QScrollArea#CopilotScroll { background: transparent; border: 0; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: #4f5056; border-radius: 5px; min-height: 28px; }"
        "QScrollBar::handle:vertical:hover { background: #686a72; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }"
        "QScrollBar::handle:horizontal { background: #4f5056; border-radius: 5px; min-width: 28px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QStatusBar { background: #202123; color: #a1a1aa; border-top: 1px solid #33343a; }"
        "QMenuBar { background: #1c1c1d; color: #d8d8dc; }"
        "QMenuBar::item:selected { background: #2b2c31; }"
        "QMenu { background: #25262a; color: #e8e8ea; border: 1px solid #46474d; }"
        "QMenu::item:selected { background: #16364f; }");
}
