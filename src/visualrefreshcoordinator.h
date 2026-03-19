#pragma once

#include <QtGlobal>

#include <functional>

class QObject;

class VisualRefreshCoordinator
{
public:
    explicit VisualRefreshCoordinator(QObject *context);

    void configure(std::function<quint64()> tokenProvider,
                   std::function<bool()> visibilityProvider,
                   std::function<void()> refreshPass,
                   std::function<void(quint64, quint64, quint64)> tokenTransitionHandler,
                   std::function<void()> presentHandler);

    void requestRefresh();

private:
    void flush();

    QObject *m_context;
    std::function<quint64()> m_tokenProvider;
    std::function<bool()> m_visibilityProvider;
    std::function<void()> m_refreshPass;
    std::function<void(quint64, quint64, quint64)> m_tokenTransitionHandler;
    std::function<void()> m_presentHandler;
    bool m_pendingRefresh;
    bool m_refreshDirty;
    quint64 m_lastVisualStateToken;
};
