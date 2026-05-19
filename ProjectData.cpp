#include "ProjectData.h"

#include <QJsonArray>

QJsonObject FeedbackRecord::toJson() const {
    QJsonObject object;
    object["id"] = id;
    object["commitHash"] = commitHash;
    object["mode"] = mode;
    object["createdAt"] = createdAt;
    object["content"] = content;
    return object;
}

FeedbackRecord FeedbackRecord::fromJson(const QJsonObject& object) {
    FeedbackRecord record;
    record.id = object["id"].toString();
    record.commitHash = object["commitHash"].toString();
    record.mode = object["mode"].toString();
    record.createdAt = object["createdAt"].toString();
    record.content = object["content"].toString();
    return record;
}

QJsonObject ProjectData::toJson() const {
    QJsonObject object;
    object["projectName"] = projectName;
    object["projectPath"] = projectPath;
    object["assignmentText"] = assignmentText;
    object["lastOpenedAt"] = lastOpenedAt;

    QJsonArray feedbackArray;
    for (const FeedbackRecord& record : feedbacks)
        feedbackArray.append(record.toJson());
    object["feedbacks"] = feedbackArray;

    return object;
}

ProjectData ProjectData::fromJson(const QJsonObject& object) {
    ProjectData data;
    data.projectName = object["projectName"].toString();
    data.projectPath = object["projectPath"].toString();
    data.assignmentText = object["assignmentText"].toString();
    data.lastOpenedAt = object["lastOpenedAt"].toString();

    const QJsonArray feedbackArray = object["feedbacks"].toArray();
    for (const QJsonValue& value : feedbackArray) {
        if (value.isObject())
            data.feedbacks.append(FeedbackRecord::fromJson(value.toObject()));
    }

    return data;
}

QList<FeedbackRecord> ProjectData::feedbacksForCommit(const QString& commitHash) const {
    QList<FeedbackRecord> result;
    for (const FeedbackRecord& record : feedbacks) {
        if (record.commitHash == commitHash)
            result.append(record);
    }
    return result;
}
