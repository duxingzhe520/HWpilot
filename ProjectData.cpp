#include "ProjectData.h"

#include <QJsonArray>

QJsonObject FeedbackItem::toJson() const {
    QJsonObject object;
    object["id"] = id;
    object["severity"] = severity;
    object["filePath"] = filePath;
    object["line"] = line;
    object["category"] = category;
    object["title"] = title;
    object["suggestion"] = suggestion;
    object["status"] = status;
    return object;
}

FeedbackItem FeedbackItem::fromJson(const QJsonObject& object) {
    FeedbackItem item;
    item.id = object["id"].toString();
    item.severity = object["severity"].toString();
    item.filePath = object["filePath"].toString();
    item.line = object["line"].toInt(-1);
    item.category = object["category"].toString();
    item.title = object["title"].toString();
    item.suggestion = object["suggestion"].toString();
    item.status = object["status"].toString("unresolved");
    return item;
}

QJsonObject FeedbackRecord::toJson() const {
    QJsonObject object;
    object["id"] = id;
    object["commitHash"] = commitHash;
    object["mode"] = mode;
    object["createdAt"] = createdAt;
    object["summary"] = summary;
    object["rawContent"] = rawContent;
    object["parseStatus"] = parseStatus;

    QJsonArray itemArray;
    for (const FeedbackItem& item : items)
        itemArray.append(item.toJson());
    object["items"] = itemArray;

    return object;
}

FeedbackRecord FeedbackRecord::fromJson(const QJsonObject& object) {
    FeedbackRecord record;
    record.id = object["id"].toString();
    record.commitHash = object["commitHash"].toString();
    record.mode = object["mode"].toString();
    record.createdAt = object["createdAt"].toString();
    record.summary = object["summary"].toString();
    record.rawContent = object["rawContent"].toString(object["content"].toString());
    record.parseStatus = object["parseStatus"].toString(record.rawContent.isEmpty() ? QString("parsed") : QString("raw"));

    const QJsonArray itemArray = object["items"].toArray();
    for (const QJsonValue& value : itemArray) {
        if (value.isObject())
            record.items.append(FeedbackItem::fromJson(value.toObject()));
    }

    if (record.summary.isEmpty())
        record.summary = record.rawContent.left(160);

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
