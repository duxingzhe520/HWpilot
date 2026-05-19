#ifndef GITSERVICE_H
#define GITSERVICE_H

#include <QList>
#include <QString>
#include <QStringList>

struct GitCommandResult {
    bool success = false;
    int exitCode = -1;
    QString stdoutText;
    QString stderrText;
};

struct GitCommit {
    QString hash;
    QString shortHash;
    QString date;
    QString subject;
};

class GitService {
public:
    void setWorkingDirectory(const QString& directory);
    QString workingDirectory() const;

    bool isGitRepo() const;
    GitCommandResult initRepo() const;
    QString statusPorcelain() const;
    QString diff() const;
    QString diffStat() const;
    QString currentHead() const;
    QList<GitCommit> log(int limit = 30) const;
    GitCommandResult addAll() const;
    GitCommandResult commit(const QString& message) const;

private:
    GitCommandResult runGit(const QStringList& arguments) const;

    QString m_workingDirectory;
};

#endif  // GITSERVICE_H
