#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPointer>

#include "HWFileScanner/HWFileScanner.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
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
    void scanCurrentProject();
    void submitVersion();
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
    void populateFileList();
    void refreshOverview();
    void refreshChangeSummary();
    void refreshVersionHistory();
    void appendVersionNode(const QString& title, const QString& note);
    QList<CodeFile> selectedFiles() const;
    QString currentModePrompt() const;
    QString currentModeName() const;
    double currentTemperature() const;
    void setBusy(bool busy);

    QString m_projectDir;
    QList<CodeFile> m_files;
    QPointer<HWpilotLLM> m_llm;

    QTreeWidget* m_projectTree = nullptr;
    QTreeWidget* m_versionTree = nullptr;
    QTreeWidgetItem* m_projectRoot = nullptr;
    QTreeWidgetItem* m_versionRoot = nullptr;

    QTabWidget* m_tabs = nullptr;
    QTextBrowser* m_overview = nullptr;
    QListWidget* m_fileList = nullptr;
    QTextBrowser* m_changeSummary = nullptr;
    QTextBrowser* m_history = nullptr;
    QTextBrowser* m_reviewReport = nullptr;

    QComboBox* m_modeCombo = nullptr;
    QTextEdit* m_taskEdit = nullptr;
    QTextEdit* m_questionEdit = nullptr;
    QTextEdit* m_responseView = nullptr;
    QCheckBox* m_includeCodeCheck = nullptr;
    QCheckBox* m_includeHistoryCheck = nullptr;
    QPushButton* m_scanButton = nullptr;
    QPushButton* m_submitButton = nullptr;
    QPushButton* m_analyzeButton = nullptr;
    QPushButton* m_saveFeedbackButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};

#endif  // MAINWINDOW_H
