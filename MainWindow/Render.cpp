#include "Render.h"

#include "../AppText.h"

#include <QFileInfo>
#include <QList>
#include <QTextDocument>

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

QString markdownToHtmlBlock(const QString& markdown) {
    QTextDocument document;
    document.setMarkdown(markdown);
    QString html = document.toHtml();
    const int bodyStart = html.indexOf("<body");
    if (bodyStart >= 0) {
        const int bodyContentStart = html.indexOf('>', bodyStart);
        const int bodyEnd = html.lastIndexOf("</body>");
        if (bodyContentStart >= 0 && bodyEnd > bodyContentStart)
            html = html.mid(bodyContentStart + 1, bodyEnd - bodyContentStart - 1);
    }
    return html.trimmed();
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
    return AppText::get("prompt.structuredFeedback");
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
        return AppText::get("diff.rootFolder");
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

    QString html = QString("<div style=\"margin-top:8px;\"><div style=\"font-weight:600; color:%1; margin-bottom:4px;\">%2</div>")
                       .arg(color, htmlEscape(AppText::get("diff.lines").arg(title).arg(lines.size())));
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
        html += renderReadableLineGroup(AppText::get("diff.addedContent"), block.addedLines, true);
    } else if (block.changeType == "deleted") {
        html += renderReadableLineGroup(AppText::get("diff.deletedContent"), block.removedLines, false);
    } else {
        html += renderReadableLineGroup(AppText::get("diff.addedContent"), block.addedLines, true);
        html += renderReadableLineGroup(AppText::get("diff.deletedContent"), block.removedLines, false);
    }

    if (additions == 0 && deletions == 0)
        html += QString("<p style=\"margin:0; color:#64748b;\">%1</p>").arg(htmlEscape(AppText::get("diff.binaryChange")));

    html += "</div></div>";
    return html;
}

QString renderReadableDiff(const QString& diff) {
    if (diff.trimmed().isEmpty())
        return QString("<p>%1</p>").arg(htmlEscape(AppText::get("diff.noContent")));

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
                current.path = AppText::get("diff.unknownFile");
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

        QString html = QString("<h4 style=\"margin:18px 0 8px 0; color:#0f172a;\">%1</h4>")
                           .arg(htmlEscape(AppText::get("diff.filesTitle").arg(title).arg(categoryBlocks.size())));
        for (const DiffFileBlock& block : categoryBlocks)
            html += renderReadableFileBlock(block);
        return html;
    };

    QString html;
    html += renderCategory(AppText::get("diff.addedFiles"), addedBlocks);
    html += renderCategory(AppText::get("diff.deletedFiles"), deletedBlocks);
    html += renderCategory(AppText::get("diff.modifiedFiles"), modifiedBlocks);

    if (html.isEmpty()) {
        html += QString("<p style=\"color:#64748b;\">%1</p>").arg(htmlEscape(AppText::get("diff.unparsed")));
    }

    return html;
}

QString renderDiffStat(const QString& diffStat) {
    if (diffStat.trimmed().isEmpty())
        return QString("<p style=\"margin:0; color:#64748b;\">%1</p>").arg(htmlEscape(AppText::get("diff.noStat")));

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
    html += QString("<span style=\"background:#f1f5f9; border:1px solid #d9dee7; border-radius: 0px; padding:5px 11px; color:#334155;\">%1</span>").arg(htmlEscape(AppText::get("diff.fileCount").arg(entries.size())));
    html += "&nbsp;&nbsp;";
    html += QString("<span style=\"background:#ecfdf3; border:1px solid #bbf7d0; border-radius: 0px; padding:5px 11px; color:#166534;\">+%1</span>").arg(totalAdditions);
    html += "&nbsp;&nbsp;";
    html += QString("<span style=\"background:#fef2f2; border:1px solid #fecaca; border-radius: 0px; padding:5px 11px; color:#991b1b;\">-%1</span>").arg(totalDeletions);
    html += "&nbsp;&nbsp;";
    if (!summary.isEmpty())
        html += QString("<span style=\"color:#64748b; padding:5px 0;\">%1</span>").arg(htmlEscape(summary));
    html += "</div>";

    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; border:1px solid #e2e8f0; border-radius: 0px; overflow:hidden;\">";
    html += QString("<tr style=\"background:#f8fafc;\"><th align=\"left\" style=\"color:#475569; font-weight:600;\">%1</th><th align=\"right\" style=\"color:#166534; font-weight:600; width:70px;\">%2</th><th align=\"right\" style=\"color:#991b1b; font-weight:600; width:70px;\">%3</th><th align=\"left\" style=\"color:#64748b; font-weight:600; width:160px;\">%4</th></tr>")
                .arg(htmlEscape(AppText::get("diff.file")), htmlEscape(AppText::get("diff.added")), htmlEscape(AppText::get("diff.deleted")), htmlEscape(AppText::get("diff.change")));
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

QString feedbackStatusPill(const QString& status) {
    if (status == "resolved" || status == "已解决")
        return pillHtml(AppText::get("feedback.resolved"), "#166534", "#ecfdf3", "#bbf7d0");
    if (status == "uncertain" || status == "无法确定")
        return pillHtml(AppText::get("feedback.uncertain"), "#92400e", "#fffbeb", "#fde68a");
    if (status == "ignored" || status == "ignore" || status == "忽略")
        return pillHtml(AppText::get("feedback.ignored"), "#64748b", "#f8fafc", "#d9dee7");
    return pillHtml(AppText::get("feedback.unresolved"), "#1d4ed8", "#eff6ff", "#bfdbfe");
}

QString renderFeedbackRecordDetail(const FeedbackRecord& feedback) {
    QString html;
    QString displayCreatedAt = feedback.createdAt;
    displayCreatedAt.replace('T', ' ');
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(feedback.summary.isEmpty() ? AppText::get("feedback.record") : feedback.summary));
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("version.commitDate")), htmlEscape(displayCreatedAt));
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2&nbsp;&nbsp;%3&nbsp;&nbsp;%4</td></tr>")
                .arg(htmlEscape(AppText::get("version.type")),
                     pillHtml(feedback.mode.isEmpty() ? AppText::get("label.fileAnalysis") : feedback.mode, "#1d4ed8", "#eff6ff", "#bfdbfe"),
                     pillHtml(feedback.parseStatus.isEmpty() ? "unknown" : feedback.parseStatus, "#475569", "#f8fafc", "#d9dee7"),
                     pillHtml(AppText::get("feedback.issueCount").arg(feedback.items.size()), "#334155", "#f1f5f9", "#d9dee7"));
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("label.summary")), htmlEscape(feedback.summary.isEmpty() ? "-" : feedback.summary));
    html += "</table>";
    html += QString("<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">%1</h3>").arg(htmlEscape(AppText::get("label.fullFeedback")));
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(markdownToHtmlBlock(feedback.rawContent.isEmpty() ? "-" : feedback.rawContent));
    return html;
}

QString renderFeedbackItemDetail(const FeedbackItem& item) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 6px 0; color:#111827;\">%1</h2>").arg(htmlEscape(item.title.isEmpty() ? AppText::get("feedback.unnamedIssue") : item.title));
    const QString location = item.filePath.isEmpty()
                                 ? AppText::get("feedback.noLocation")
                                 : QString("%1%2").arg(item.filePath, item.line >= 0 ? QString(":%1").arg(item.line) : QString());
    html += "<table cellspacing=\"0\" cellpadding=\"6\" style=\"border-collapse:collapse; width:100%; margin-bottom:10px;\">";
    html += QString("<tr><td style=\"width:82px; color:#64748b;\">%1</td><td>%2&nbsp;&nbsp;%3&nbsp;&nbsp;%4</td></tr>").arg(htmlEscape(AppText::get("label.attributes")))
                .arg(severityPill(item.severity),
                     pillHtml(item.category.isEmpty() ? "other" : item.category, "#475569", "#f8fafc", "#d9dee7"),
                     feedbackStatusPill(item.status));
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>").arg(htmlEscape(AppText::get("label.location")), htmlEscape(location));
    html += QString("<tr><td style=\"color:#64748b;\">%1</td><td>%2</td></tr>")
                .arg(htmlEscape(AppText::get("feedback.reviewState")),
                     htmlEscape(item.reviewRecordId.isEmpty() ? AppText::get("feedback.notReviewed") : AppText::get("feedback.reviewed")));
    html += "</table>";
    html += QString("<h3 style=\"margin:10px 0 6px 0; color:#0f172a;\">%1</h3>").arg(htmlEscape(AppText::get("label.fullSuggestion")));
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:10px; line-height:1.55; color:#1f2937;\">%1</div>")
                .arg(markdownToHtmlBlock(item.suggestion.isEmpty() ? AppText::get("feedback.noSuggestion") : item.suggestion));
    return html;
}

QString renderHeuristicItemDetail(const FeedbackItem& item) {
    QString html;
    html += QString("<h2 style=\"margin:0 0 8px 0; color:#111827;\">%1</h2>").arg(htmlEscape(item.title.isEmpty() ? AppText::get("feedback.unnamedIssue") : item.title));
    html += QString("<div style=\"border:1px solid #d9dee7; background:#ffffff; padding:12px; line-height:1.65; color:#1f2937;\">%1</div>")
                .arg(markdownToHtmlBlock(item.suggestion.isEmpty() ? AppText::get("feedback.noSuggestion") : item.suggestion));
    return html;
}

}  // namespace MainWindowRender
