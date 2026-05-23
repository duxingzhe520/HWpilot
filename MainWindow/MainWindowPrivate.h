#ifndef MAINWINDOW_PRIVATE_H
#define MAINWINDOW_PRIVATE_H

#include <QList>
#include <QObject>
#include <QPointer>

#include "../FeedbackStore/FeedbackStore.h"
#include "../GitService/GitService.h"
#include "../HWFileScanner/HWFileScanner.h"
#include "../MainWindow.h"
#include "../ProjectManager/ProjectManager.h"

class QLabel;
class QMenu;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QTextBrowser;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class HWpilotLLM;
class MainWindowPrivate : public QObject {
    Q_OBJECT

public:
    explicit MainWindowPrivate(MainWindow* window);

    void buildUi();
    void buildLeftPanel();
    void buildCenterPanel();
    void buildAiPanel();
    void buildMenuBar();
    void applyLanguage();
    void applyStyle();

    void openProjectFolder();
    void refreshCurrentProject();
    void commitCurrentSnapshot();
    void startAiAnalysis();
    void startHeuristicQuestions();
    void saveFeedbackToVersion();
    void saveHeuristicToVersion();
    void updateCurrentVersionPanel();
    void selectAllFiles();
    void selectAllHeuristicFiles();
    void selectAllAiFeedbackRecords();
    void showAiCodeFilePicker();
    void showAiFeedbackRecordPicker();
    void handleAiFeedbackItemChanged(QTreeWidgetItem* item, int column);
    void handleFileItemChanged(QTreeWidgetItem* item, int column);
    void handleHeuristicFileItemChanged(QTreeWidgetItem* item, int column);
    void handleFeedbackTreeSelection(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void showFeedbackTreeContextMenu(const QPoint& position);
    void showFeedbackIssueRecords();
    void showFeedbackReviewRecords();
    void showHeuristicQuestionRecords();

private:
    static constexpr int NoteRole = Qt::UserRole + 2;
    static constexpr int CommitHashRole = Qt::UserRole + 3;
    static constexpr int FilePathRole = Qt::UserRole + 4;
    static constexpr int IsDirectoryRole = Qt::UserRole + 5;
    static constexpr int DetailHtmlRole = Qt::UserRole + 6;
    static constexpr int ItemIdRole = Qt::UserRole + 7;
    static constexpr int RecordIdRole = Qt::UserRole + 8;
    static constexpr int ReviewRecordIdRole = Qt::UserRole + 9;

    void setProjectFolder(const QString& folderPath);
    void rememberRecentProject(const QString& folderPath);
    void updateRecentProjectsMenu();
    void scanCurrentProject(bool showWarnings = true);
    void refreshProjectPanel();
    void populateFileTree();
    void populateFileTreeForPaths(const QStringList& paths);
    void populateHeuristicFileTreeForPaths(const QStringList& paths);
    void setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state);
    void updateParentCheckState(QTreeWidgetItem* item);
    void refreshChangeSummary();
    void refreshGitState();
    void rebuildVersionTree(const QString& preferredCommitHash = QString());
    void appendVersionNode(const QString& title, const QString& note, const QString& commitHash = QString());
    QList<CodeFile> selectedFiles() const;
    QList<CodeFile> selectedHeuristicFiles() const;
    QStringList checkedFilePaths() const;
    QStringList checkedHeuristicFilePaths() const;
    QStringList selectedFeedbackItemIds() const;
    QString selectedCommitHash() const;
    QString feedbackContextHash() const;
    QString feedbackSaveContextHash() const;
    QString currentWorkContextHash() const;
    QString currentFeedbackHistory() const;
    QString selectedFeedbackHistory() const;
    FeedbackRecord buildFeedbackRecord(const QString& replyText) const;
    void populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks);
    void populateAiFeedbackPicker();
    void selectFeedbackRecordById(const QString& recordId);
    QString currentModePrompt() const;
    QString currentModeName() const;
    QString heuristicModePrompt() const;
    QString heuristicModeName() const;
    void updateAiActionText();
    double currentTemperature() const;
    void setBusy(bool busy);

    MainWindow* q = nullptr;
    QString m_projectDir;
    QList<CodeFile> m_files;
    QList<GitCommit> m_commits;
    ProjectManager m_projectManager;
    FeedbackStore m_feedbackStore;
    GitService m_gitService;
    QPointer<HWpilotLLM> m_llm;

    QTreeWidget* m_versionTree = nullptr;
    QTreeWidgetItem* m_versionRoot = nullptr;
    QLabel* m_projectNameLabel = nullptr;
    QLabel* m_projectPathLabel = nullptr;
    QLabel* m_fileCountLabel = nullptr;
    QLabel* m_feedbackCountLabel = nullptr;
    QPushButton* m_openProjectButton = nullptr;
    QPushButton* m_refreshProjectButton = nullptr;
    QPushButton* m_commitButton = nullptr;

    QTabWidget* m_tabs = nullptr;
    QStackedWidget* m_aiPickerStack = nullptr;
    QTreeWidget* m_fileTree = nullptr;
    QTreeWidget* m_heuristicFileTree = nullptr;
    QTreeWidget* m_aiFeedbackTree = nullptr;
    QPushButton* m_selectAllFilesButton = nullptr;
    QPushButton* m_selectAllHeuristicFilesButton = nullptr;
    QPushButton* m_selectAllFeedbackRecordsButton = nullptr;
    QTextBrowser* m_changeSummary = nullptr;
    QTextBrowser* m_reviewReport = nullptr;
    QTreeWidget* m_feedbackTree = nullptr;
    QPushButton* m_feedbackIssuesButton = nullptr;
    QPushButton* m_feedbackReviewButton = nullptr;
    QPushButton* m_heuristicQuestionsButton = nullptr;
    QPushButton* m_openReviewRecordButton = nullptr;

    QTextEdit* m_taskEdit = nullptr;
    QTextEdit* m_questionEdit = nullptr;
    QTextEdit* m_responseView = nullptr;
    QLabel* m_aiTitleLabel = nullptr;
    QPushButton* m_chooseCodeFilesButton = nullptr;
    QPushButton* m_chooseFeedbackRecordsButton = nullptr;
    QPushButton* m_analyzeButton = nullptr;
    QPushButton* m_saveFeedbackButton = nullptr;
    QPushButton* m_cancelFeedbackButton = nullptr;
    QTextEdit* m_heuristicTaskEdit = nullptr;
    QTextEdit* m_heuristicQuestionEdit = nullptr;
    QTextEdit* m_heuristicResponseView = nullptr;
    QLabel* m_heuristicTitleLabel = nullptr;
    QPushButton* m_generateHeuristicButton = nullptr;
    QPushButton* m_saveHeuristicButton = nullptr;
    QPushButton* m_cancelHeuristicButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QMenu* m_recentProjectsMenu = nullptr;
    QString m_lastAiReply;
    QString m_lastHeuristicReply;
    QString m_lastAiModeName;
    QString m_pendingFeedbackMode;
    double m_temperature = 0.3;
    bool m_updatingFileTree = false;
    bool m_updatingHeuristicFileTree = false;
    bool m_updatingAiFeedbackTree = false;
    bool m_showingHeuristicRecords = false;
    bool m_showingReviewRecords = false;
    bool m_fileTreeInitialized = false;
    bool m_heuristicFileTreeInitialized = false;
};

#endif  // MAINWINDOW_PRIVATE_H
