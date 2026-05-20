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
    return runGit({"diff", "--", ".", ":(exclude).hwpilot/**"}).stdoutText;
}

QString GitService::diffStat() const {
    return runGit({"diff", "--stat", "--", ".", ":(exclude).hwpilot/**"}).stdoutText;
}

QString GitService::diffForCommit(const QString& commitHash) const {
    if (commitHash.isEmpty())
        return diff();

    return runGit({"show", "--format=", "--patch", commitHash, "--", ".", ":(exclude).hwpilot/**"}).stdoutText;
}

QString GitService::diffStatForCommit(const QString& commitHash) const {
    if (commitHash.isEmpty())
        return diffStat();

    return runGit({"show", "--format=", "--stat", commitHash, "--", ".", ":(exclude).hwpilot/**"}).stdoutText;
}

QStringList GitService::filesAtCommit(const QString& commitHash) const {
    if (commitHash.isEmpty())
        return {};

    const GitCommandResult result = runGit({"-c", "core.quotepath=false", "ls-tree", "-r", "--name-only", commitHash});
    if (!result.success)
        return {};

    QStringList paths = result.stdoutText.split('\n', Qt::SkipEmptyParts);
    for (QString& path : paths)
        path = path.trimmed();
    paths.removeAll(QString());
    return paths;
}

QString GitService::fileContentAtCommit(const QString& commitHash, const QString& filePath) const {
    if (commitHash.isEmpty() || filePath.isEmpty())
        return {};

    const QString objectName = QString("%1:%2").arg(commitHash, filePath);
    GitCommandResult result = runGit({"show", "--no-ext-diff", "--no-textconv", objectName});
    if (result.success)
        return result.stdoutText;

    result = runGit({"cat-file", "-p", objectName});
    return result.success ? result.stdoutText : QString();
}

QString GitService::currentHead() const {
    const GitCommandResult result = runGit({"rev-parse", "HEAD"});
    return result.success ? result.stdoutText.trimmed() : QString();
}

QList<GitCommit> GitService::log(int limit) const {
    QList<GitCommit> commits;
    const QString format = "%H%x1f%h%x1f%ad%x1f%s%x1e";
    const GitCommandResult result = runGit({"log", "--branches", "--remotes", QString("--max-count=%1").arg(limit), "--date=format:%Y-%m-%d %H:%M", QString("--pretty=format:%1").arg(format)});
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

QStringList GitService::branchesContainingCommit(const QString& commitHash) const {
    if (commitHash.isEmpty())
        return {};

    const GitCommandResult result = runGit({"branch", "-a", "--contains", commitHash, "--format=%(refname:short)"});
    if (!result.success)
        return {};

    QStringList branches = result.stdoutText.split('\n', Qt::SkipEmptyParts);
    for (QString& branch : branches)
        branch = branch.trimmed();
    branches.removeAll(QString());
    branches.removeAll("origin/HEAD");
    branches.removeAll("remotes/origin/HEAD");
    branches.removeDuplicates();
    branches.sort(Qt::CaseInsensitive);
    return branches;
}

QStringList GitService::changedPaths() const {
    const GitCommandResult result = runGit({"status", "--porcelain=v1", "-z", "--untracked-files=all"});
    if (!result.success)
        return {};

    QStringList paths;
    const QStringList records = result.stdoutText.split(QChar('\0'), Qt::SkipEmptyParts);
    for (int i = 0; i < records.size(); ++i) {
        const QString& record = records.at(i);
        if (record.size() < 4)
            continue;

        const QString status = record.left(2);
        const QString path = record.mid(3);
        if (!path.isEmpty())
            paths.append(path);

        if (status.contains('R') || status.contains('C'))
            ++i;
    }

    paths.removeDuplicates();
    return paths;
}

GitCommandResult GitService::addPaths(const QStringList& paths) const {
    GitCommandResult result;
    if (paths.isEmpty()) {
        result.success = true;
        result.exitCode = 0;
        return result;
    }

    QStringList arguments = {"add", "--"};
    arguments.append(paths);
    return runGit(arguments);
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
    if (!process.waitForFinished(30000)) {
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
