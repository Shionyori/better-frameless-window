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

    // Background colour / image / opacity belong to BackdropMode::None; the
    // setters stay usable but only that surface paints them.
    void backgroundColourRoundTrip()
    {
        FramelessWindow window;
        QVERIFY(!window.backgroundColor().isValid());

        window.setBackgroundColor(QColor(12, 34, 56));
        QCOMPARE(window.backgroundColor(), QColor(12, 34, 56));

        window.setBackgroundColor(QColor());
        QVERIFY(!window.backgroundColor().isValid());
    }

    void noBackdropMeansTheApplicationPaints()
    {
        FramelessWindow window;
        window.setBackdropMode(WindowEffect::BackdropMode::None);
        QVERIFY(!window.isBackdropActive());

        // Setting an application background does not turn a material back into
        // an opaque surface: the requested mode keeps ownership.
        window.setBackgroundImage(QPixmap(4, 4));
        QVERIFY(!window.isBackdropActive());
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
