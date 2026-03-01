#pragma once

#include <QString>

namespace Diagnostics {

void setEnabled(bool enabled);
bool isEnabled();
void logWarning(const QString &message);

} // namespace Diagnostics
