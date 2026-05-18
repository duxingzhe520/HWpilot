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
class QListWidget;
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

private:
    void buildUi();
    void buildToolBar();
    void buildLeftPanel();
    void buildCenterPanel();
    void buildAiPanel();
    void applyStyle();

    void setProjectFolder(const QString& folderPath);
    void scanCurrentProject();
    void populateFileList();
    void refreshChangeSummary();
    void refreshGitState();
    void rebuildVersionTree();
    void appendVersionNode(const QString& title, const QString& note, const QString& commitHash = QString());
    QList<CodeFile> selectedFiles() const;
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

    QTreeWidget* m_projectTree = nullptr;
    QTreeWidget* m_versionTree = nullptr;
    QTreeWidgetItem* m_projectRoot = nullptr;
    QTreeWidgetItem* m_versionRoot = nullptr;

    QTabWidget* m_tabs = nullptr;
    QListWidget* m_fileList = nullptr;
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
};

#endif  // MAINWINDOW_H
