#include "../MainWindow.h"

#include "../AppText.h"

#include <algorithm>

#include <QDateTime>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QtGlobal>

#include "Render.h"

using namespace MainWindowRender;

namespace {
QString compactFeedbackTitle(QString title) {
    title = title.simplified();
    title.replace("，", " ");
    title.replace(",", " ");
    title.replace("；", " ");
    title.replace(";", " ");
    title = title.simplified();

    const int firstBreak = title.indexOf(' ');
    if (firstBreak > 0)
        title = title.left(firstBreak);

    constexpr qsizetype MaxTitleLength = 12;
    if (title.size() > MaxTitleLength)
        title = title.left(MaxTitleLength) + "...";

    return title;
}

QString normalizedFeedbackStatus(const QString& status) {
    if (status == "resolved" || status == "已解决")
        return "resolved";
    if (status == "ignored" || status == "ignore" || status == "忽略")
        return "ignored";
    return "unresolved";
}

QString feedbackStatusText(const QString& status) {
    const QString normalized = normalizedFeedbackStatus(status);
    if (normalized == "resolved")
        return AppText::get("feedback.resolved");
    if (normalized == "ignored")
        return AppText::get("feedback.ignored");
    return AppText::get("feedback.unresolved");
}

void applyFeedbackStatusStyle(QTreeWidgetItem* item, const QString& status) {
    const bool ignored = normalizedFeedbackStatus(status) == "ignored";
    const QBrush ignoredBrush(QColor("#94a3b8"));
    for (int column = 0; column < item->columnCount(); ++column)
        item->setForeground(column, ignored ? ignoredBrush : QBrush());
}

QString feedbackStatusComboStyle(const QString& status) {
    const QString comboColor = normalizedFeedbackStatus(status) == "ignored" ? "#94a3b8" : "#243244";
    return QString(
        "QComboBox { color: %1; padding-left: 6px; padding-right: 24px; min-height: 24px; }"
        "QComboBox::drop-down { width: 22px; border: 0; }"
        "QComboBox QAbstractItemView { min-width: 88px; }")
        .arg(comboColor);
}

QString displayFeedbackTime(QString createdAt) {
    return createdAt.replace('T', ' ');
}

QString feedbackGroupTitle(const QString& commitHash, const QString& currentWorkHash, const QList<GitCommit>& commits) {
    if (commitHash == currentWorkHash || commitHash == "working-tree")
        return AppText::get("feedback.currentWorkspace");

    for (const GitCommit& commit : commits) {
        if (commit.hash == commitHash)
            return commit.subject.trimmed().isEmpty() ? AppText::get("feedback.untitledCommit") : commit.subject.trimmed();
    }

    return commitHash.isEmpty() ? AppText::get("feedback.unlinkedVersion") : AppText::get("feedback.unknownVersion").arg(commitHash.left(8));
}
}  // namespace

FeedbackRecord MainWindow::buildFeedbackRecord(const QString& replyText) const {
    FeedbackRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.commitHash = feedbackSaveContextHash();
    record.mode = currentModeName();
    record.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    record.rawContent = replyText;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(extractJsonObjectText(replyText).toUtf8(), &parseError);
    if (!document.isObject()) {
        record.parseStatus = "raw";
        record.summary = AppText::get("feedback.parseFailed");
        return record;
    }

    const QJsonObject object = document.object();
    record.parseStatus = "parsed";
    record.summary = compactFeedbackTitle(object["summary"].toString(AppText::get("feedback.defaultSummary")));
    record.rawContent = object["rawReport"].toString(replyText);

    const QJsonArray items = object["items"].toArray();
    int index = 1;
    for (const QJsonValue& value : items) {
        if (!value.isObject())
            continue;

        FeedbackItem item = FeedbackItem::fromJson(value.toObject());
        item.id = QString("%1-item-%2").arg(record.id).arg(index++);
        if (item.status.isEmpty())
            item.status = "unresolved";
        item.status = normalizedFeedbackStatus(item.status);
        if (item.severity.isEmpty())
            item.severity = "medium";
        if (item.category.isEmpty())
            item.category = "other";
        record.items.append(item);
    }

    return record;
}

void MainWindow::populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks) {
    m_feedbackTree->clear();

    if (feedbacks.isEmpty()) {
        m_reviewReport->setHtml(QString("<h2 style=\"margin:0 0 8px 0; color:#111827;\">%1</h2><p style=\"color:#64748b;\">%2</p>")
                                     .arg(htmlEscape(AppText::get("feedback.record")), htmlEscape(AppText::get("feedback.noRecordsForVersion"))));
        return;
    }

    for (const FeedbackRecord& feedback : feedbacks) {
        const QString title = feedback.summary.isEmpty() ? AppText::get("feedback.record") : feedback.summary;
        auto* recordItem = new QTreeWidgetItem(m_feedbackTree, QStringList() << title << AppText::get("feedback.issueCount").arg(feedback.items.size()) << feedback.mode << feedback.parseStatus);
        recordItem->setData(0, DetailHtmlRole, renderFeedbackRecordDetail(feedback));
        recordItem->setToolTip(0, displayFeedbackTime(feedback.createdAt));

        QList<FeedbackItem> sortedItems = feedback.items;
        std::stable_sort(sortedItems.begin(), sortedItems.end(), [](const FeedbackItem& left, const FeedbackItem& right) {
            return normalizedFeedbackStatus(left.status) != "ignored" && normalizedFeedbackStatus(right.status) == "ignored";
        });

        for (const FeedbackItem& item : sortedItems) {
            const QString itemStatus = normalizedFeedbackStatus(item.status);
            const QString location = item.filePath.isEmpty()
                                         ? QString()
                                         : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
            auto* itemNode = new QTreeWidgetItem(recordItem, QStringList() << (item.title.isEmpty() ? AppText::get("feedback.unnamedIssue") : item.title) << item.severity << location << feedbackStatusText(itemStatus));
            itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
            itemNode->setData(3, Qt::UserRole, itemStatus);
            applyFeedbackStatusStyle(itemNode, itemStatus);

            auto* statusCombo = new QComboBox(m_feedbackTree);
            statusCombo->setMinimumWidth(88);
            statusCombo->addItem(AppText::get("feedback.unresolved"), "unresolved");
            statusCombo->addItem(AppText::get("feedback.resolved"), "resolved");
            statusCombo->addItem(AppText::get("feedback.ignored"), "ignored");
            statusCombo->setCurrentIndex(statusCombo->findData(itemStatus));
            statusCombo->setStyleSheet(feedbackStatusComboStyle(itemStatus));
            connect(statusCombo, QOverload<int>::of(&QComboBox::activated), this, [this, itemNode, item = item, statusCombo](int index) mutable {
                const QString status = statusCombo->itemData(index).toString();
                QString errorMessage;
                if (!m_feedbackStore.updateItemStatus(item.id, status, &errorMessage)) {
                    QMessageBox::warning(this, AppText::get("feedback.statusUpdateFailed"), errorMessage);
                    return;
                }

                const int scrollPosition = m_feedbackTree->verticalScrollBar()->value();
                item.status = status;
                itemNode->setText(3, feedbackStatusText(status));
                itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
                itemNode->setData(3, Qt::UserRole, status);
                applyFeedbackStatusStyle(itemNode, status);
                statusCombo->setStyleSheet(feedbackStatusComboStyle(status));

                m_feedbackTree->verticalScrollBar()->setValue(scrollPosition);
                if (m_feedbackTree->currentItem() == itemNode)
                    m_reviewReport->setHtml(itemNode->data(0, DetailHtmlRole).toString());
                m_statusLabel->setText(AppText::get("status.feedbackStatusUpdated").arg(feedbackStatusText(status)));
            });
            m_feedbackTree->setItemWidget(itemNode, 3, statusCombo);
        }
    }

    for (int column = 1; column < m_feedbackTree->columnCount(); ++column)
        m_feedbackTree->resizeColumnToContents(column);
    m_feedbackTree->setColumnWidth(3, qMax(m_feedbackTree->columnWidth(3), 104));

    if (m_feedbackTree->topLevelItemCount() > 0)
        m_feedbackTree->setCurrentItem(m_feedbackTree->topLevelItem(0));
}

void MainWindow::populateAiFeedbackPicker() {
    if (!m_aiFeedbackTree)
        return;

    m_updatingAiFeedbackTree = true;
    m_aiFeedbackTree->clear();
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.allFeedbacks();
    if (feedbacks.isEmpty()) {
        auto* emptyItem = new QTreeWidgetItem(m_aiFeedbackTree, QStringList() << AppText::get("feedback.noRecordsForProject"));
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEnabled);
        m_updatingAiFeedbackTree = false;
        return;
    }

    const QString currentWorkHash = currentWorkContextHash();
    QMap<QString, QTreeWidgetItem*> groupItems;
    for (const FeedbackRecord& feedback : feedbacks) {
        const QString groupKey = feedback.commitHash.isEmpty() ? QString("untracked") : feedback.commitHash;
        QTreeWidgetItem* groupItem = groupItems.value(groupKey, nullptr);
        if (!groupItem) {
            groupItem = new QTreeWidgetItem(m_aiFeedbackTree, QStringList() << feedbackGroupTitle(feedback.commitHash, currentWorkHash, m_commits));
            groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsUserCheckable);
            groupItem->setData(0, IsDirectoryRole, true);
            groupItem->setToolTip(0, feedback.commitHash.isEmpty() ? AppText::get("feedback.unlinkedVersion") : feedback.commitHash);
            groupItems.insert(groupKey, groupItem);
        }

        const QString title = feedback.summary.isEmpty() ? AppText::get("feedback.record") : feedback.summary;
        auto* item = new QTreeWidgetItem(groupItem, QStringList() << title);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Checked);
        item->setData(0, IsDirectoryRole, true);
        item->setData(0, NoteRole, feedback.rawContent);
        item->setToolTip(0, displayFeedbackTime(feedback.createdAt));

        int issueIndex = 1;
        for (const FeedbackItem& feedbackItem : feedback.items) {
            const QString issueTitle = feedbackItem.title.isEmpty() ? AppText::get("feedback.issueNumber").arg(issueIndex) : feedbackItem.title;
            auto* issueNode = new QTreeWidgetItem(item, QStringList() << issueTitle);
            issueNode->setFlags(issueNode->flags() | Qt::ItemIsUserCheckable);
            issueNode->setCheckState(0, Qt::Checked);
            issueNode->setData(0, IsDirectoryRole, false);
            const QString location = feedbackItem.filePath.isEmpty()
                                         ? AppText::get("feedback.noLocation")
                                         : QString("%1%2").arg(feedbackItem.filePath, feedbackItem.line >= 0 ? QString(":%1").arg(feedbackItem.line) : QString());
            issueNode->setData(0, NoteRole,
                               AppText::get("feedback.issueTemplate")
                                   .arg(issueTitle,
                                        feedbackItem.severity.isEmpty() ? "medium" : feedbackItem.severity,
                                        feedbackItem.category.isEmpty() ? "other" : feedbackItem.category,
                                        location,
                                        feedbackItem.suggestion.isEmpty() ? AppText::get("feedback.noSuggestion") : feedbackItem.suggestion));
            issueNode->setToolTip(0, issueTitle);
            ++issueIndex;
        }
    }

    m_updatingAiFeedbackTree = false;
}

void MainWindow::selectAllAiFeedbackRecords() {
    if (!m_aiFeedbackTree)
        return;

    m_updatingAiFeedbackTree = true;
    QTreeWidgetItem* root = m_aiFeedbackTree->invisibleRootItem();
    for (int groupIndex = 0; groupIndex < root->childCount(); ++groupIndex) {
        QTreeWidgetItem* groupItem = root->child(groupIndex);
        for (int recordIndex = 0; recordIndex < groupItem->childCount(); ++recordIndex) {
            QTreeWidgetItem* item = groupItem->child(recordIndex);
            if (item->flags() & Qt::ItemIsUserCheckable) {
                item->setCheckState(0, Qt::Checked);
                setTreeChildrenCheckState(item, Qt::Checked);
            }
        }
    }
    m_updatingAiFeedbackTree = false;
}

void MainWindow::handleAiFeedbackItemChanged(QTreeWidgetItem* item, int column) {
    if (!item || column != 0 || m_updatingAiFeedbackTree)
        return;

    m_updatingAiFeedbackTree = true;
    if (item->childCount() > 0 && item->checkState(0) != Qt::PartiallyChecked)
        setTreeChildrenCheckState(item, item->checkState(0));

    QTreeWidgetItem* parent = item->parent();
    if (parent && (parent->flags() & Qt::ItemIsUserCheckable)) {
        int checkedCount = 0;
        int partialCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            const Qt::CheckState state = parent->child(i)->checkState(0);
            if (state == Qt::Checked)
                ++checkedCount;
            else if (state == Qt::PartiallyChecked)
                ++partialCount;
        }

        if (checkedCount == parent->childCount())
            parent->setCheckState(0, Qt::Checked);
        else if (checkedCount == 0 && partialCount == 0)
            parent->setCheckState(0, Qt::Unchecked);
        else
            parent->setCheckState(0, Qt::PartiallyChecked);
    }
    m_updatingAiFeedbackTree = false;
}

void MainWindow::handleFeedbackTreeSelection(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current)
        return;

    const QString detailHtml = current->data(0, DetailHtmlRole).toString();
    if (!detailHtml.isEmpty())
        m_reviewReport->setHtml(detailHtml);
}

void MainWindow::saveFeedbackToVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    const QString feedback = m_lastAiReply.trimmed().isEmpty() ? m_responseView->toPlainText().trimmed() : m_lastAiReply.trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(this, AppText::get("dialog.noFeedback.title"), AppText::get("dialog.noFeedback.body"));
        return;
    }

    FeedbackRecord record = buildFeedbackRecord(feedback);

    QString errorMessage;
    if (!m_feedbackStore.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(this, AppText::get("dialog.saveFailed"), errorMessage);
        return;
    }

    m_statusLabel->setText(AppText::get("status.savedFeedback"));
    m_lastAiReply.clear();
    m_responseView->clear();
    m_questionEdit->clear();
    m_responseView->setPlaceholderText(AppText::get("placeholder.savedFeedback"));

    refreshProjectPanel();
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        if (item->data(0, CommitHashRole).toString().isEmpty()) {
            m_versionTree->setCurrentItem(item);
            break;
        }
    }
    updateCurrentVersionPanel();
}
