#include "../MainWindow.h"

#include <QCheckBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>

#include "../HWpilotLLM/HWpilotLLM.h"
#include "Render.h"

using namespace MainWindowRender;

QString MainWindow::currentFeedbackHistory() const {
    QString history;
    const QString commitHash = feedbackContextHash();
    const QList<FeedbackRecord> records = m_feedbackStore.feedbacksForCommit(commitHash);
    for (const FeedbackRecord& record : records) {
        history += QString("### %1 %2\n%3\n%4\n\n").arg(record.createdAt, record.mode, record.summary, record.rawContent);
    }
    return history.trimmed();
}

QString MainWindow::currentModePrompt() const {
    return
        "你是一位严谨、耐心的程序设计课助教。"
        "用户会提供作业要求、代码文件、当前 Git 变更和补充问题。"
        "请聚焦于这次作业代码本身，分析可能的 Bug、边界条件、代码质量和实现建议。"
        "如果信息不足，请明确说明需要补充什么。";
}

QString MainWindow::currentModeName() const {
    return "文件分析";
}

double MainWindow::currentTemperature() const {
    return 0.3;
}

void MainWindow::startAiAnalysis() {
    QList<CodeFile> filesForAnalysis;
    if (m_includeCodeCheck->isChecked()) {
        filesForAnalysis = selectedFiles();
        if (filesForAnalysis.isEmpty()) {
            QMessageBox::information(this, "没有可分析的文件", "请先打开项目并勾选至少一个文件，或取消“包含勾选的代码文件”。");
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
            QMessageBox::warning(this, "文件内容为空", "已勾选文件，但没有读取到任何文件内容。请重新扫描项目，或切换到其它提交后再试。");
            return;
        }
    }

    const QString apiKey = qEnvironmentVariable("DEEPSEEK_API_KEY");
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "缺少 API Key", "请先设置环境变量 DEEPSEEK_API_KEY，再使用文件分析功能。");
        return;
    }

    QString userContent;
    const QString task = m_taskEdit->toPlainText().trimmed();
    const QString question = m_questionEdit->toPlainText().trimmed();
    if (!question.isEmpty()) {
        userContent += "【用户问题】\n" + question + "\n\n";
    }

    m_projectManager.data().assignmentText = task;
    QString errorMessage;
    m_projectManager.save(&errorMessage);

    if (m_includeHistoryCheck->isChecked()) {
        const QString feedback = currentFeedbackHistory();
        if (!feedback.isEmpty()) {
            userContent += "【当前版本历史反馈记录】\n" + feedback + "\n\n";
        }
    }

    const QString selectedCommit = selectedCommitHash();
    const QString gitDiff = selectedCommit.isEmpty() ? m_gitService.diff().trimmed() : m_gitService.diffForCommit(selectedCommit).trimmed();
    if (!gitDiff.isEmpty()) {
        userContent += selectedCommit.isEmpty() ? "【当前 Git 变更 diff】\n" : "【当前提交相对于上一个提交的 diff】\n";
        userContent += gitDiff + "\n\n";
    }

    if (selectedCommit.isEmpty()) {
        const QString gitStatus = m_gitService.statusPorcelain().trimmed();
        if (!gitStatus.isEmpty()) {
            userContent += "【当前 Git status】\n";
            userContent += gitStatus + "\n\n";
        }
    }

    if (m_includeCodeCheck->isChecked()) {
        userContent += HWFileScanner::formatFilesForLLM(filesForAnalysis, task);
    } else if (!task.isEmpty()) {
        userContent += "【作业要求 / 任务描述】\n" + task + "\n\n";
    }

    if (userContent.trimmed().isEmpty()) {
        QMessageBox::information(this, "内容为空", "请填写问题、作业要求，或勾选代码文件。");
        return;
    }

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = currentModePrompt() + "\n\n" + structuredFeedbackInstruction();
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userContent;
    messages.append(userMsg);

    if (m_llm)
        m_llm->deleteLater();
    m_llm = new HWpilotLLM(apiKey, this);

    connect(m_llm, &HWpilotLLM::responseReceived, this, [this](const QString& replyText) {
        m_responseView->setPlainText(replyText);
        setBusy(false);
        m_statusLabel->setText("文件分析完成");
    });
    connect(m_llm, &HWpilotLLM::errorOccurred, this, [this](const QString& errorString) {
        m_responseView->setPlainText("网络/API 错误：\n" + errorString);
        setBusy(false);
        m_statusLabel->setText("文件分析失败");
    });

    setBusy(true);
    m_responseView->setPlainText("正在向 DeepSeek 发送请求，请稍候...");
    m_llm->sendChatRequest(messages, currentTemperature());
}

void MainWindow::setBusy(bool busy) {
    m_analyzeButton->setDisabled(busy);
    m_saveFeedbackButton->setDisabled(busy);
    m_refreshProjectButton->setDisabled(busy);
    m_commitButton->setDisabled(busy);
    m_statusLabel->setText(busy ? "正在进行文件分析..." : "就绪");
}

