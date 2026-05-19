#include "FeedbackStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool FeedbackStore::openProject(const QString& projectPath, QString* errorMessage) {
    m_projectPath = QDir(projectPath).absolutePath();
    QDir projectDir(m_projectPath);
    if (!projectDir.exists()) {
        if (errorMessage)
            *errorMessage = "项目目录不存在，无法打开反馈仓库。";
        return false;
    }

    if (!projectDir.exists(".hwpilot") && !projectDir.mkpath(".hwpilot")) {
        if (errorMessage)
            *errorMessage = "无法创建 .hwpilot 目录。";
        return false;
    }

    return load(errorMessage);
}

bool FeedbackStore::load(QString* errorMessage) {
    m_feedbacks.clear();

    QFile file(feedbackFilePath());
    if (!file.exists())
        return true;

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = "无法读取反馈数据。";
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage)
            *errorMessage = "反馈数据不是有效 JSON。";
        return false;
    }

    const QJsonArray feedbackArray = document.object()["feedbacks"].toArray();
    for (const QJsonValue& value : feedbackArray) {
        if (value.isObject())
            m_feedbacks.append(FeedbackRecord::fromJson(value.toObject()));
    }

    return true;
}

bool FeedbackStore::save(QString* errorMessage) const {
    QDir projectDir(m_projectPath);
    if (!projectDir.exists(".hwpilot") && !projectDir.mkpath(".hwpilot")) {
        if (errorMessage)
            *errorMessage = "无法创建 .hwpilot 目录。";
        return false;
    }

    QJsonArray feedbackArray;
    for (const FeedbackRecord& record : m_feedbacks)
        feedbackArray.append(record.toJson());

    QJsonObject root;
    root["feedbacks"] = feedbackArray;

    QFile file(feedbackFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = "无法写入反馈数据。";
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool FeedbackStore::addFeedback(const FeedbackRecord& record, QString* errorMessage) {
    m_feedbacks.append(record);
    return save(errorMessage);
}

bool FeedbackStore::updateItemStatus(const QString& itemId, const QString& status, QString* errorMessage) {
    for (FeedbackRecord& record : m_feedbacks) {
        for (FeedbackItem& item : record.items) {
            if (item.id == itemId) {
                item.status = status;
                return save(errorMessage);
            }
        }
    }

    if (errorMessage)
        *errorMessage = "没有找到对应的问题条目。";
    return false;
}

bool FeedbackStore::reassignCommit(const QString& fromCommitHash, const QString& toCommitHash, QString* errorMessage) {
    bool changed = false;
    for (FeedbackRecord& record : m_feedbacks) {
        if (record.commitHash == fromCommitHash) {
            record.commitHash = toCommitHash;
            changed = true;
        }
    }

    return changed ? save(errorMessage) : true;
}

QList<FeedbackRecord> FeedbackStore::allFeedbacks() const {
    return m_feedbacks;
}

QList<FeedbackRecord> FeedbackStore::feedbacksForCommit(const QString& commitHash) const {
    QList<FeedbackRecord> result;
    for (const FeedbackRecord& record : m_feedbacks) {
        if (record.commitHash == commitHash)
            result.append(record);
    }
    return result;
}

bool FeedbackStore::isEmpty() const {
    return m_feedbacks.isEmpty();
}

void FeedbackStore::importFeedbacks(const QList<FeedbackRecord>& records) {
    m_feedbacks = records;
}

QString FeedbackStore::feedbackFilePath() const {
    return QDir(m_projectPath).filePath(".hwpilot/feedbacks.json");
}
