#include "MainWindowPrivate.h"

#include "../AppText.h"

#include <QApplication>
#include <QActionGroup>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPair>
#include <QPushButton>
#include <QSettings>
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
    q->resize(1280, 820);

    auto* root = new QWidget(q);
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
    q->setCentralWidget(root);

    m_statusLabel = new QLabel(AppText::get("label.chooseProjectStart"), q);
    q->statusBar()->addWidget(m_statusLabel, 1);
    refreshProjectPanel();
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

void MainWindowPrivate::applyStyle() {
    qApp->setStyleSheet(
        "QMainWindow, QWidget { background: #eef2f6; color: #1f2937; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Arial, sans-serif; font-size: 14px; line-height: 1.45; }"
        "QSplitter::handle { background: #e4eaf1; }"
        "QSplitter::handle:hover { background: #d5dee9; }"
        "QToolButton, QPushButton { background: #ffffff; border: 0; border-radius: 0px; padding: 8px 13px; color: #243244; line-height: 1.45; }"
        "QToolButton:hover, QPushButton:hover { background: #f3f8ff; }"
        "QToolButton:pressed, QPushButton:pressed { background: #e8f1ff; }"
        "QPushButton#PrimaryButton { background: #2563eb; color: white; font-weight: 600; }"
        "QPushButton#PrimaryButton:hover { background: #1d4ed8; }"
        "QPushButton:checked { background: #dbeafe; color: #1d4ed8; font-weight: 600; }"
        "QPushButton:disabled { color: #94a3b8; background: #e8edf3; }"
        "QTreeWidget, QTextEdit, QTextBrowser, QComboBox { background: #ffffff; border: 0; border-radius: 0px; padding: 5px; selection-background-color: #dbeafe; selection-color: #0f172a; line-height: 1.45; }"
        "QTextEdit[readOnly=\"true\"] { background: #fbfcfe; }"
        "QHeaderView::section { background: #f3f6fa; color: #475569; border: 0; padding: 7px 8px; font-weight: 600; line-height: 1.45; }"
        "QLabel#PanelTitle { background: transparent; color: #0f172a; font-size: 16px; font-weight: 700; padding: 2px 0 6px 0; line-height: 1.45; }"
        "QLabel#ProjectName { background: transparent; color: #0f172a; font-size: 18px; font-weight: 700; padding: 2px 0 3px 0; line-height: 1.45; }"
        "QLabel#MetaLabel { background: transparent; color: #5b6878; padding: 1px 0; line-height: 1.45; }"
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
