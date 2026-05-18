#ifndef PROJECTDATA_H
#define PROJECTDATA_H

#include <QJsonObject>
#include <QList>
#include <QString>

struct FeedbackRecord {
    QString id;
    QString commitHash;
    QString mode;
    QString createdAt;
    QString content;

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
    QList<FeedbackRecord> feedbacksForCommit(const QString& commitHash) const;
};

#endif  // PROJECTDATA_H
