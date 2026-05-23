#ifndef PROJECTDATA_H
#define PROJECTDATA_H

#include <QJsonObject>
#include <QList>
#include <QString>

struct FeedbackItem {
    QString id;
    QString severity;
    QString filePath;
    int line = -1;
    QString category;
    QString title;
    QString suggestion;
    QString status = "unresolved";
    QString sourceFeedbackId;
    QString reviewRecordId;

    QJsonObject toJson() const;
    static FeedbackItem fromJson(const QJsonObject& object);
};

struct FeedbackRecord {
    QString id;
    QString commitHash;
    QString mode;
    QString createdAt;
    QString summary;
    QString rawContent;
    QString parseStatus;
    QList<FeedbackItem> items;

    QJsonObject toJson() const;
    static FeedbackRecord fromJson(const QJsonObject& object);
};

struct ProjectData {
    QString projectName;
    QString projectPath;
    QString assignmentText;
    QString lastOpenedAt;
    QList<FeedbackRecord> feedbacks;

    QJsonObject toJson() const;
    static ProjectData fromJson(const QJsonObject& object);
};

#endif  // PROJECTDATA_H
