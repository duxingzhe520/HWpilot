#ifndef FEEDBACKSTORE_H
#define FEEDBACKSTORE_H

#include <QList>
#include <QString>
#include <QStringList>

#include "../ProjectData.h"

class FeedbackStore {
public:
    bool openProject(const QString& projectPath, QString* errorMessage = nullptr);
    bool save(QString* errorMessage = nullptr) const;
    bool addFeedback(const FeedbackRecord& record, QString* errorMessage = nullptr);
    bool updateItemStatus(const QString& itemId, const QString& status, QString* errorMessage = nullptr);
    bool updateItemsStatus(const QStringList& itemIds, const QString& status, QString* errorMessage = nullptr);
    bool renameFeedbackRecord(const QString& recordId, const QString& summary, QString* errorMessage = nullptr);
    bool markItemsReviewed(const QStringList& itemIds, const QString& reviewRecordId, QString* errorMessage = nullptr);
    bool reassignCommit(const QString& fromCommitHash, const QString& toCommitHash, QString* errorMessage = nullptr);

    QList<FeedbackRecord> allFeedbacks() const;
    QList<FeedbackRecord> feedbacksForCommit(const QString& commitHash) const;
    bool isEmpty() const;
    void importFeedbacks(const QList<FeedbackRecord>& records);
    QString feedbackFilePath() const;

private:
    bool load(QString* errorMessage);

    QString m_projectPath;
    QList<FeedbackRecord> m_feedbacks;
};

#endif  // FEEDBACKSTORE_H
