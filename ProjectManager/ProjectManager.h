#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>

#include "../ProjectData.h"

class ProjectManager {
public:
    bool openProject(const QString& projectPath, QString* errorMessage = nullptr);
    bool save(QString* errorMessage = nullptr) const;
    bool addFeedback(const FeedbackRecord& record, QString* errorMessage = nullptr);

    ProjectData& data();
    const ProjectData& data() const;
    QString metadataDirectory() const;
    QString projectFilePath() const;

private:
    bool load(QString* errorMessage);

    ProjectData m_data;
};

#endif  // PROJECTMANAGER_H
