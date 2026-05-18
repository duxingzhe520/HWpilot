#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPointer>

#include "FeedbackStore/FeedbackStore.h"
#include "GitService/GitService.h"
#include "HWFileScanner/HWFileScanner.h"
#include "ProjectManager/ProjectManager.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QTabWidget;
class QTableWidget;
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
    void startAiAnalysis();
    void saveFeedbackToVersion();
    void updateCurrentVersionPanel();
    void selectAllFiles();
    void handleFileItemChanged(QTreeWidgetItem* item, int column);

private:
    void buildUi();
    void buildLeftPanel();
    void buildCenterPanel();
    void buildAiPanel();
    void applyStyle();

    void setProjectFolder(const QString& folderPath);
    void scanCurrentProject();
    void refreshProjectPanel();
    void populateFileTree();
    void populateFileTreeForPaths(const QStringList& paths);
    void setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state);
    void updateParentCheckState(QTreeWidgetItem* item);
    void refreshChangeSummary();
    void refreshGitState();
    void rebuildVersionTree();
    void appendVersionNode(const QString& title, const QString& note, const QString& commitHash = QString());
    QList<CodeFile> selectedFiles() const;
    QStringList checkedFilePaths() const;
    QString selectedCommitHash() const;
    QString feedbackContextHash() const;
    QString currentWorkContextHash() const;
    QString currentFeedbackHistory() const;
    FeedbackRecord buildFeedbackRecord(const QString& replyText) const;
    void populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks);
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

    QTabWidget* m_tabs = nullptr;
    QTreeWidget* m_fileTree = nullptr;
    QPushButton* m_selectAllFilesButton = nullptr;
    QTextBrowser* m_changeSummary = nullptr;
    QTextBrowser* m_reviewReport = nullptr;
    QTableWidget* m_feedbackIssueTable = nullptr;

    QTextEdit* m_taskEdit = nullptr;
    QTextEdit* m_questionEdit = nullptr;
    QTextEdit* m_responseView = nullptr;
    QLabel* m_aiTitleLabel = nullptr;
    QCheckBox* m_includeCodeCheck = nullptr;
    QCheckBox* m_includeHistoryCheck = nullptr;
    QPushButton* m_analyzeButton = nullptr;
    QPushButton* m_saveFeedbackButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    bool m_updatingFileTree = false;
};

#endif  // MAINWINDOW_H
