#include "diagnostics.h"

#include <QDebug>

#include <atomic>

namespace {
std::atomic_bool g_diagnosticsEnabled{false};
}

namespace Diagnostics {

void setEnabled(bool enabled)
{
    g_diagnosticsEnabled.store(enabled, std::memory_order_relaxed);
}

bool isEnabled()
{
    return g_diagnosticsEnabled.load(std::memory_order_relaxed);
}

void logWarning(const QString &message)
{
    if (!isEnabled()) {
        return;
    }

    qWarning().noquote() << "[FramelessDiag]" << message;
}

} // namespace Diagnostics
