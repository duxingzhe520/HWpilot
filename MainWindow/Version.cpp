#include "../MainWindow.h"

#include "../AppText.h"

#include <QDateTime>
#include <QDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTreeWidget>

#include "Render.h"

using namespace MainWindowRender;

void MainWindow::commitCurrentSnapshot() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    const QStringList paths = m_gitService.changedPaths();
    if (paths.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.noChanges.title"), AppText::get("dialog.noChanges.body"));
        return;
    }

    const QString defaultMessage = QString("Save progress: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    QInputDialog dialog(this);
    dialog.setWindowTitle(AppText::get("dialog.commitMessage.title"));
    dialog.setLabelText(AppText::get("dialog.commitMessage.label"));
    dialog.setTextValue(defaultMessage);
    dialog.setTextEchoMode(QLineEdit::Normal);
    dialog.setMinimumSize(520, 180);
    dialog.setStyleSheet(
        "QInputDialog { background: #f8fafc; color: #1f2937; }"
        "QLabel { background: transparent; color: #1f2937; font-size: 14px; }"
        "QLineEdit { background: #ffffff; color: #111827; border: 1px solid #cbd5e1; padding: 8px; selection-background-color: #dbeafe; }"
        "QPushButton { background: #ffffff; color: #243244; border: 1px solid #d9dee7; padding: 7px 14px; }"
        "QPushButton:hover { background: #eef6ff; }");

    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString message = dialog.textValue().trimmed();
    if (message.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.emptyCommit.title"), AppText::get("dialog.emptyCommit.body"));
        return;
    }

    m_statusLabel->setText(AppText::get("status.committing"));
    const GitCommandResult addResult = m_gitService.addPaths(paths);
    if (!addResult.success) {
        QMessageBox::warning(this, AppText::get("dialog.gitAddFailed"), addResult.stderrText.trimmed().isEmpty() ? addResult.stdoutText.trimmed() : addResult.stderrText.trimmed());
        m_statusLabel->setText(AppText::get("status.commitFailed"));
        return;
    }

    const GitCommandResult commitResult = m_gitService.commit(message);
    if (!commitResult.success) {
        QMessageBox::warning(this, AppText::get("dialog.gitCommitFailed"), commitResult.stderrText.trimmed().isEmpty() ? commitResult.stdoutText.trimmed() : commitResult.stderrText.trimmed());
        m_statusLabel->setText(AppText::get("status.commitFailed"));
        return;
    }

    const QString newHead = m_gitService.currentHead();
    refreshGitState();
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        if (item->data(0, CommitHashRole).toString() == newHead) {
            m_versionTree->setCurrentItem(item);
            break;
        }
    }
    m_statusLabel->setText(AppText::get("status.commitDone"));
}

void MainWindow::refreshChangeSummary() {
    QString html = QString("<h2>%1</h2>").arg(htmlEscape(AppText::get("label.versionOverview")));
    if (m_projectDir.isEmpty()) {
        html += QString("<p>%1</p>").arg(htmlEscape(AppText::get("label.noProject")));
        m_changeSummary->setHtml(html);
        return;
    }

    const QString commitHash = selectedCommitHash();
    const QString versionTitle = m_versionTree->currentItem() ? m_versionTree->currentItem()->text(0) : AppText::get("version.currentWorkspace");
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.feedbacksForCommit(feedbackContextHash());
    const int fileCount = commitHash.isEmpty() ? m_files.size() : m_gitService.filesAtCommit(commitHash).size();

    html += QString("<h3>%1</h3>").arg(htmlEscape(AppText::get("version.info")));
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%;\">";
    html += QString("<tr><td style=\"color:#64748b; width:110px;\">%1</td><td><b>%2</b></td></tr>").arg(htmlEscape(AppText::get("version.version")), htmlEscape(versionTitle));
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.fileCount"))).arg(fileCount);
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.feedbackCount")), htmlEscape(AppText::get("version.recordsUnit").arg(feedbacks.size())));

    if (commitHash.isEmpty()) {
        const QString diffStat = m_gitService.diffStat().trimmed();
        const QString diff = m_gitService.diff().trimmed();

        html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.type")), htmlEscape(AppText::get("version.currentWorkspace")));
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.changeStat")))
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += QString("<h3>%1</h3>").arg(htmlEscape(AppText::get("version.diffTitle")));
        html += renderReadableDiff(diff);
    } else {
        GitCommit selectedCommit;
        for (const GitCommit& commit : m_commits) {
            if (commit.hash == commitHash) {
                selectedCommit = commit;
                break;
            }
        }
        const QString diffStat = m_gitService.diffStatForCommit(commitHash).trimmed();
        const QString diff = m_gitService.diffForCommit(commitHash).trimmed();

        html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.type")), htmlEscape(AppText::get("version.historyCommit")));
        html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.commitMessage")), htmlEscape(selectedCommit.subject));
        html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.commitDate")), htmlEscape(selectedCommit.date));
        const QStringList branches = m_gitService.branchesContainingCommit(commitHash);
        html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>")
                    .arg(htmlEscape(AppText::get("version.branch")), htmlEscape(branches.isEmpty() ? AppText::get("version.noBranch") : branches.join(", ")));
        html += QString("<tr><td style=\"color:#64748b;\">Commit Hash</td><td><code>%1</code></td></tr>").arg(htmlEscape(commitHash));
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.changeStat")))
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += QString("<h3>%1</h3>").arg(htmlEscape(AppText::get("version.diffTitle")));
        html += renderReadableDiff(diff);
    }
    m_changeSummary->setHtml(html);
}

void MainWindow::refreshGitState() {
    const QString previousCommitHash = selectedCommitHash();
    m_commits = m_gitService.log();
    rebuildVersionTree(previousCommitHash);
    populateFileTree();
    refreshChangeSummary();
}

void MainWindow::rebuildVersionTree(const QString& preferredCommitHash) {
    m_versionRoot->takeChildren();
    if (m_projectDir.isEmpty()) {
        appendVersionNode(AppText::get("version.noProjectNode"), AppText::get("version.noProjectNote"));
    } else {
        appendVersionNode(AppText::get("version.currentWorkspace"), AppText::get("version.currentWorkspaceNote"));

        for (const GitCommit& commit : m_commits) {
            const QString baseTitle = commit.subject.trimmed().isEmpty() ? AppText::get("feedback.untitledCommit") : commit.subject.trimmed();
            QString note = AppText::get("version.dateCommit").arg(commit.date, commit.hash);
            appendVersionNode(baseTitle, note, commit.hash);
        }
    }

    m_versionTree->expandAll();
    QTreeWidgetItem* itemToSelect = nullptr;
    if (!preferredCommitHash.isEmpty()) {
        for (int i = 0; i < m_versionRoot->childCount(); ++i) {
            QTreeWidgetItem* item = m_versionRoot->child(i);
            if (item->data(0, CommitHashRole).toString() == preferredCommitHash) {
                itemToSelect = item;
                break;
            }
        }
    }
    if (!itemToSelect && m_versionRoot->childCount() > 0)
        itemToSelect = m_versionRoot->child(0);
    if (itemToSelect)
        m_versionTree->setCurrentItem(itemToSelect);
}

void MainWindow::appendVersionNode(const QString& title, const QString& note, const QString& commitHash) {
    auto* item = new QTreeWidgetItem(QStringList() << title);
    item->setData(0, NoteRole, note);
    item->setData(0, CommitHashRole, commitHash);
    item->setToolTip(0, commitHash.isEmpty() ? note : note.section('\n', 0, 0));
    m_versionRoot->addChild(item);
}

void MainWindow::updateCurrentVersionPanel() {
    QTreeWidgetItem* item = m_versionTree->currentItem();
    if (!item)
        return;

    populateFileTree();
    refreshChangeSummary();

    const QString commitHash = feedbackContextHash();
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.feedbacksForCommit(commitHash);
    QString html;
    html += QString("<h2>%1</h2>").arg(htmlEscape(item->text(0)));
    html += QString("<p>%1</p>").arg(htmlEscape(item->data(0, NoteRole).toString()));
    html += "<hr>";
    m_reviewReport->setHtml(html);
    populateFeedbackPanel(feedbacks);
    if (m_aiPickerStack && m_aiPickerStack->currentIndex() == 1)
        populateAiFeedbackPicker();
}
