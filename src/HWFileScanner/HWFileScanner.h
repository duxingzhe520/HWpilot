#ifndef HWFILESCANNER_H
#define HWFILESCANNER_H

#include <QString>
#include <QStringList>
#include <QList>

// 代表一个被扫描到的代码文件
struct CodeFile {
    QString relativePath;   // 相对于作业根目录的路径，用于展示给用户和LLM
    QString absolutePath;   // 绝对路径，用于读取文件
    QString content;        // 文件的文本内容
    QString extension;      // 文件扩展名（小写，不含点）
};

class HWFileScanner
{
public:
    // 默认支持的代码文件扩展名白名单
    static const QStringList DEFAULT_CODE_EXTENSIONS;

    /**
     * @brief 扫描指定目录下的所有代码文件（递归）
     * @param dirPath       要扫描的目录的绝对路径
     * @param extensions    文件扩展名白名单（不含"."，如 "cpp", "h", "py"）
     * @param maxFileSizeKB 单个文件大小上限(KB)，超出则跳过，防止上下文溢出，默认 200KB
     * @return              找到的代码文件列表，按相对路径排序
     */
    static QList<CodeFile> scanDirectory(
        const QString& dirPath,
        const QStringList& extensions = DEFAULT_CODE_EXTENSIONS,
        int maxFileSizeKB = 200
    );

    /**
     * @brief 将扫描到的文件列表格式化为发给 LLM 的结构化字符串
     * @param files             scanDirectory() 返回的文件列表
     * @param taskDescription   可选：作业/任务的要求描述（学生输入）
     * @return                  格式化好的字符串，包含所有文件内容，可直接作为 user message
     */
    static QString formatFilesForLLM(
        const QList<CodeFile>& files,
        const QString& taskDescription = QString()
    );

    /**
     * @brief 打印扫描结果摘要到控制台（供用户确认）
     * @param files     要打印的文件列表
     * @param dirPath   扫描的根目录（用于展示）
     */
    static void printScanSummary(const QList<CodeFile>& files, const QString& dirPath);

private:
    static bool isTargetFile(const QString& suffix, const QStringList& extensions);
};

#endif // HWFILESCANNER_H
