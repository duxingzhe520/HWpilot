#include "../MainWindow.h"

#include "../AppText.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QTextEdit>
#include <QTreeWidget>

#include "Render.h"

using namespace MainWindowRender;

void MainWindow::openProjectFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, AppText::get("menu.openProject"), m_projectDir);
    if (folder.isEmpty())
        return;

    setProjectFolder(folder);
    scanCurrentProject();
}

void MainWindow::refreshCurrentProject() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    scanCurrentProject();
    refreshGitState();
    m_statusLabel->setText(AppText::get("status.refreshDone").arg(m_files.size()));
}

void MainWindow::setProjectFolder(const QString& folderPath) {
    m_projectDir = QDir(folderPath).absolutePath();
    QString errorMessage;
    if (!m_projectManager.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(this, AppText::get("dialog.projectInitFailed"), errorMessage);
        return;
    }
    if (!m_feedbackStore.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(this, AppText::get("dialog.feedbackStoreInitFailed"), errorMessage);
        return;
    }
    if (m_feedbackStore.isEmpty() && !m_projectManager.data().feedbacks.isEmpty()) {
        m_feedbackStore.importFeedbacks(m_projectManager.data().feedbacks);
        m_feedbackStore.save(&errorMessage);
    }

    m_gitService.setWorkingDirectory(m_projectDir);
    if (!m_gitService.isGitRepo()) {
        const GitCommandResult initResult = m_gitService.initRepo();
        if (!initResult.success) {
            QMessageBox::warning(this, AppText::get("dialog.gitInitFailed"), initResult.stderrText.trimmed());
        }
    }

    m_taskEdit->setPlainText(m_projectManager.data().assignmentText);
    rememberRecentProject(m_projectDir);
    refreshProjectPanel();
    refreshGitState();
}

void MainWindow::rememberRecentProject(const QString& folderPath) {
    QStringList projects = QSettings("HWpilot", "HWpilot").value("recentProjects").toStringList();
    const QString absolutePath = QDir(folderPath).absolutePath();
    projects.removeAll(absolutePath);
    projects.prepend(absolutePath);
    while (projects.size() > 6)
        projects.removeLast();

    QSettings("HWpilot", "HWpilot").setValue("recentProjects", projects);
    updateRecentProjectsMenu();
}

void MainWindow::scanCurrentProject(bool showWarnings) {
    if (m_projectDir.isEmpty()) {
        if (showWarnings)
            openProjectFolder();
        return;
    }

    if (showWarnings)
        m_statusLabel->setText(AppText::get("status.scanning"));
    m_files = HWFileScanner::scanDirectory(m_projectDir);
    m_projectManager.data().assignmentText = m_taskEdit->toPlainText().trimmed();
    QString errorMessage;
    m_projectManager.save(&errorMessage);
    populateFileTree();
    refreshProjectPanel();
    refreshChangeSummary();
    if (showWarnings)
        m_statusLabel->setText(AppText::get("status.scanDone").arg(m_files.size()));

    if (showWarnings && m_files.isEmpty()) {
        QMessageBox::warning(this, AppText::get("dialog.noFilesFound.title"), AppText::get("dialog.noFilesFound.body"));
    }
}

void MainWindow::refreshProjectPanel() {
    const QString name = m_projectDir.isEmpty() ? AppText::get("label.noProject") : QFileInfo(m_projectDir).fileName();
    m_projectNameLabel->setText(name);
    m_projectPathLabel->setText(m_projectDir.isEmpty() ? AppText::get("label.chooseProjectStart") : m_projectDir);
    m_fileCountLabel->setText(AppText::get("label.filesCount").arg(m_files.size()));
    m_feedbackCountLabel->setText(AppText::get("label.feedbackCount").arg(m_feedbackStore.allFeedbacks().size()));
}

void MainWindow::populateFileTree() {
    QStringList paths;
    const QString commitHash = selectedCommitHash();
    if (commitHash.isEmpty()) {
        for (const CodeFile& file : m_files)
            paths.append(file.relativePath);
    } else {
        const QStringList committedPaths = m_gitService.filesAtCommit(commitHash);
        for (const QString& path : committedPaths) {
            if (isSupportedFilePath(path))
                paths.append(path);
        }
    }

    populateFileTreeForPaths(paths);
}

void MainWindow::populateFileTreeForPaths(const QStringList& paths) {
    m_updatingFileTree = true;
    m_fileTree->clear();

    QStringList sortedPaths = paths;
    sortedPaths.removeDuplicates();
    sortedPaths.sort(Qt::CaseInsensitive);

    for (const QString& path : sortedPaths) {
        QTreeWidgetItem* parent = m_fileTree->invisibleRootItem();
        QString currentPath;
        const QStringList parts = path.split('/', Qt::SkipEmptyParts);
        for (int i = 0; i < parts.size(); ++i) {
            const bool isLast = i == parts.size() - 1;
            currentPath = currentPath.isEmpty() ? parts.at(i) : currentPath + "/" + parts.at(i);

            QTreeWidgetItem* child = nullptr;
            for (int childIndex = 0; childIndex < parent->childCount(); ++childIndex) {
                QTreeWidgetItem* existing = parent->child(childIndex);
                if (existing->text(0) == parts.at(i)) {
                    child = existing;
                    break;
                }
            }

            if (!child) {
                child = new QTreeWidgetItem(parent, QStringList() << parts.at(i));
                child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
                child->setCheckState(0, Qt::Checked);
            }

            child->setData(0, FilePathRole, currentPath);
            child->setData(0, IsDirectoryRole, !isLast);
            child->setToolTip(0, currentPath);
            parent = child;
        }
    }

    m_fileTree->expandAll();
    m_updatingFileTree = false;
}

void MainWindow::setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state) {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        child->setCheckState(0, state);
        setTreeChildrenCheckState(child, state);
    }
}

void MainWindow::updateParentCheckState(QTreeWidgetItem* item) {
    QTreeWidgetItem* parent = item->parent();
    while (parent) {
        int checkedCount = 0;
        int partialCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            const Qt::CheckState state = parent->child(i)->checkState(0);
            if (state == Qt::Checked)
                ++checkedCount;
            else if (state == Qt::PartiallyChecked)
                ++partialCount;
        }

        if (checkedCount == parent->childCount()) {
            parent->setCheckState(0, Qt::Checked);
        } else if (checkedCount == 0 && partialCount == 0) {
            parent->setCheckState(0, Qt::Unchecked);
        } else {
            parent->setCheckState(0, Qt::PartiallyChecked);
        }

        parent = parent->parent();
    }
}

QList<CodeFile> MainWindow::selectedFiles() const {
    QList<CodeFile> files;
    const QString commitHash = selectedCommitHash();
    const QStringList paths = checkedFilePaths();
    for (const QString& path : paths) {
        if (commitHash.isEmpty()) {
            const QString absolutePath = QDir(m_projectDir).filePath(path);
            for (const CodeFile& file : m_files) {
                if (file.absolutePath == absolutePath || file.relativePath == path) {
                    files.append(file);
                    break;
                }
            }
            continue;
        }

        CodeFile file;
        file.relativePath = path;
        file.absolutePath = QDir(m_projectDir).filePath(path);
        file.extension = extensionForPath(path);
        file.content = m_gitService.fileContentAtCommit(commitHash, path);
        files.append(file);
    }
    return files;
}

QStringList MainWindow::checkedFilePaths() const {
    QStringList paths;
    auto collect = [&](auto&& self, QTreeWidgetItem* item) -> void {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child->data(0, IsDirectoryRole).toBool()) {
                self(self, child);
            } else if (child->checkState(0) == Qt::Checked) {
                paths.append(child->data(0, FilePathRole).toString());
            }
        }
    };

    collect(collect, m_fileTree->invisibleRootItem());
    return paths;
}

QString MainWindow::selectedCommitHash() const {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item || item == m_versionRoot)
        return QString();
    return item->data(0, CommitHashRole).toString();
}

QString MainWindow::feedbackContextHash() const {
    const QString commitHash = selectedCommitHash();
    return commitHash.isEmpty() ? currentWorkContextHash() : commitHash;
}

QString MainWindow::feedbackSaveContextHash() const {
    return currentWorkContextHash();
}

void MainWindow::selectAllFiles() {
    if (!m_fileTree)
        return;

    m_updatingFileTree = true;
    QTreeWidgetItem* root = m_fileTree->invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* item = root->child(i);
        item->setCheckState(0, Qt::Checked);
        setTreeChildrenCheckState(item, Qt::Checked);
    }
    m_updatingFileTree = false;
}

void MainWindow::handleFileItemChanged(QTreeWidgetItem* item, int column) {
    if (!item || column != 0 || m_updatingFileTree)
        return;

    m_updatingFileTree = true;
    if (item->data(0, IsDirectoryRole).toBool())
        setTreeChildrenCheckState(item, item->checkState(0));
    updateParentCheckState(item);
    m_updatingFileTree = false;
}

QString MainWindow::currentWorkContextHash() const {
    const QString head = m_gitService.currentHead();
    if (head.isEmpty() || !m_gitService.statusPorcelain().trimmed().isEmpty())
        return "working-tree";
    return head;
}
