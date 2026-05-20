#include "../MainWindow.h"

#include <QDateTime>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTreeWidget>

#include "Render.h"

using namespace MainWindowRender;

void MainWindow::commitCurrentSnapshot() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const QStringList paths = m_gitService.changedPaths();
    if (paths.isEmpty()) {
        QMessageBox::information(this, "没有可提交的变更", "当前工作区没有 Git 可提交的变更。");
        return;
    }

    bool ok = false;
    const QString defaultMessage = QString("保存作业进度：%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    const QString message = QInputDialog::getText(this, "Commit message", "请输入 commit message：", QLineEdit::Normal, defaultMessage, &ok).trimmed();
    if (!ok)
        return;
    if (message.isEmpty()) {
        QMessageBox::information(this, "Commit message 为空", "请输入 commit message 后再提交。");
        return;
    }

    m_statusLabel->setText("正在提交当前快照...");
    const GitCommandResult addResult = m_gitService.addPaths(paths);
    if (!addResult.success) {
        QMessageBox::warning(this, "Git add 失败", addResult.stderrText.trimmed().isEmpty() ? addResult.stdoutText.trimmed() : addResult.stderrText.trimmed());
        m_statusLabel->setText("提交失败");
        return;
    }

    const GitCommandResult commitResult = m_gitService.commit(message);
    if (!commitResult.success) {
        QMessageBox::warning(this, "Git commit 失败", commitResult.stderrText.trimmed().isEmpty() ? commitResult.stdoutText.trimmed() : commitResult.stderrText.trimmed());
        m_statusLabel->setText("提交失败");
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
    m_statusLabel->setText("已提交当前快照");
}

void MainWindow::refreshChangeSummary() {
    QString html = "<h2>版本概览</h2>";
    if (m_projectDir.isEmpty()) {
        html += "<p>尚未打开项目。</p>";
        m_changeSummary->setHtml(html);
        return;
    }

    const QString commitHash = selectedCommitHash();
    const QString versionTitle = m_versionTree->currentItem() ? m_versionTree->currentItem()->text(0) : QString("当前工作区");
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.feedbacksForCommit(feedbackContextHash());
    const int fileCount = commitHash.isEmpty() ? m_files.size() : m_gitService.filesAtCommit(commitHash).size();

    html += "<h3>信息概览</h3>";
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%;\">";
    html += QString("<tr><td style=\"color:#64748b; width:110px;\">版本</td><td><b>%1</b></td></tr>").arg(htmlEscape(versionTitle));
    html += QString("<tr><td style=\"color:#64748b;\">文件数量</td><td>%1</td></tr>").arg(fileCount);
    html += QString("<tr><td style=\"color:#64748b;\">反馈记录</td><td>%1 条</td></tr>").arg(feedbacks.size());

    if (commitHash.isEmpty()) {
        const QString diffStat = m_gitService.diffStat().trimmed();
        const QString diff = m_gitService.diff().trimmed();

        html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>当前工作区</td></tr>");
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">变更统计</td><td>%1</td></tr>")
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += "<h3>相对上一版本的变更</h3>";
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

        html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>历史提交</td></tr>");
        html += QString("<tr><td style=\"color:#64748b;\">提交信息</td><td>%1</td></tr>").arg(htmlEscape(selectedCommit.subject));
        html += QString("<tr><td style=\"color:#64748b;\">提交日期</td><td>%1</td></tr>").arg(htmlEscape(selectedCommit.date));
        const QStringList branches = m_gitService.branchesContainingCommit(commitHash);
        html += QString("<tr><td style=\"color:#64748b;\">所在分支</td><td>%1</td></tr>")
                    .arg(htmlEscape(branches.isEmpty() ? "未找到本地分支引用" : branches.join(", ")));
        html += QString("<tr><td style=\"color:#64748b;\">Commit Hash</td><td><code>%1</code></td></tr>").arg(htmlEscape(commitHash));
        html += QString("<tr><td style=\"color:#64748b; vertical-align:top;\">变更统计</td><td>%1</td></tr>")
                    .arg(renderDiffStat(diffStat));
        html += "</table>";
        html += "<h3>相对上一版本的变更</h3>";
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
        appendVersionNode("尚未打开项目", "请选择一个作业项目文件夹。");
    } else {
        appendVersionNode("当前工作区", "当前磁盘上的项目文件和未提交变更。");

        for (const GitCommit& commit : m_commits) {
            const QString baseTitle = commit.subject.trimmed().isEmpty() ? "未命名提交" : commit.subject.trimmed();
            const QStringList branches = m_gitService.branchesContainingCommit(commit.hash);
            const QString branchHint = branches.isEmpty() ? "无分支引用" : branches.first();
            const QString title = QString("%1  |  %2  |  %3").arg(baseTitle, commit.date, branchHint);

            QString note = QString("日期：%1\nCommit：%2").arg(commit.date, commit.hash);
            if (!branches.isEmpty())
                note += QString("\n所在分支：%1").arg(branches.join(", "));
            appendVersionNode(title, note, commit.hash);
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
}

