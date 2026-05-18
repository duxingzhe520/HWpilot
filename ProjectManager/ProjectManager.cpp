#include "ProjectManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

bool ProjectManager::openProject(const QString& projectPath, QString* errorMessage) {
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        if (errorMessage)
            *errorMessage = "项目目录不存在。";
        return false;
    }

    m_data = ProjectData();
    m_data.projectPath = projectDir.absolutePath();
    m_data.projectName = QFileInfo(m_data.projectPath).fileName();
    m_data.lastOpenedAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QDir metadataDir(metadataDirectory());
    if (!metadataDir.exists() && !projectDir.mkpath(".hwpilot")) {
        if (errorMessage)
            *errorMessage = "无法创建 .hwpilot 元数据目录。";
        return false;
    }

    if (!load(errorMessage))
        return false;

    m_data.projectPath = projectDir.absolutePath();
    if (m_data.projectName.isEmpty())
        m_data.projectName = QFileInfo(m_data.projectPath).fileName();
    m_data.lastOpenedAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    return save(errorMessage);
}

bool ProjectManager::load(QString* errorMessage) {
    QFile file(projectFilePath());
    if (!file.exists())
        return true;

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = "无法读取项目元数据。";
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage)
            *errorMessage = "项目元数据不是有效 JSON。";
        return false;
    }

    const QString projectPath = m_data.projectPath;
    m_data = ProjectData::fromJson(document.object());
    if (m_data.projectPath.isEmpty())
        m_data.projectPath = projectPath;

    return true;
}

bool ProjectManager::save(QString* errorMessage) const {
    QDir projectDir(m_data.projectPath);
    if (!projectDir.exists()) {
        if (errorMessage)
            *errorMessage = "项目目录不存在，无法保存元数据。";
        return false;
    }

    if (!projectDir.exists(".hwpilot") && !projectDir.mkpath(".hwpilot")) {
        if (errorMessage)
            *errorMessage = "无法创建 .hwpilot 元数据目录。";
        return false;
    }

    QFile file(projectFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = "无法写入项目元数据。";
        return false;
    }

    const QJsonDocument document(m_data.toJson());
    file.write(document.toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectManager::addFeedback(const FeedbackRecord& record, QString* errorMessage) {
    m_data.feedbacks.append(record);
    return save(errorMessage);
}

ProjectData& ProjectManager::data() {
    return m_data;
}

const ProjectData& ProjectManager::data() const {
    return m_data;
}

QString ProjectManager::metadataDirectory() const {
    return QDir(m_data.projectPath).filePath(".hwpilot");
}

QString ProjectManager::projectFilePath() const {
    return QDir(metadataDirectory()).filePath("project.json");
}
