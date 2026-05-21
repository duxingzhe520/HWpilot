#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPointer>

#include "FeedbackStore/FeedbackStore.h"
#include "GitService/GitService.h"
#include "HWFileScanner/HWFileScanner.h"
#include "ProjectManager/ProjectManager.h"

class QLabel;
class QMenu;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;
class HWpilotLLM;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openProjectFolder();
    void refreshCurrentProject();
    void commitCurrentSnapshot();
    void startAiAnalysis();
    void saveFeedbackToVersion();
    void updateCurrentVersionPanel();
    void selectAllFiles();
    void selectAllAiFeedbackRecords();
    void showAiCodeFilePicker();
    void showAiFeedbackRecordPicker();
    void handleAiFeedbackItemChanged(QTreeWidgetItem* item, int column);
    void handleFileItemChanged(QTreeWidgetItem* item, int column);
    void handleFeedbackTreeSelection(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    static constexpr int NoteRole = Qt::UserRole + 2;
    static constexpr int CommitHashRole = Qt::UserRole + 3;
    static constexpr int FilePathRole = Qt::UserRole + 4;
    static constexpr int IsDirectoryRole = Qt::UserRole + 5;
    static constexpr int DetailHtmlRole = Qt::UserRole + 6;

    void buildUi();
    void buildLeftPanel();
    void buildCenterPanel();
    void buildAiPanel();
    void buildMenuBar();
    void applyLanguage();
    void applyStyle();

    void setProjectFolder(const QString& folderPath);
    void rememberRecentProject(const QString& folderPath);
    void updateRecentProjectsMenu();
    void scanCurrentProject(bool showWarnings = true);
    void refreshProjectPanel();
    void populateFileTree();
    void populateFileTreeForPaths(const QStringList& paths);
    void setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state);
    void updateParentCheckState(QTreeWidgetItem* item);
    void refreshChangeSummary();
    void refreshGitState();
    void rebuildVersionTree(const QString& preferredCommitHash = QString());
    void appendVersionNode(const QString& title, const QString& note, const QString& commitHash = QString());
    QList<CodeFile> selectedFiles() const;
    QStringList checkedFilePaths() const;
    QString selectedCommitHash() const;
    QString feedbackContextHash() const;
    QString feedbackSaveContextHash() const;
    QString currentWorkContextHash() const;
    QString currentFeedbackHistory() const;
    QString selectedFeedbackHistory() const;
    FeedbackRecord buildFeedbackRecord(const QString& replyText) const;
    void populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks);
    void populateAiFeedbackPicker();
    QString currentModePrompt() const;
    QString currentModeName() const;
    double currentTemperature() const;
    void setBusy(bool busy);

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
    QTreeWidget* m_aiFeedbackTree = nullptr;
    QPushButton* m_selectAllFilesButton = nullptr;
    QPushButton* m_selectAllFeedbackRecordsButton = nullptr;
    QTextBrowser* m_changeSummary = nullptr;
    QTextBrowser* m_reviewReport = nullptr;
    QTreeWidget* m_feedbackTree = nullptr;

    QTextEdit* m_taskEdit = nullptr;
    QTextEdit* m_questionEdit = nullptr;
    QTextEdit* m_responseView = nullptr;
    QLabel* m_aiTitleLabel = nullptr;
    QPushButton* m_chooseCodeFilesButton = nullptr;
    QPushButton* m_chooseFeedbackRecordsButton = nullptr;
    QPushButton* m_analyzeButton = nullptr;
    QPushButton* m_saveFeedbackButton = nullptr;
    QPushButton* m_cancelFeedbackButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QMenu* m_recentProjectsMenu = nullptr;
    QString m_lastAiReply;
    double m_temperature = 0.3;
    bool m_updatingFileTree = false;
    bool m_updatingAiFeedbackTree = false;
};

#endif  // MAINWINDOW_H
