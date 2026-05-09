#include "HWFileScanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <algorithm>

const QStringList HWFileScanner::DEFAULT_CODE_EXTENSIONS = {
    // C / C++
    "c", "cpp", "cc", "cxx", "h", "hpp", "hxx",
    // Python
    "py",
    // Java
    "java",
    // JavaScript / TypeScript
    "js", "ts", "jsx", "tsx",
    // Go
    "go",
    // Rust
    "rs",
    // C#
    "cs",
    // Shell
    "sh", "bash",
    // 配置 / 构建文件（可帮助理解项目结构）
    "cmake", "makefile", "mk",
    // 文档
    "md", "txt"};

QList<CodeFile> HWFileScanner::scanDirectory(const QString& dirPath, const QStringList& extensions, int maxFileSizeKB) {
    QList<CodeFile> result;
    QDir baseDir(dirPath);

    if (!baseDir.exists()) {
        qWarning() << "[HWFileScanner] 目录不存在：" << dirPath;
        return result;
    }

    // 递归遍历目录下所有文件
    QDirIterator it(dirPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);

        // 跳过隐藏文件 / 目录（如 .git）
        if (fileInfo.isHidden())
            continue;
        if (fileInfo.absolutePath().contains("/."))
            continue;

        // 检查扩展名
        QString suffix = fileInfo.suffix().toLower();

        // CMakeLists.txt 特殊处理（无扩展名但文件名固定）
        bool isCMake = (fileInfo.fileName().toLower() == "cmakelists.txt");

        if (!isCMake && !isTargetFile(suffix, extensions))
            continue;

        // 检查文件大小
        qint64 fileSizeKB = fileInfo.size() / 1024;
        if (fileSizeKB > maxFileSizeKB) {
            qDebug() << "[HWFileScanner] 跳过超大文件（" << fileSizeKB << "KB）：" << fileInfo.fileName();
            continue;
        }

        // 读取文件内容（UTF-8）
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "[HWFileScanner] 无法打开文件：" << filePath;
            continue;
        }

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        CodeFile codeFile;
        codeFile.absolutePath = filePath;
        codeFile.relativePath = baseDir.relativeFilePath(filePath);
        codeFile.extension = isCMake ? "cmake" : suffix;
        codeFile.content = in.readAll();
        file.close();

        result.append(codeFile);
    }

    // 按相对路径排序，保证结果稳定
    std::sort(result.begin(), result.end(), [](const CodeFile& a, const CodeFile& b) { return a.relativePath < b.relativePath; });

    return result;
}

QString HWFileScanner::formatFilesForLLM(const QList<CodeFile>& files, const QString& taskDescription) {
    QString result;

    // --- 作业要求（可选）---
    if (!taskDescription.trimmed().isEmpty()) {
        result += "【作业要求 / 任务描述】\n";
        result += taskDescription.trimmed();
        result += "\n\n";
    }

    // --- 文件列表 ---
    result += QString("【学生提交的代码，共 %1 个文件】\n\n").arg(files.size());

    for (const CodeFile& f : files) {
        // 用 Markdown 代码块包裹，并标注语言便于 LLM 理解语法
        result += QString("### 文件：`%1`\n").arg(f.relativePath);
        result += QString("```%1\n").arg(f.extension);
        result += f.content;
        // 确保代码块结束前有换行
        if (!f.content.endsWith('\n'))
            result += '\n';
        result += "```\n\n";
    }

    return result;
}

void HWFileScanner::printScanSummary(const QList<CodeFile>& files, const QString& dirPath) {
    qDebug() << "\n========================================";
    qDebug() << " 扫描目录：" << dirPath;
    qDebug() << QString(" 共找到 %1 个代码文件：").arg(files.size());
    for (int i = 0; i < files.size(); ++i) {
        const CodeFile& f = files[i];
        qint64 sizeKB = f.content.size() / 1024;
        qDebug() << QString("  [%1] %2  (%3 KB)").arg(i + 1).arg(f.relativePath).arg(sizeKB == 0 ? QString("<1") : QString::number(sizeKB));
    }
    qDebug() << "========================================\n";
}

bool HWFileScanner::isTargetFile(const QString& suffix, const QStringList& extensions) {
    return extensions.contains(suffix);
}
