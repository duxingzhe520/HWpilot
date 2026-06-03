#include "MainWindowPrivate.h"

#include "../AppText.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTreeWidget>

#include "../HWpilotLLM/HWpilotLLM.h"
#include "Render.h"

using namespace MainWindowRender;

namespace {
QString readableAiReply(const QString& replyText) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(extractJsonObjectText(replyText).toUtf8(), &parseError);
    if (!document.isObject())
        return replyText;

    const QJsonObject object = document.object();
    const QString rawReport = object["rawReport"].toString().trimmed();
    if (!rawReport.isEmpty())
        return rawReport;

    QString readable;
    const QString summary = object["summary"].toString().trimmed();
    if (!summary.isEmpty())
        readable += summary + "\n\n";

    const QJsonArray items = object["items"].toArray();
    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject item = items.at(i).toObject();
        const QString title = item["title"].toString(AppText::get("feedback.issueNumber").arg(i + 1)).trimmed();
        const QString suggestion = item["suggestion"].toString().trimmed();
        readable += QString("%1. %2\n").arg(i + 1).arg(title);
        if (!suggestion.isEmpty())
            readable += suggestion + "\n";
        readable += "\n";
    }

    return readable.trimmed().isEmpty() ? replyText : readable.trimmed();
}
}  // namespace

QString MainWindowPrivate::currentFeedbackHistory() const {
    QString history;
    const QString commitHash = feedbackContextHash();
    const QList<FeedbackRecord> records = m_feedbackStore.feedbacksForCommit(commitHash);
    for (const FeedbackRecord& record : records) {
        history += QString("### %1 %2\n%3\n%4\n\n").arg(record.createdAt, record.mode, record.summary, record.rawContent);
    }
    return history.trimmed();
}

QString MainWindowPrivate::selectedFeedbackHistory() const {
    if (!m_aiFeedbackTree)
        return QString();

    QString history;
    QTreeWidgetItem* root = m_aiFeedbackTree->invisibleRootItem();
    for (int groupIndex = 0; groupIndex < root->childCount(); ++groupIndex) {
        QTreeWidgetItem* groupItem = root->child(groupIndex);
        for (int recordIndex = 0; recordIndex < groupItem->childCount(); ++recordIndex) {
            QTreeWidgetItem* recordItem = groupItem->child(recordIndex);
            if (!(recordItem->flags() & Qt::ItemIsUserCheckable))
                continue;

            if (recordItem->checkState(0) == Qt::Checked) {
                const QString rawContent = recordItem->data(0, NoteRole).toString().trimmed();
                if (!rawContent.isEmpty())
                    history += QString("### %1 / %2\n反馈记录ID：%3\n%4\n\n")
                                   .arg(groupItem->text(0), recordItem->text(0), recordItem->data(0, RecordIdRole).toString(), rawContent);
                continue;
            }

            QString selectedIssues;
            for (int childIndex = 0; childIndex < recordItem->childCount(); ++childIndex) {
                QTreeWidgetItem* issueItem = recordItem->child(childIndex);
                if (issueItem->checkState(0) != Qt::Checked)
                    continue;

                const QString issueContent = issueItem->data(0, NoteRole).toString().trimmed();
                if (!issueContent.isEmpty())
                    selectedIssues += QString("- %1\n反馈条目ID：%2\n%3\n\n").arg(issueItem->text(0), issueItem->data(0, ItemIdRole).toString(), issueContent);
            }

            if (!selectedIssues.trimmed().isEmpty())
                history += QString("### %1 / %2 (selected issues)\n%3\n").arg(groupItem->text(0), recordItem->text(0), selectedIssues.trimmed());
        }
    }

    return history.trimmed();
}

QString MainWindowPrivate::currentModePrompt() const {
    return
        AppText::get("prompt.system");
}

QString MainWindowPrivate::currentModeName() const {
    return AppText::get("label.fileAnalysis");
}

QString MainWindowPrivate::heuristicModePrompt() const {
    return AppText::get("prompt.heuristicSystem");
}

QString MainWindowPrivate::heuristicModeName() const {
    return AppText::get("label.heuristicQuestions");
}

void MainWindowPrivate::updateAiActionText() {
    if (!m_analyzeButton)
        return;

    const bool hasSelectedFeedback = !selectedFeedbackHistory().isEmpty();
    m_analyzeButton->setText(hasSelectedFeedback ? AppText::get("button.reviewSelectedFeedback") : AppText::get("button.aiAssistant"));
}

double MainWindowPrivate::currentTemperature() const {
    return m_temperature;
}

void MainWindowPrivate::startAiAnalysis() {
    scanCurrentProject(false);
    QList<CodeFile> filesForAnalysis = selectedFiles();
    if (filesForAnalysis.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.noFiles.title"), AppText::get("dialog.noFiles.body"));
        return;
    }

    bool hasReadableContent = false;
    for (const CodeFile& file : filesForAnalysis) {
        if (!file.content.isEmpty()) {
            hasReadableContent = true;
            break;
        }
    }
    if (!hasReadableContent) {
        QMessageBox::warning(q, AppText::get("dialog.emptyFiles.title"), AppText::get("dialog.emptyFiles.body"));
        return;
    }

    const QString apiKey = m_projectManager.data().apiKey.trimmed();
    if (apiKey.isEmpty()) {
        QMessageBox::warning(q, AppText::get("dialog.noApiKey.title"), AppText::get("dialog.noApiKey.body"));
        return;
    }

    QString userContent;
    const QString task = m_taskEdit->toPlainText().trimmed();
    const QString question = m_questionEdit->toPlainText().trimmed();
    if (!question.isEmpty()) {
        userContent += AppText::get("prompt.userQuestion") + question + "\n\n";
    }

    m_projectManager.data().assignmentText = task;
    QString errorMessage;
    m_projectManager.save(&errorMessage);

    const QString feedback = selectedFeedbackHistory();
    const bool reviewingFeedback = !feedback.isEmpty();
    if (reviewingFeedback)
        userContent += AppText::get("prompt.selectedFeedback") + feedback + "\n\n";

    userContent += HWFileScanner::formatFilesForLLM(filesForAnalysis, task);

    if (userContent.trimmed().isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.emptyContent.title"), AppText::get("dialog.emptyContent.body"));
        return;
    }

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = reviewingFeedback
                               ? AppText::get("prompt.feedbackReviewSystem") + "\n\n" + AppText::get("prompt.feedbackReviewStructured")
                               : currentModePrompt() + "\n\n" + structuredFeedbackInstruction();
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userContent;
    messages.append(userMsg);

    if (m_llm)
        m_llm->deleteLater();
    m_llm = new HWpilotLLM(apiKey, this);

    connect(m_llm, &HWpilotLLM::responseReceived, this, [this](const QString& replyText) {
        m_lastAiReply = replyText;
        m_responseView->setMarkdown(readableAiReply(replyText));
        setBusy(false);
        m_statusLabel->setText(AppText::get("status.aiDone"));
    });
    connect(m_llm, &HWpilotLLM::errorOccurred, this, [this](const QString& errorString) {
        m_lastAiReply.clear();
        m_lastAiModeName.clear();
        m_responseView->setPlainText(QString("Network/API error:\n") + errorString);
        setBusy(false);
        m_statusLabel->setText(AppText::get("status.aiFailed"));
    });

    setBusy(true);
    m_lastAiReply.clear();
    m_lastAiModeName = reviewingFeedback ? AppText::get("label.feedbackReview") : currentModeName();
    m_responseView->setPlainText(AppText::get("status.aiSending"));
    m_llm->sendChatRequest(messages, currentTemperature());
}

void MainWindowPrivate::startHeuristicQuestions() {
    scanCurrentProject(false);
    QList<CodeFile> filesForAnalysis = selectedHeuristicFiles();

    const QString apiKey = m_projectManager.data().apiKey.trimmed();
    if (apiKey.isEmpty()) {
        QMessageBox::warning(q, AppText::get("dialog.noApiKey.title"), AppText::get("dialog.noApiKey.body"));
        return;
    }

    QString userContent;
    const QString task = m_heuristicTaskEdit->toPlainText().trimmed();
    const QString question = m_heuristicQuestionEdit->toPlainText().trimmed();
    if (filesForAnalysis.isEmpty() && task.isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.emptyContent.title"), AppText::get("dialog.emptyContent.body"));
        return;
    }

    if (!filesForAnalysis.isEmpty()) {
        bool hasReadableContent = false;
        for (const CodeFile& file : filesForAnalysis) {
            if (!file.content.isEmpty()) {
                hasReadableContent = true;
                break;
            }
        }
        if (!hasReadableContent) {
            QMessageBox::warning(q, AppText::get("dialog.emptyFiles.title"), AppText::get("dialog.emptyFiles.body"));
            return;
        }
    }

    if (!question.isEmpty())
        userContent += AppText::get("prompt.userQuestion") + question + "\n\n";

    if (filesForAnalysis.isEmpty()) {
        userContent += AppText::get("prompt.assignment") + task + "\n\n";
    } else {
        userContent += HWFileScanner::formatFilesForLLM(filesForAnalysis, task);
    }
    if (userContent.trimmed().isEmpty()) {
        QMessageBox::information(q, AppText::get("dialog.emptyContent.title"), AppText::get("dialog.emptyContent.body"));
        return;
    }

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = heuristicModePrompt() + "\n\n" + AppText::get("prompt.heuristicStructuredFeedback");
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userContent;
    messages.append(userMsg);

    if (m_llm)
        m_llm->deleteLater();
    m_llm = new HWpilotLLM(apiKey, this);

    connect(m_llm, &HWpilotLLM::responseReceived, this, [this](const QString& replyText) {
        m_lastHeuristicReply = replyText;
        m_heuristicResponseView->setMarkdown(readableAiReply(replyText));
        setBusy(false);
        m_statusLabel->setText(AppText::get("status.aiDone"));
    });
    connect(m_llm, &HWpilotLLM::errorOccurred, this, [this](const QString& errorString) {
        m_lastHeuristicReply.clear();
        m_heuristicResponseView->setPlainText(QString("Network/API error:\n") + errorString);
        setBusy(false);
        m_statusLabel->setText(AppText::get("status.aiFailed"));
    });

    setBusy(true);
    m_lastHeuristicReply.clear();
    m_heuristicResponseView->setPlainText(AppText::get("status.aiSending"));
    m_llm->sendChatRequest(messages, currentTemperature());
}

void MainWindowPrivate::setBusy(bool busy) {
    m_analyzeButton->setDisabled(busy);
    m_saveFeedbackButton->setDisabled(busy);
    m_cancelFeedbackButton->setDisabled(busy);
    m_refreshProjectButton->setDisabled(busy);
    m_commitButton->setDisabled(busy);
    if (m_generateHeuristicButton)
        m_generateHeuristicButton->setDisabled(busy);
    if (m_saveHeuristicButton)
        m_saveHeuristicButton->setDisabled(busy);
    if (m_cancelHeuristicButton)
        m_cancelHeuristicButton->setDisabled(busy);
    m_statusLabel->setText(busy ? AppText::get("status.aiRunning") : AppText::get("status.ready"));
}
