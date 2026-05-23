#ifndef MAINWINDOW_RENDER_H
#define MAINWINDOW_RENDER_H

#include <QString>

#include "../ProjectData.h"

namespace MainWindowRender {

QString htmlEscape(const QString& text);
QString extractJsonObjectText(const QString& text);
QString structuredFeedbackInstruction();
bool isSupportedFilePath(const QString& path);
QString extensionForPath(const QString& path);
QString renderReadableDiff(const QString& diff);
QString renderDiffStat(const QString& diffStat);
QString renderFeedbackRecordDetail(const FeedbackRecord& feedback);
QString renderFeedbackItemDetail(const FeedbackItem& item);
QString renderHeuristicItemDetail(const FeedbackItem& item);

}  // namespace MainWindowRender

#endif  // MAINWINDOW_RENDER_H
