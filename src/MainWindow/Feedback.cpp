#include "MainWindowPrivate.h"

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
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>
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
    if (status == "uncertain" || status == "无法确定")
        return "uncertain";
    if (status == "ignored" || status == "ignore" || status == "忽略")
        return "ignored";
    return "unresolved";
}

QString feedbackStatusText(const QString& status) {
    const QString normalized = normalizedFeedbackStatus(status);
    if (normalized == "resolved")
        return AppText::get("feedback.resolved");
    if (normalized == "uncertain")
        return AppText::get("feedback.uncertain");
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

void setFeedbackRowHeight(QTreeWidgetItem* item) {
    static constexpr int RowHeight = 44;
    for (int column = 0; column < item->columnCount(); ++column)
        item->setSizeHint(column, QSize(-1, RowHeight));
}

QString feedbackStatusComboStyle(const QString& status) {
    const QString normalizedStatus = normalizedFeedbackStatus(status);
    const QString comboColor = normalizedStatus == "ignored" ? "#94a3b8" : "#243244";
    return QString(
        "QComboBox { background: transparent; color: %1; border: 0; padding: 2px 24px 2px 10px; min-height: 24px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid transparent; background: transparent; }"
        "QComboBox QAbstractItemView { min-width: 96px; background: #ffffff; color: #243244; border: 1px solid #94a3b8; border-radius: 8px; padding: 4px; selection-background-color: #dbeafe; selection-color: #0f172a; }")
        .arg(comboColor);
}

QString feedbackStatusCellStyle(const QString& status) {
    const QString normalizedStatus = normalizedFeedbackStatus(status);
    const bool ignored = normalizedStatus == "ignored";
    QString accentColor = "#ffffff";
    QString dividerColor = "#dbe3ee";
    if (normalizedStatus == "unresolved") {
        accentColor = "#fee2e2";
        dividerColor = "#fca5a5";
    } else if (normalizedStatus == "resolved") {
        accentColor = "#dcfce7";
        dividerColor = "#86efac";
    } else if (normalizedStatus == "uncertain") {
        accentColor = "#fef3c7";
        dividerColor = "#fcd34d";
    }
    const QString borderColor = ignored ? "#cbd5e1" : "#94a3b8";
    const QString background = ignored
                                   ? QString("#ffffff")
                                   : QString("qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffffff, stop:0.79 #ffffff, stop:0.80 %1, stop:1 %2)")
                                         .arg(dividerColor, accentColor);
    return QString(
        "QWidget#StatusCell { background: %1; border: 1px solid %2; border-radius: 8px; }"
        "QWidget#StatusCell:hover { background: %1; border-color: #2563eb; }")
        .arg(background, borderColor);
}

QString displayFeedbackTime(QString createdAt) {
    return createdAt.replace('T', ' ');
}

QString feedbackGroupTitle(const QString& commitHash, const QString& currentWorkHash, const QList<GitCommit>& commits) {
    Q_UNUSED(currentWorkHash);
    if (commitHash == "working-tree")
        return AppText::get("feedback.currentWorkspace");

    for (const GitCommit& commit : commits) {
        if (commit.hash == commitHash)
            return commit.subject.trimmed().isEmpty() ? AppText::get("feedback.untitledCommit") : commit.subject.trimmed();
    }

    return commitHash.isEmpty() ? AppText::get("feedback.unlinkedVersion") : AppText::get("feedback.unknownVersion").arg(commitHash.left(8));
}

bool isHeuristicRecord(const FeedbackRecord& feedback) {
    const QString heuristicMode = AppText::get("label.heuristicQuestions");
    return feedback.mode == heuristicMode || feedback.mode == "Guiding Questions" || feedback.mode == "启发式问题";
}

bool isReviewRecord(const FeedbackRecord& feedback) {
    const QString reviewMode = AppText::get("label.feedbackReview");
    return feedback.mode == reviewMode || feedback.mode == "Feedback Review" || feedback.mode == "反馈复查";
}

void configureFeedbackTreeColumns(QTreeWidget* tree, bool showingHeuristicRecords) {
    if (showingHeuristicRecords) {
        tree->setColumnCount(1);
        tree->setHeaderLabels({AppText::get("label.heuristicQuestions")});
        tree->header()->setStretchLastSection(true);
        return;
    }

    tree->setColumnCount(4);
    tree->setHeaderLabels({AppText::get("label.feedbackIssue"), AppText::get("label.severity"), AppText::get("label.location"), AppText::get("label.status")});
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    tree->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    tree->setColumnWidth(1, 110);
    tree->setColumnWidth(2, 240);
    tree->setColumnWidth(3, 126);
}
}  // namespace

FeedbackRecord MainWindowPrivate::buildFeedbackRecord(const QString& replyText) const {
    FeedbackRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.commitHash = feedbackSaveContextHash();
    record.mode = m_pendingFeedbackMode.isEmpty() ? currentModeName() : m_pendingFeedbackMode;
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
        item.sourceFeedbackId = value.toObject()["sourceFeedbackId"].toString(item.sourceFeedbackId);
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

void MainWindowPrivate::populateFeedbackPanel(const QList<FeedbackRecord>& feedbacks) {
    m_feedbackTree->clear();
    if (m_openReviewRecordButton)
        m_openReviewRecordButton->setVisible(false);
    configureFeedbackTreeColumns(m_feedbackTree, m_showingHeuristicRecords);
    if (m_feedbackIssuesButton)
        m_feedbackIssuesButton->setChecked(!m_showingHeuristicRecords && !m_showingReviewRecords);
    if (m_feedbackReviewButton)
        m_feedbackReviewButton->setChecked(m_showingReviewRecords);
    if (m_heuristicQuestionsButton)
        m_heuristicQuestionsButton->setChecked(m_showingHeuristicRecords);

    QList<FeedbackRecord> visibleFeedbacks;
    for (const FeedbackRecord& feedback : feedbacks) {
        const bool isHeuristic = isHeuristicRecord(feedback);
        const bool isReview = isReviewRecord(feedback);
        if (m_showingHeuristicRecords && isHeuristic)
            visibleFeedbacks.append(feedback);
        else if (m_showingReviewRecords && isReview)
            visibleFeedbacks.append(feedback);
        else if (!m_showingHeuristicRecords && !m_showingReviewRecords && !isHeuristic && !isReview)
            visibleFeedbacks.append(feedback);
    }

    if (visibleFeedbacks.isEmpty()) {
        m_reviewReport->setHtml(QString("<h2 style=\"margin:0 0 8px 0; color:#111827;\">%1</h2><p style=\"color:#64748b;\">%2</p>")
                                     .arg(htmlEscape(AppText::get("feedback.record")), htmlEscape(AppText::get("feedback.noRecordsForVersion"))));
        return;
    }

    for (const FeedbackRecord& feedback : visibleFeedbacks) {
        const QString title = feedback.summary.isEmpty() ? AppText::get("feedback.record") : feedback.summary;
        auto* recordItem = m_showingHeuristicRecords
                               ? new QTreeWidgetItem(m_feedbackTree, QStringList() << title)
                               : new QTreeWidgetItem(m_feedbackTree, QStringList() << title << AppText::get("feedback.issueCount").arg(feedback.items.size()) << feedback.mode << feedback.parseStatus);
        setFeedbackRowHeight(recordItem);
        recordItem->setData(0, DetailHtmlRole, renderFeedbackRecordDetail(feedback));
        recordItem->setData(0, RecordIdRole, feedback.id);
        recordItem->setToolTip(0, displayFeedbackTime(feedback.createdAt));

        QList<FeedbackItem> sortedItems = feedback.items;
        std::stable_sort(sortedItems.begin(), sortedItems.end(), [](const FeedbackItem& left, const FeedbackItem& right) {
            return normalizedFeedbackStatus(left.status) != "ignored" && normalizedFeedbackStatus(right.status) == "ignored";
        });

        for (const FeedbackItem& item : sortedItems) {
            if (m_showingHeuristicRecords) {
                auto* itemNode = new QTreeWidgetItem(recordItem, QStringList() << (item.title.isEmpty() ? AppText::get("feedback.unnamedIssue") : item.title));
                setFeedbackRowHeight(itemNode);
                itemNode->setData(0, DetailHtmlRole, renderHeuristicItemDetail(item));
                itemNode->setData(0, ItemIdRole, item.id);
                itemNode->setToolTip(0, item.suggestion);
                continue;
            }

            const QString itemStatus = normalizedFeedbackStatus(item.status);
            const QString location = item.filePath.isEmpty()
                                         ? QString()
                                         : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
            auto* itemNode = new QTreeWidgetItem(recordItem, QStringList() << (item.title.isEmpty() ? AppText::get("feedback.unnamedIssue") : item.title) << item.severity << location << feedbackStatusText(itemStatus));
            setFeedbackRowHeight(itemNode);
            itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
            itemNode->setData(0, ItemIdRole, item.id);
            itemNode->setData(0, ReviewRecordIdRole, item.reviewRecordId);
            itemNode->setData(3, Qt::UserRole, itemStatus);
            applyFeedbackStatusStyle(itemNode, itemStatus);

            auto* statusCell = new QWidget(m_feedbackTree);
            statusCell->setObjectName("StatusCell");
            statusCell->setStyleSheet(feedbackStatusCellStyle(itemStatus));
            statusCell->setMinimumHeight(44);
            auto* statusCellLayout = new QHBoxLayout(statusCell);
            statusCellLayout->setContentsMargins(0, 0, 0, 0);
            statusCellLayout->setSpacing(0);

            auto* statusCombo = new QComboBox(statusCell);
            statusCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            statusCombo->addItem(AppText::get("feedback.unresolved"), "unresolved");
            statusCombo->addItem(AppText::get("feedback.resolved"), "resolved");
            statusCombo->addItem(AppText::get("feedback.uncertain"), "uncertain");
            statusCombo->addItem(AppText::get("feedback.ignored"), "ignored");
            statusCombo->setCurrentIndex(statusCombo->findData(itemStatus));
            statusCombo->setStyleSheet(feedbackStatusComboStyle(itemStatus));
            statusCellLayout->addWidget(statusCombo);
            connect(statusCombo, QOverload<int>::of(&QComboBox::activated), this, [this, itemNode, item = item, statusCell, statusCombo](int index) mutable {
                const QString status = statusCombo->itemData(index).toString();
                QString errorMessage;
                if (!m_feedbackStore.updateItemStatus(item.id, status, &errorMessage)) {
                    QMessageBox::warning(q, AppText::get("feedback.statusUpdateFailed"), errorMessage);
                    return;
                }

                const int scrollPosition = m_feedbackTree->verticalScrollBar()->value();
                item.status = status;
                itemNode->setText(3, feedbackStatusText(status));
                itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
                itemNode->setData(3, Qt::UserRole, status);
                applyFeedbackStatusStyle(itemNode, status);
                statusCell->setStyleSheet(feedbackStatusCellStyle(status));
                statusCombo->setStyleSheet(feedbackStatusComboStyle(status));

                m_feedbackTree->verticalScrollBar()->setValue(scrollPosition);
                if (m_feedbackTree->currentItem() == itemNode)
                    m_reviewReport->setHtml(itemNode->data(0, DetailHtmlRole).toString());
                m_statusLabel->setText(AppText::get("status.feedbackStatusUpdated").arg(feedbackStatusText(status)));
            });
            m_feedbackTree->setItemWidget(itemNode, 3, statusCell);
        }
    }

    if (m_feedbackTree->topLevelItemCount() > 0)
        m_feedbackTree->setCurrentItem(m_feedbackTree->topLevelItem(0));
}

void MainWindowPrivate::showFeedbackIssueRecords() {
    m_showingHeuristicRecords = false;
    m_showingReviewRecords = false;
    if (m_feedbackIssuesButton)
        m_feedbackIssuesButton->setChecked(true);
    if (m_feedbackReviewButton)
        m_feedbackReviewButton->setChecked(false);
    if (m_heuristicQuestionsButton)
        m_heuristicQuestionsButton->setChecked(false);
    updateCurrentVersionPanel();
}

void MainWindowPrivate::showFeedbackReviewRecords() {
    m_showingHeuristicRecords = false;
    m_showingReviewRecords = true;
    if (m_feedbackIssuesButton)
        m_feedbackIssuesButton->setChecked(false);
    if (m_feedbackReviewButton)
        m_feedbackReviewButton->setChecked(true);
    if (m_heuristicQuestionsButton)
        m_heuristicQuestionsButton->setChecked(false);
    updateCurrentVersionPanel();
}

void MainWindowPrivate::showHeuristicQuestionRecords() {
    m_showingHeuristicRecords = true;
    m_showingReviewRecords = false;
    if (m_feedbackIssuesButton)
        m_feedbackIssuesButton->setChecked(false);
    if (m_feedbackReviewButton)
        m_feedbackReviewButton->setChecked(false);
    if (m_heuristicQuestionsButton)
        m_heuristicQuestionsButton->setChecked(true);
    updateCurrentVersionPanel();
}

void MainWindowPrivate::populateAiFeedbackPicker() {
    if (!m_aiFeedbackTree)
        return;

    const QStringList previouslySelectedItemIds = selectedFeedbackItemIds();
    m_updatingAiFeedbackTree = true;
    m_aiFeedbackTree->clear();
    const QList<FeedbackRecord> feedbacks = m_feedbackStore.allFeedbacks();

    const QString currentWorkHash = currentWorkContextHash();
    bool hasVisibleFeedback = false;
    QMap<QString, QTreeWidgetItem*> groupItems;
    for (const FeedbackRecord& feedback : feedbacks) {
        if (isHeuristicRecord(feedback) || isReviewRecord(feedback))
            continue;

        QList<FeedbackItem> visibleItems;
        for (const FeedbackItem& feedbackItem : feedback.items) {
            const QString status = normalizedFeedbackStatus(feedbackItem.status);
            if (status == "unresolved" || status == "uncertain")
                visibleItems.append(feedbackItem);
        }
        if (visibleItems.isEmpty() && !feedback.items.isEmpty())
            continue;

        const QString groupKey = feedback.commitHash.isEmpty() ? QString("untracked") : feedback.commitHash;
        const bool isCurrentWorkFeedback = feedback.commitHash == "working-tree";
        QTreeWidgetItem* groupItem = groupItems.value(groupKey, nullptr);
        if (!groupItem) {
            groupItem = new QTreeWidgetItem(m_aiFeedbackTree, QStringList() << feedbackGroupTitle(feedback.commitHash, currentWorkHash, m_commits));
            groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsUserCheckable);
            groupItem->setData(0, IsDirectoryRole, true);
            groupItem->setToolTip(0, feedback.commitHash.isEmpty() ? AppText::get("feedback.unlinkedVersion") : feedback.commitHash);
            groupItem->setExpanded(isCurrentWorkFeedback);
            groupItems.insert(groupKey, groupItem);
        }

        const QString title = feedback.summary.isEmpty() ? AppText::get("feedback.record") : feedback.summary;
        auto* item = new QTreeWidgetItem(groupItem, QStringList() << title);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Unchecked);
        item->setData(0, IsDirectoryRole, true);
        item->setData(0, RecordIdRole, feedback.id);
        item->setToolTip(0, displayFeedbackTime(feedback.createdAt));
        item->setExpanded(isCurrentWorkFeedback);

        QString visibleRecordContext;
        int issueIndex = 1;
        for (const FeedbackItem& feedbackItem : visibleItems) {
            const QString issueTitle = feedbackItem.title.isEmpty() ? AppText::get("feedback.issueNumber").arg(issueIndex) : feedbackItem.title;
            auto* issueNode = new QTreeWidgetItem(item, QStringList() << issueTitle);
            issueNode->setFlags(issueNode->flags() | Qt::ItemIsUserCheckable);
            issueNode->setCheckState(0, previouslySelectedItemIds.contains(feedbackItem.id) ? Qt::Checked : Qt::Unchecked);
            issueNode->setData(0, IsDirectoryRole, false);
            issueNode->setData(0, ItemIdRole, feedbackItem.id);
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
            visibleRecordContext += QString("- %1\n反馈条目ID：%2\n%3\n\n")
                                        .arg(issueTitle, feedbackItem.id, issueNode->data(0, NoteRole).toString());
            ++issueIndex;
        }
        item->setData(0, NoteRole, feedback.items.isEmpty() ? feedback.rawContent : visibleRecordContext.trimmed());
        updateParentCheckState(item);
        hasVisibleFeedback = true;
    }

    if (!hasVisibleFeedback) {
        auto* emptyItem = new QTreeWidgetItem(m_aiFeedbackTree, QStringList() << AppText::get("feedback.noRecordsForProject"));
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEnabled);
    }

    m_updatingAiFeedbackTree = false;
    updateAiActionText();
}

void MainWindowPrivate::selectAllAiFeedbackRecords() {
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
    updateAiActionText();
}

void MainWindowPrivate::handleAiFeedbackItemChanged(QTreeWidgetItem* item, int column) {
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
    updateAiActionText();
}

void MainWindowPrivate::handleFeedbackTreeSelection(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current)
        return;

    const QString detailHtml = current->data(0, DetailHtmlRole).toString();
    if (!detailHtml.isEmpty())
        m_reviewReport->setHtml(detailHtml);

    const QString reviewRecordId = current->data(0, ReviewRecordIdRole).toString();
    if (m_openReviewRecordButton) {
        m_openReviewRecordButton->setProperty("reviewRecordId", reviewRecordId);
        m_openReviewRecordButton->setVisible(!reviewRecordId.isEmpty());
    }
}

void MainWindowPrivate::showFeedbackTreeContextMenu(const QPoint& position) {
    QTreeWidgetItem* item = m_feedbackTree->itemAt(position);
    if (!item || item->parent())
        return;

    const QString recordId = item->data(0, RecordIdRole).toString();
    if (recordId.isEmpty())
        return;

    QMenu menu(m_feedbackTree);
    QAction* renameAction = menu.addAction(AppText::get("menu.rename"));
    QAction* selectedAction = menu.exec(m_feedbackTree->viewport()->mapToGlobal(position));
    if (selectedAction != renameAction)
        return;

    bool accepted = false;
    const QString currentName = item->text(0);
    const QString newName = QInputDialog::getText(q,
                                                  AppText::get("dialog.renameFeedback.title"),
                                                  AppText::get("dialog.renameFeedback.label"),
                                                  QLineEdit::Normal,
                                                  currentName,
                                                  &accepted)
                                .trimmed();
    if (!accepted || newName.isEmpty() || newName == currentName)
        return;

    QString errorMessage;
    if (!m_feedbackStore.renameFeedbackRecord(recordId, newName, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.saveFailed"), errorMessage);
        return;
    }

    updateCurrentVersionPanel();
}

void MainWindowPrivate::saveFeedbackToVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    const QString feedback = m_lastAiReply.trimmed().isEmpty() ? m_responseView->toPlainText().trimmed() : m_lastAiReply.trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noFeedback.title"), AppText::get("dialog.noFeedback.body"));
        return;
    }

    const bool isReviewRecord = m_lastAiModeName == AppText::get("label.feedbackReview");
    const QStringList reviewedItemIds = isReviewRecord ? selectedFeedbackItemIds() : QStringList();

    m_pendingFeedbackMode = m_lastAiModeName.isEmpty() ? currentModeName() : m_lastAiModeName;
    FeedbackRecord record = buildFeedbackRecord(feedback);
    m_pendingFeedbackMode.clear();

    if (isReviewRecord) {
        for (int i = 0; i < record.items.size() && i < reviewedItemIds.size(); ++i) {
            if (record.items[i].sourceFeedbackId.isEmpty())
                record.items[i].sourceFeedbackId = reviewedItemIds.at(i);
        }
    }

    QString errorMessage;
    if (!m_feedbackStore.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.saveFailed"), errorMessage);
        return;
    }
    if (isReviewRecord && !m_feedbackStore.markItemsReviewed(reviewedItemIds, record.id, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.saveFailed"), errorMessage);
        return;
    }
    if (isReviewRecord && !reviewedItemIds.isEmpty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            q,
            AppText::get("dialog.markReviewedResolved.title"),
            AppText::get("dialog.markReviewedResolved.body"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer == QMessageBox::Yes && !m_feedbackStore.updateItemsStatus(reviewedItemIds, "resolved", &errorMessage)) {
            QMessageBox::warning(q, AppText::get("dialog.saveFailed"), errorMessage);
            return;
        }
    }

    m_statusLabel->setText(AppText::get("status.savedFeedback"));
    m_lastAiReply.clear();
    m_lastAiModeName.clear();
    m_responseView->clear();
    m_questionEdit->clear();
    m_responseView->setPlaceholderText(AppText::get("placeholder.savedFeedback"));

    refreshProjectPanel();
    const QString savedCommitHash = record.commitHash;
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        const QString itemCommitHash = item->data(0, CommitHashRole).toString();
        if (itemCommitHash == savedCommitHash || (savedCommitHash == "working-tree" && itemCommitHash.isEmpty())) {
            m_versionTree->setCurrentItem(item);
            break;
        }
    }
    updateCurrentVersionPanel();
}

void MainWindowPrivate::selectFeedbackRecordById(const QString& recordId) {
    if (recordId.isEmpty() || !m_feedbackTree)
        return;

    m_showingHeuristicRecords = false;
    m_showingReviewRecords = true;
    updateCurrentVersionPanel();

    for (int i = 0; i < m_feedbackTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* recordItem = m_feedbackTree->topLevelItem(i);
        if (recordItem->data(0, RecordIdRole).toString() == recordId) {
            m_feedbackTree->setCurrentItem(recordItem);
            recordItem->setExpanded(true);
            m_tabs->setCurrentIndex(3);
            return;
        }
    }
}

void MainWindowPrivate::saveHeuristicToVersion() {
    if (m_projectDir.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noProject.title"), AppText::get("dialog.noProject.body"));
        return;
    }

    const QString feedback = m_lastHeuristicReply.trimmed().isEmpty() ? m_heuristicResponseView->toPlainText().trimmed() : m_lastHeuristicReply.trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noFeedback.title"), AppText::get("dialog.noFeedback.body"));
        return;
    }

    m_pendingFeedbackMode = heuristicModeName();
    FeedbackRecord record = buildFeedbackRecord(feedback);
    m_pendingFeedbackMode.clear();

    QString errorMessage;
    if (!m_feedbackStore.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(q, AppText::get("dialog.saveFailed"), errorMessage);
        return;
    }

    m_statusLabel->setText(AppText::get("status.savedFeedback"));
    m_lastHeuristicReply.clear();
    m_heuristicResponseView->clear();
    m_heuristicQuestionEdit->clear();
    m_heuristicResponseView->setPlaceholderText(AppText::get("placeholder.savedFeedback"));

    refreshProjectPanel();
    m_showingHeuristicRecords = true;
    const QString savedCommitHash = record.commitHash;
    for (int i = 0; i < m_versionRoot->childCount(); ++i) {
        QTreeWidgetItem* item = m_versionRoot->child(i);
        const QString itemCommitHash = item->data(0, CommitHashRole).toString();
        if (itemCommitHash == savedCommitHash || (savedCommitHash == "working-tree" && itemCommitHash.isEmpty())) {
            m_versionTree->setCurrentItem(item);
            break;
        }
    }
    updateCurrentVersionPanel();
}
