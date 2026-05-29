#include "MainWindowPrivate.h"

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

void MainWindowPrivate::openProjectFolder() {
    const QString folder = QFileDialog::getExistingDirectory(q, AppText::get("menu.openProject"), m_projectDir);
    if (folder.isEmpty())
        return;

    setProjectFolder(folder);
    scanCurrentProject();
}

void MainWindowPrivate::refreshCurrentProject() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    scanCurrentProject();
    refreshGitState();
    m_statusLabel->setText(AppText::get("status.refreshDone").arg(m_files.size()));
}

void MainWindowPrivate::setProjectFolder(const QString& folderPath) {
    m_projectDir = QDir(folderPath).absolutePath();
    m_fileTreeInitialized = false;
    m_heuristicFileTreeInitialized = false;
    QString errorMessage;
    if (!m_projectManager.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.projectInitFailed"), errorMessage);
        return;
    }
    if (!m_feedbackStore.openProject(m_projectDir, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.feedbackStoreInitFailed"), errorMessage);
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
            QMessageBox::warning(q, AppText::get("dialog.gitInitFailed"), initResult.stderrText.trimmed());
        }
    }

    m_taskEdit->setPlainText(m_projectManager.data().assignmentText);
    rememberRecentProject(m_projectDir);
    refreshProjectPanel();
    refreshGitState();
}

void MainWindowPrivate::rememberRecentProject(const QString& folderPath) {
    QStringList projects = QSettings("HWpilot", "HWpilot").value("recentProjects").toStringList();
    const QString absolutePath = QDir(folderPath).absolutePath();
    projects.removeAll(absolutePath);
    projects.prepend(absolutePath);
    while (projects.size() > 6)
        projects.removeLast();

    QSettings("HWpilot", "HWpilot").setValue("recentProjects", projects);
    updateRecentProjectsMenu();
}

void MainWindowPrivate::scanCurrentProject(bool showWarnings) {
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
        QMessageBox::warning(q, AppText::get("dialog.noFilesFound.title"), AppText::get("dialog.noFilesFound.body"));
    }
}

void MainWindowPrivate::refreshProjectPanel() {
    const QString name = m_projectDir.isEmpty() ? AppText::get("label.noProject") : QFileInfo(m_projectDir).fileName();
    m_projectNameLabel->setText(name);
    m_projectPathLabel->setText(m_projectDir.isEmpty() ? AppText::get("label.chooseProjectStart") : m_projectDir);
    m_fileCountLabel->setText(AppText::get("label.filesCount").arg(m_files.size()));
    m_feedbackCountLabel->setText(AppText::get("label.feedbackCount").arg(m_feedbackStore.allFeedbacks().size()));
    refreshCopilotPanel();
}

void MainWindowPrivate::populateFileTree() {
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
    populateHeuristicFileTreeForPaths(paths);
}

void MainWindowPrivate::populateFileTreeForPaths(const QStringList& paths) {
    const QStringList previouslyCheckedPaths = m_fileTreeInitialized ? checkedFilePaths() : paths;
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
                child->setCheckState(0, Qt::Unchecked);
            }

            child->setData(0, FilePathRole, currentPath);
            child->setData(0, IsDirectoryRole, !isLast);
            if (isLast)
                child->setCheckState(0, previouslyCheckedPaths.contains(currentPath) ? Qt::Checked : Qt::Unchecked);
            child->setToolTip(0, currentPath);
            parent = child;
        }
    }

    auto updateParents = [&](auto&& self, QTreeWidgetItem* item) -> void {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            self(self, child);
            if (!child->data(0, IsDirectoryRole).toBool())
                updateParentCheckState(child);
        }
    };
    updateParents(updateParents, m_fileTree->invisibleRootItem());

    m_fileTree->expandAll();
    m_fileTreeInitialized = true;
    m_updatingFileTree = false;
}

void MainWindowPrivate::populateHeuristicFileTreeForPaths(const QStringList& paths) {
    if (!m_heuristicFileTree)
        return;

    const QStringList previouslyCheckedPaths = m_heuristicFileTreeInitialized ? checkedHeuristicFilePaths() : paths;
    m_updatingHeuristicFileTree = true;
    m_heuristicFileTree->clear();

    QStringList sortedPaths = paths;
    sortedPaths.removeDuplicates();
    sortedPaths.sort(Qt::CaseInsensitive);

    for (const QString& path : sortedPaths) {
        QTreeWidgetItem* parent = m_heuristicFileTree->invisibleRootItem();
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
                child->setCheckState(0, Qt::Unchecked);
            }

            child->setData(0, FilePathRole, currentPath);
            child->setData(0, IsDirectoryRole, !isLast);
            if (isLast)
                child->setCheckState(0, previouslyCheckedPaths.contains(currentPath) ? Qt::Checked : Qt::Unchecked);
            child->setToolTip(0, currentPath);
            parent = child;
        }
    }

    auto updateParents = [&](auto&& self, QTreeWidgetItem* item) -> void {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            self(self, child);
            if (!child->data(0, IsDirectoryRole).toBool())
                updateParentCheckState(child);
        }
    };
    updateParents(updateParents, m_heuristicFileTree->invisibleRootItem());

    m_heuristicFileTree->expandAll();
    m_heuristicFileTreeInitialized = true;
    m_updatingHeuristicFileTree = false;
}

void MainWindowPrivate::setTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState state) {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        child->setCheckState(0, state);
        setTreeChildrenCheckState(child, state);
    }
}

void MainWindowPrivate::updateParentCheckState(QTreeWidgetItem* item) {
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

QList<CodeFile> MainWindowPrivate::selectedFiles() const {
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

QStringList MainWindowPrivate::checkedFilePaths() const {
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

QList<CodeFile> MainWindowPrivate::selectedHeuristicFiles() const {
    QList<CodeFile> files;
    const QString commitHash = selectedCommitHash();
    const QStringList paths = checkedHeuristicFilePaths();
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

QStringList MainWindowPrivate::checkedHeuristicFilePaths() const {
    QStringList paths;
    if (!m_heuristicFileTree)
        return paths;

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

    collect(collect, m_heuristicFileTree->invisibleRootItem());
    return paths;
}

QStringList MainWindowPrivate::selectedFeedbackItemIds() const {
    QStringList ids;
    if (!m_aiFeedbackTree)
        return ids;

    QTreeWidgetItem* root = m_aiFeedbackTree->invisibleRootItem();
    for (int groupIndex = 0; groupIndex < root->childCount(); ++groupIndex) {
        QTreeWidgetItem* groupItem = root->child(groupIndex);
        for (int recordIndex = 0; recordIndex < groupItem->childCount(); ++recordIndex) {
            QTreeWidgetItem* recordItem = groupItem->child(recordIndex);
            for (int childIndex = 0; childIndex < recordItem->childCount(); ++childIndex) {
                QTreeWidgetItem* issueItem = recordItem->child(childIndex);
                if (issueItem->checkState(0) != Qt::Checked)
                    continue;

                const QString id = issueItem->data(0, ItemIdRole).toString();
                if (!id.isEmpty())
                    ids.append(id);
            }
        }
    }

    ids.removeDuplicates();
    return ids;
}

QString MainWindowPrivate::selectedCommitHash() const {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item || item == m_versionRoot)
        return QString();
    return item->data(0, CommitHashRole).toString();
}

QString MainWindowPrivate::feedbackContextHash() const {
    const QString commitHash = selectedCommitHash();
    return commitHash.isEmpty() ? currentWorkContextHash() : commitHash;
}

QString MainWindowPrivate::feedbackSaveContextHash() const {
    return currentWorkContextHash();
}

void MainWindowPrivate::selectAllFiles() {
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

void MainWindowPrivate::selectAllHeuristicFiles() {
    if (!m_heuristicFileTree)
        return;

    m_updatingHeuristicFileTree = true;
    QTreeWidgetItem* root = m_heuristicFileTree->invisibleRootItem();
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem* item = root->child(i);
        item->setCheckState(0, Qt::Checked);
        setTreeChildrenCheckState(item, Qt::Checked);
    }
    m_updatingHeuristicFileTree = false;
}

void MainWindowPrivate::setCheckedFilePaths(QTreeWidget* tree, const QStringList& paths, bool& updatingFlag) {
    if (!tree)
        return;

    updatingFlag = true;
    auto apply = [&](auto&& self, QTreeWidgetItem* item) -> void {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem* child = item->child(i);
            if (child->data(0, IsDirectoryRole).toBool()) {
                self(self, child);
            } else {
                child->setCheckState(0, paths.contains(child->data(0, FilePathRole).toString()) ? Qt::Checked : Qt::Unchecked);
                updateParentCheckState(child);
            }
        }
    };
    apply(apply, tree->invisibleRootItem());

    for (int i = 0; i < tree->invisibleRootItem()->childCount(); ++i)
        updateParentCheckState(tree->invisibleRootItem()->child(i));
    updatingFlag = false;
}

void MainWindowPrivate::handleFileItemChanged(QTreeWidgetItem* item, int column) {
    if (!item || column != 0 || m_updatingFileTree)
        return;

    m_updatingFileTree = true;
    if (item->data(0, IsDirectoryRole).toBool())
        setTreeChildrenCheckState(item, item->checkState(0));
    updateParentCheckState(item);
    m_updatingFileTree = false;
}

void MainWindowPrivate::handleHeuristicFileItemChanged(QTreeWidgetItem* item, int column) {
    if (!item || column != 0 || m_updatingHeuristicFileTree)
        return;

    m_updatingHeuristicFileTree = true;
    if (item->data(0, IsDirectoryRole).toBool())
        setTreeChildrenCheckState(item, item->checkState(0));
    updateParentCheckState(item);
    m_updatingHeuristicFileTree = false;
}

QString MainWindowPrivate::currentWorkContextHash() const {
    const QString head = m_gitService.currentHead();
    if (head.isEmpty() || !m_gitService.statusPorcelain().trimmed().isEmpty())
        return "working-tree";
    return head;
}
