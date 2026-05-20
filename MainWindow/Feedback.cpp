#include "../MainWindow.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>

#include "Render.h"

using namespace MainWindowRender;

FeedbackRecord MainWindow::buildFeedbackRecord(const QString& replyText) const {
    FeedbackRecord record;
    record.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    record.commitHash = feedbackContextHash();
    record.mode = currentModeName();
    record.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    record.rawContent = replyText;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(extractJsonObjectText(replyText).toUtf8(), &parseError);
    if (!document.isObject()) {
        record.parseStatus = "raw";
        record.summary = "未能解析结构化 JSON，已保存原始回复。";
        return record;
    }

    const QJsonObject object = document.object();
    record.parseStatus = "parsed";
    record.summary = object["summary"].toString("AI 已返回结构化反馈。");
    record.rawContent = object["rawReport"].toString(replyText);

    const QJsonArray items = object["items"].toArray();
    int index = 1;
    for (const QJsonValue& value : items) {
        if (!value.isObject())
            continue;

        FeedbackItem item = FeedbackItem::fromJson(value.toObject());
        item.id = QString("%1-item-%2").arg(record.id).arg(index++);
        if (item.status.isEmpty())
            item.status = "open";
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
        m_reviewReport->setHtml("<h2 style=\"margin:0 0 8px 0; color:#111827;\">反馈记录</h2><p style=\"color:#64748b;\">当前版本还没有保存反馈记录。</p>");
        return;
    }

    for (const FeedbackRecord& feedback : feedbacks) {
        const QString title = QString("%1  |  %2").arg(feedback.createdAt, feedback.summary.isEmpty() ? "反馈记录" : feedback.summary);
        auto* recordItem = new QTreeWidgetItem(m_feedbackTree, QStringList() << title << QString("%1 项").arg(feedback.items.size()) << feedback.mode << feedback.parseStatus);
        recordItem->setData(0, DetailHtmlRole, renderFeedbackRecordDetail(feedback));

        for (const FeedbackItem& item : feedback.items) {
            const QString location = item.filePath.isEmpty()
                                         ? QString()
                                         : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
            auto* itemNode = new QTreeWidgetItem(recordItem, QStringList() << (item.title.isEmpty() ? "未命名问题" : item.title) << item.severity << location << item.status);
            itemNode->setData(0, DetailHtmlRole, renderFeedbackItemDetail(item));
        }
    }

    m_feedbackTree->expandAll();
    for (int column = 1; column < m_feedbackTree->columnCount(); ++column)
        m_feedbackTree->resizeColumnToContents(column);

    if (m_feedbackTree->topLevelItemCount() > 0)
        m_feedbackTree->setCurrentItem(m_feedbackTree->topLevelItem(0));
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
        QMessageBox::information(this, "尚未打开项目", "请先打开一个作业项目文件夹。");
        return;
    }

    const QString feedback = m_responseView->toPlainText().trimmed();
    if (feedback.isEmpty()) {
        QMessageBox::information(this, "没有反馈", "当前没有可保存的 AI 回复。");
        return;
    }

    FeedbackRecord record = buildFeedbackRecord(feedback);

    QString errorMessage;
    if (!m_feedbackStore.addFeedback(record, &errorMessage)) {
        QMessageBox::warning(this, "保存失败", errorMessage);
        return;
    }

    m_statusLabel->setText("已保存为一条新的反馈记录");
    m_responseView->clear();
    m_questionEdit->clear();
    m_responseView->setPlaceholderText("已保存为反馈记录。新的文件分析回复会显示在这里。");

    refreshProjectPanel();
    updateCurrentVersionPanel();
}
