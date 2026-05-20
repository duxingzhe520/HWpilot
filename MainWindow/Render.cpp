#include "Render.h"

#include <QFileInfo>
#include <QList>

#include "../HWFileScanner/HWFileScanner.h"

namespace MainWindowRender {
constexpr int NoteRole = Qt::UserRole + 2;
constexpr int CommitHashRole = Qt::UserRole + 3;
constexpr int FilePathRole = Qt::UserRole + 4;
constexpr int IsDirectoryRole = Qt::UserRole + 5;
constexpr int DetailHtmlRole = Qt::UserRole + 6;

struct DiffFileBlock {
    QString path;
    QString changeType = "modified";
    QStringList addedLines;
    QStringList removedLines;
};

struct DiffStatEntry {
    QString path;
    int additions = 0;
    int deletions = 0;
    QString detail;
};

QString htmlEscape(const QString& text) {
    QString escaped = text.toHtmlEscaped();
    return escaped.replace('\n', "<br>");
}

QString extractJsonObjectText(const QString& text) {
    QString trimmed = text.trimmed();
    if (trimmed.startsWith("```")) {
        const int firstNewline = trimmed.indexOf('\n');
        const int lastFence = trimmed.lastIndexOf("```");
        if (firstNewline >= 0 && lastFence > firstNewline)
            trimmed = trimmed.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
    }

    const int firstBrace = trimmed.indexOf('{');
    const int lastBrace = trimmed.lastIndexOf('}');
    if (firstBrace >= 0 && lastBrace > firstBrace)
        return trimmed.mid(firstBrace, lastBrace - firstBrace + 1);

    return trimmed;
}

QString structuredFeedbackInstruction() {
    return
        "请严格输出一个 JSON 对象，不要在 JSON 外添加解释文字。JSON 结构如下：\n"
        "{\n"
        "  \"summary\": \"一句话总结本次反馈\",\n"
        "  \"items\": [\n"
        "    {\n"
        "      \"severity\": \"high|medium|low\",\n"
        "      \"filePath\": \"相关文件路径，无法判断则留空\",\n"
        "      \"line\": 具体行号，无法判断则为 -1,\n"
        "      \"category\": \"bug|boundary|memory|style|design|test|learning|other\",\n"
        "      \"title\": \"问题标题\",\n"
        "      \"suggestion\": \"具体修改或学习建议\"\n"
        "    }\n"
        "  ],\n"
        "  \"rawReport\": \"完整自然语言反馈报告\"\n"
        "}\n"
        "如果没有发现具体问题，items 输出空数组，并在 summary 与 rawReport 中说明。";
}

bool isSupportedFilePath(const QString& path) {
    const QFileInfo info(path);
    if (info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0)
        return true;

    return HWFileScanner::DEFAULT_CODE_EXTENSIONS.contains(info.suffix().toLower());
}

QString extensionForPath(const QString& path) {
    const QFileInfo info(path);
    if (info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0)
        return "cmake";

    return info.suffix().toLower();
}

QString folderForPath(const QString& path) {
    const int slash = path.lastIndexOf('/');
    if (slash <= 0)
        return "根目录";
    return path.left(slash);
}

QString pathFromDiffHeader(const QString& line) {
    const QString prefix = "diff --git ";
    if (!line.startsWith(prefix))
        return QString();

    const QStringList parts = line.mid(prefix.size()).split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return QString();

    QString path = parts.at(1);
    if (path.startsWith("b/"))
        path = path.mid(2);
    return path;
}

QString diffLineHtml(const QString& line) {
    QString color = "#475569";
    QString background = "#ffffff";
    QString border = "#e5e7eb";
    QString prefix = "&nbsp;";

    if (line.startsWith("@@")) {
        color = "#1d4ed8";
        background = "#eff6ff";
        border = "#bfdbfe";
        prefix = "@";
    } else if (line.startsWith('+') && !line.startsWith("+++")) {
        color = "#166534";
        background = "#ecfdf3";
        border = "#bbf7d0";
        prefix = "+";
    } else if (line.startsWith('-') && !line.startsWith("---")) {
        color = "#991b1b";
        background = "#fef2f2";
        border = "#fecaca";
        prefix = "-";
    } else if (line.startsWith("diff --git") || line.startsWith("index ") || line.startsWith("---") || line.startsWith("+++")) {
        color = "#64748b";
        background = "#f8fafc";
        border = "#e2e8f0";
        prefix = " ";
    }

    QString text = line.toHtmlEscaped();
    text.replace(" ", "&nbsp;");
    return QString("<div style=\"font-family: Menlo, Consolas, monospace; font-size: 12px; line-height: 1.45; color: %1; background: %2; border-left: 3px solid %3; padding: 1px 8px;\"><span style=\"display:inline-block; width:18px; color:%1;\">%4</span>%5</div>")
        .arg(color, background, border, prefix, text);
}

QString readableCodeLineHtml(const QString& line, const QString& color, const QString& background, const QString& border, const QString& prefix) {
    QString text = line.toHtmlEscaped();
    text.replace(" ", "&nbsp;");
    if (text.isEmpty())
        text = "&nbsp;";

    return QString("<div style=\"font-family: Menlo, Consolas, monospace; font-size:12px; line-height:1.45; color:%1; background:%2; border-left:3px solid %3; padding:2px 8px;\"><span style=\"display:inline-block; width:18px; color:%1;\">%4</span>%5</div>")
        .arg(color, background, border, prefix, text);
}

QString renderReadableLineGroup(const QString& title, const QStringList& lines, bool added) {
    if (lines.isEmpty())
        return QString();

    const QString color = added ? "#166534" : "#991b1b";
    const QString background = added ? "#ecfdf3" : "#fef2f2";
    const QString border = added ? "#bbf7d0" : "#fecaca";
    const QString prefix = added ? "+" : "-";

    QString html = QString("<div style=\"margin-top:8px;\"><div style=\"font-weight:600; color:%1; margin-bottom:4px;\">%2（%3 行）</div>")
                       .arg(color, htmlEscape(title))
                       .arg(lines.size());
    for (const QString& line : lines)
        html += readableCodeLineHtml(line, color, background, border, prefix);
    html += "</div>";
    return html;
}

QString renderReadableFileBlock(const DiffFileBlock& block) {
    const int additions = block.addedLines.size();
    const int deletions = block.removedLines.size();
    QString html;
    html += QString("<div style=\"border:1px solid #d9dee7; border-radius:0px; overflow:hidden; margin-bottom:10px; background:#ffffff;\">"
                    "<div style=\"background:#f1f5f9; padding:8px 10px; font-weight:600; color:#1f2937;\">%1"
                    "<span style=\"float:right; color:#64748b; font-weight:400;\">+%2 / -%3</span></div>"
                    "<div style=\"padding:8px 10px;\">")
                .arg(htmlEscape(block.path))
                .arg(additions)
                .arg(deletions);

    if (block.changeType == "added") {
        html += renderReadableLineGroup("新增内容", block.addedLines, true);
    } else if (block.changeType == "deleted") {
        html += renderReadableLineGroup("删除内容", block.removedLines, false);
    } else {
        html += renderReadableLineGroup("新增内容", block.addedLines, true);
        html += renderReadableLineGroup("删除内容", block.removedLines, false);
    }

    if (additions == 0 && deletions == 0)
        html += "<p style=\"margin:0; color:#64748b;\">这个文件有元数据或二进制变化，没有可展示的文本行。</p>";

    html += "</div></div>";
    return html;
}

QString renderReadableDiff(const QString& diff) {
    if (diff.trimmed().isEmpty())
        return "<p>没有可显示的内容变更。</p>";

    QList<DiffFileBlock> blocks;
    DiffFileBlock current;
    const QStringList lines = diff.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("diff --git ")) {
            if (!current.path.isEmpty())
                blocks.append(current);

            current = DiffFileBlock();
            current.path = pathFromDiffHeader(line);
            if (current.path.isEmpty())
                current.path = "未知文件";
            continue;
        }

        if (current.path.isEmpty())
            continue;

        if (line.startsWith("new file mode ")) {
            current.changeType = "added";
            continue;
        }
        if (line.startsWith("deleted file mode ")) {
            current.changeType = "deleted";
            continue;
        }
        if (line.startsWith("rename to ")) {
            current.path = line.mid(QString("rename to ").size()).trimmed();
            current.changeType = "modified";
            continue;
        }
        if (line.startsWith('+') && !line.startsWith("+++")) {
            current.addedLines.append(line.mid(1));
            continue;
        }
        if (line.startsWith('-') && !line.startsWith("---")) {
            current.removedLines.append(line.mid(1));
            continue;
        }
    }

    if (!current.path.isEmpty())
        blocks.append(current);

    QList<DiffFileBlock> addedBlocks;
    QList<DiffFileBlock> deletedBlocks;
    QList<DiffFileBlock> modifiedBlocks;
    for (const DiffFileBlock& block : blocks) {
        if (block.changeType == "added")
            addedBlocks.append(block);
        else if (block.changeType == "deleted")
            deletedBlocks.append(block);
        else
            modifiedBlocks.append(block);
    }

    auto renderCategory = [](const QString& title, const QList<DiffFileBlock>& categoryBlocks) {
        if (categoryBlocks.isEmpty())
            return QString();

        QString html = QString("<h4 style=\"margin:18px 0 8px 0; color:#0f172a;\">%1（%2 个文件）</h4>")
                           .arg(htmlEscape(title))
                           .arg(categoryBlocks.size());
        for (const DiffFileBlock& block : categoryBlocks)
            html += renderReadableFileBlock(block);
        return html;
    };

    QString html;
    html += renderCategory("新增文件", addedBlocks);
    html += renderCategory("删除文件", deletedBlocks);
    html += renderCategory("修改文件", modifiedBlocks);

    if (html.isEmpty()) {
        html += "<p style=\"color:#64748b;\">检测到了 diff，但没有解析出可展示的文本变更。</p>";
    }

    return html;
}

QString renderDiffStat(const QString& diffStat) {
    if (diffStat.trimmed().isEmpty())
        return "<p style=\"margin:0; color:#64748b;\">暂无文件变更统计。</p>";

    QList<DiffStatEntry> entries;
    QString summary;
    const QStringList lines = diffStat.split('\n', Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        const int pipeIndex = line.indexOf('|');
        if (pipeIndex < 0) {
            summary = line;
            continue;
        }

        DiffStatEntry entry;
        entry.path = line.left(pipeIndex).trimmed();
        entry.detail = line.mid(pipeIndex + 1).trimmed();
        for (const QChar ch : entry.detail) {
            if (ch == '+')
                ++entry.additions;
            else if (ch == '-')
                ++entry.deletions;
        }
        entries.append(entry);
    }

    if (entries.isEmpty())
        return QString("<p style=\"margin:0; color:#64748b;\">%1</p>").arg(htmlEscape(summary.isEmpty() ? diffStat.trimmed() : summary));

    int totalAdditions = 0;
    int totalDeletions = 0;
    for (const DiffStatEntry& entry : entries) {
        totalAdditions += entry.additions;
        totalDeletions += entry.deletions;
    }

    QString html;
    html += "<div style=\"display:flex; gap:8px; margin:2px 0 10px 0; flex-wrap:wrap;\">";
    html += QString("<span style=\"background:#f1f5f9; border:1px solid #d9dee7; border-radius: 0px; padding:5px 11px; color:#334155;\">%1 个文件</span>").arg(entries.size());
    html += "&nbsp;&nbsp;";
    html += QString("<span style=\"background:#ecfdf3; border:1px solid #bbf7d0; border-radius: 0px; padding:5px 11px; color:#166534;\">+%1</span>").arg(totalAdditions);
    html += "&nbsp;&nbsp;";
    html += QString("<span style=\"background:#fef2f2; border:1px solid #fecaca; border-radius: 0px; padding:5px 11px; color:#991b1b;\">-%1</span>").arg(totalDeletions);
    html += "&nbsp;&nbsp;";
    if (!summary.isEmpty())
        html += QString("<span style=\"color:#64748b; padding:5px 0;\">%1</span>").arg(htmlEscape(summary));
    html += "</div>";

    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; border:1px solid #e2e8f0; border-radius: 0px; overflow:hidden;\">";
    html += "<tr style=\"background:#f8fafc;\"><th align=\"left\" style=\"color:#475569; font-weight:600;\">文件</th><th align=\"right\" style=\"color:#166534; font-weight:600; width:70px;\">新增</th><th align=\"right\" style=\"color:#991b1b; font-weight:600; width:70px;\">删除</th><th align=\"left\" style=\"color:#64748b; font-weight:600; width:160px;\">变化</th></tr>";
    for (const DiffStatEntry& entry : entries) {
        const int totalChanges = entry.additions + entry.deletions;
        html += QString("<tr style=\"border-top:1px solid #e2e8f0;\"><td>%1</td><td align=\"right\" style=\"color:#166534;\">+%2</td><td align=\"right\" style=\"color:#991b1b;\">-%3</td><td style=\"font-family:Menlo, Consolas, monospace; color:#64748b;\">%4</td></tr>")
                    .arg(htmlEscape(entry.path))
                    .arg(entry.additions)
                    .arg(entry.deletions)
                    .arg(totalChanges);
    }
    html += "</table>";
    return html;
}

QString pillHtml(const QString& text, const QString& color, const QString& background, const QString& border) {
    return QString("<span style=\"border:1px solid %1; border-radius: 0px; padding:2px 7px; color:%2; background:%3; font-size:12px;\">%4</span>")
        .arg(border, color, background, htmlEscape(text));
}

QString severityPill(const QString& severity) {
    if (severity == "high")
        return pillHtml("high", "#991b1b", "#fef2f2", "#fecaca");
    if (severity == "low")
        return pillHtml("low", "#166534", "#ecfdf3", "#bbf7d0");
    return pillHtml(severity.isEmpty() ? "medium" : severity, "#92400e", "#fffbeb", "#fde68a");
}

QString renderFeedbackRecordDetail(const FeedbackRecord& feedback) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(feedback.summary.isEmpty() ? "反馈记录" : feedback.summary));
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">时间</td><td>%1</td></tr>").arg(htmlEscape(feedback.createdAt));
    html += QString("<tr><td style=\"color:#64748b;\">类型</td><td>%1&nbsp;&nbsp;%2&nbsp;&nbsp;%3</td></tr>")
                .arg(pillHtml(feedback.mode.isEmpty() ? "文件分析" : feedback.mode, "#1d4ed8", "#eff6ff", "#bfdbfe"),
                     pillHtml(feedback.parseStatus.isEmpty() ? "unknown" : feedback.parseStatus, "#475569", "#f8fafc", "#d9dee7"),
                     pillHtml(QString("%1 个问题").arg(feedback.items.size()), "#334155", "#f1f5f9", "#d9dee7"));
    html += QString("<tr><td style=\"color:#64748b;\">摘要</td><td>%1</td></tr>").arg(htmlEscape(feedback.summary.isEmpty() ? "无摘要" : feedback.summary));
    html += "</table>";
    html += "<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">完整反馈</h3>";
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(htmlEscape(feedback.rawContent.isEmpty() ? "这次反馈没有保存正文。" : feedback.rawContent));
    return html;
}

QString renderFeedbackItemDetail(const FeedbackItem& item) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(item.title.isEmpty() ? "问题详情" : item.title));
    const QString location = item.filePath.isEmpty()
                                 ? "未定位到具体文件"
                                 : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">属性</td><td>%1&nbsp;&nbsp;%2&nbsp;&nbsp;%3</td></tr>")
                .arg(severityPill(item.severity),
                     pillHtml(item.category.isEmpty() ? "other" : item.category, "#475569", "#f8fafc", "#d9dee7"),
                     pillHtml(item.status.isEmpty() ? "open" : item.status, "#1d4ed8", "#eff6ff", "#bfdbfe"));
    html += QString("<tr><td style=\"color:#64748b;\">位置</td><td>%1</td></tr>").arg(htmlEscape(location));
    html += "</table>";
    html += "<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">完整建议</h3>";
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(htmlEscape(item.suggestion.isEmpty() ? "这条问题没有保存建议。" : item.suggestion));
    return html;
}

}  // namespace MainWindowRender
