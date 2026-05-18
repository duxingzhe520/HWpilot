#include "GitService.h"

#include <QProcess>

void GitService::setWorkingDirectory(const QString& directory) {
    m_workingDirectory = directory;
}

QString GitService::workingDirectory() const {
    return m_workingDirectory;
}

bool GitService::isGitRepo() const {
    const GitCommandResult result = runGit({"rev-parse", "--is-inside-work-tree"});
    return result.success && result.stdoutText.trimmed() == "true";
}

GitCommandResult GitService::initRepo() const {
    return runGit({"init"});
}

QString GitService::statusPorcelain() const {
    return runGit({"status", "--short"}).stdoutText;
}

QString GitService::diff() const {
    return runGit({"diff", "--", "."}).stdoutText;
}

QString GitService::diffStat() const {
    return runGit({"diff", "--stat", "--", "."}).stdoutText;
}

QString GitService::currentHead() const {
    const GitCommandResult result = runGit({"rev-parse", "HEAD"});
    return result.success ? result.stdoutText.trimmed() : QString();
}

QList<GitCommit> GitService::log(int limit) const {
    QList<GitCommit> commits;
    const QString format = "%H%x1f%h%x1f%ad%x1f%s%x1e";
    const GitCommandResult result = runGit({"log", QString("--max-count=%1").arg(limit), "--date=short", QString("--pretty=format:%1").arg(format)});
    if (!result.success)
        return commits;

    const QStringList records = result.stdoutText.split(QChar(0x1e), Qt::SkipEmptyParts);
    for (const QString& record : records) {
        const QStringList fields = record.split(QChar(0x1f));
        if (fields.size() < 4)
            continue;

        GitCommit commit;
        commit.hash = fields.at(0).trimmed();
        commit.shortHash = fields.at(1).trimmed();
        commit.date = fields.at(2).trimmed();
        commit.subject = fields.at(3).trimmed();
        commits.append(commit);
    }

    return commits;
}

GitCommandResult GitService::addAll() const {
    return runGit({"add", "-A"});
}

GitCommandResult GitService::commit(const QString& message) const {
    return runGit({"commit", "-m", message});
}

GitCommandResult GitService::runGit(const QStringList& arguments) const {
    GitCommandResult result;
    if (m_workingDirectory.isEmpty()) {
        result.stderrText = "Git 工作目录为空。";
        return result;
    }

    QProcess process;
    process.setWorkingDirectory(m_workingDirectory);
    process.start("git", arguments);
    if (!process.waitForFinished(10000)) {
        process.kill();
        process.waitForFinished();
        result.stderrText = "Git 命令执行超时。";
        return result;
    }

    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    result.stderrText = QString::fromUtf8(process.readAllStandardError());
    result.success = process.exitStatus() == QProcess::NormalExit && result.exitCode == 0;
    return result;
}
