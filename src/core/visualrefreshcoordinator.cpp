#include "visualrefreshcoordinator.h"

#include <QObject>
#include <QTimer>

VisualRefreshCoordinator::VisualRefreshCoordinator(QObject *context)
    : m_context(context)
    , m_tokenProvider()
    , m_visibilityProvider()
    , m_refreshPass()
    , m_tokenTransitionHandler()
    , m_presentHandler()
    , m_pendingRefresh(false)
    , m_refreshDirty(false)
    , m_lastVisualStateToken(0)
{
}

void VisualRefreshCoordinator::configure(std::function<quint64()> tokenProvider,
                                         std::function<bool()> visibilityProvider,
                                         std::function<void()> refreshPass,
                                         std::function<void(quint64, quint64, quint64)> tokenTransitionHandler,
                                         std::function<void()> presentHandler)
{
    m_tokenProvider = std::move(tokenProvider);
    m_visibilityProvider = std::move(visibilityProvider);
    m_refreshPass = std::move(refreshPass);
    m_tokenTransitionHandler = std::move(tokenTransitionHandler);
    m_presentHandler = std::move(presentHandler);
}

void VisualRefreshCoordinator::requestRefresh()
{
    m_refreshDirty = true;
    if (m_pendingRefresh || m_context == nullptr) {
        return;
    }

    m_pendingRefresh = true;
    QTimer::singleShot(0, m_context, [this]() {
        flush();
    });
}

void VisualRefreshCoordinator::flush()
{
    m_pendingRefresh = false;
    if (!m_visibilityProvider || !m_visibilityProvider()) {
        m_refreshDirty = false;
        return;
    }

    if (!m_tokenProvider || !m_refreshPass || !m_presentHandler) {
        m_refreshDirty = false;
        return;
    }

    bool needsReschedule = false;
    for (int pass = 0; pass < 3; ++pass) {
        const quint64 tokenBefore = m_tokenProvider();
        const quint64 previousToken = m_lastVisualStateToken;
        m_refreshDirty = false;

        m_refreshPass();

        const quint64 tokenAfter = m_tokenProvider();
        if (m_tokenTransitionHandler) {
            m_tokenTransitionHandler(previousToken, tokenBefore, tokenAfter);
        }

        m_lastVisualStateToken = tokenAfter;
        m_presentHandler();

        if (!m_refreshDirty && tokenAfter == tokenBefore) {
            return;
        }

        needsReschedule = true;
    }

    if (needsReschedule) {
        requestRefresh();
    }
}
