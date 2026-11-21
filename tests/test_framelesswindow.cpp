#include <QtTest>

#include "framelesswindow.h"

class TestFramelessWindow : public QObject
{
    Q_OBJECT

private slots:
    // Callers ask for the intended state before the window is shown, so a
    // titlebar that has simply not been shown yet must not read as hidden.
    void titleBarIsVisibleByDefault()
    {
        FramelessWindow window;
        QVERIFY(window.isTitleBarVisible());
    }

    void titleBarVisibilityRoundTrip()
    {
        FramelessWindow window;

        window.setTitleBarVisible(false);
        QVERIFY(!window.isTitleBarVisible());

        window.setTitleBarVisible(true);
        QVERIFY(window.isTitleBarVisible());
    }

    void backdropModeDefaultsToAuto()
    {
        FramelessWindow window;
        QCOMPARE(window.backdropMode(), WindowEffect::BackdropMode::Auto);
    }

    void titleBarHeightIsAdjustable()
    {
        FramelessWindow window;
        window.setTitleBarHeight(56);
        QCOMPARE(window.titleBarHeight(), 56);
    }
};

QTEST_MAIN(TestFramelessWindow)
#include "test_framelesswindow.moc"
