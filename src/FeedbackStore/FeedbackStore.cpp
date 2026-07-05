#include "FeedbackStore.h"

#include "../AppText.h"

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
            *errorMessage = AppText::get("error.projectDirMissingFeedback");
        return false;
    }

    if (!projectDir.exists(".hwpilot") && !projectDir.mkpath(".hwpilot")) {
        if (errorMessage)
            *errorMessage = AppText::get("error.feedbackDirCreate");
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
            *errorMessage = AppText::get("error.feedbackRead");
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage)
            *errorMessage = AppText::get("error.feedbackInvalid");
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
            *errorMessage = AppText::get("error.feedbackDirCreate");
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
            *errorMessage = AppText::get("error.feedbackWrite");
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
        *errorMessage = AppText::get("error.feedbackItemMissing");
    return false;
}

bool FeedbackStore::updateItemsStatus(const QStringList& itemIds, const QString& status, QString* errorMessage) {
    if (itemIds.isEmpty())
        return true;

    bool changed = false;
    for (FeedbackRecord& record : m_feedbacks) {
        for (FeedbackItem& item : record.items) {
            if (itemIds.contains(item.id)) {
                item.status = status;
                changed = true;
            }
        }
    }

    if (!changed && errorMessage)
        *errorMessage = AppText::get("error.feedbackItemMissing");
    return changed ? save(errorMessage) : false;
}

bool FeedbackStore::renameFeedbackRecord(const QString& recordId, const QString& summary, QString* errorMessage) {
    for (FeedbackRecord& record : m_feedbacks) {
        if (record.id == recordId) {
            record.summary = summary;
            return save(errorMessage);
        }
    }

    if (errorMessage)
        *errorMessage = AppText::get("error.feedbackRecordMissing");
    return false;
}

bool FeedbackStore::markItemsReviewed(const QStringList& itemIds, const QString& reviewRecordId, QString* errorMessage) {
    if (itemIds.isEmpty() || reviewRecordId.isEmpty())
        return true;

    bool changed = false;
    for (FeedbackRecord& record : m_feedbacks) {
        for (FeedbackItem& item : record.items) {
            if (itemIds.contains(item.id)) {
                item.reviewRecordId = reviewRecordId;
                changed = true;
            }
        }
    }

    return changed ? save(errorMessage) : true;
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
