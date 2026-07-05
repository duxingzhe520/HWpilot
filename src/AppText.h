#ifndef APPTEXT_H
#define APPTEXT_H

#include <QString>

namespace AppText {

QString language();
void setLanguage(const QString& language);
QString get(const char* key);

}  // namespace AppText

#endif  // APPTEXT_H
