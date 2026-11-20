#include <QtTest>
#include <core/windowgeometry.h>

class TestWindowGeometry : public QObject
{
    Q_OBJECT

private slots:
    void isRestorableInsideScreen()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        QVERIFY(WindowGeometry::isRestorable(QRect(100, 100, 800, 600), screens));
    }

    void isRestorableAcrossTwoScreens()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)};
        // Straddles the seam but the caption strip is visible on both.
        QVERIFY(WindowGeometry::isRestorable(QRect(1800, 200, 800, 600), screens));
    }

    void isRestorableRejectsBarelyVisibleWindow()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        // Only a 5px sliver on the right edge: "intersects" but not grabbable.
        QVERIFY(!WindowGeometry::isRestorable(QRect(1915, 300, 800, 600), screens));
    }

    void isRestorableRejectsWindowAboveTopEdge()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        // Caption strip is off-screen, only the body would be visible.
        QVERIFY(!WindowGeometry::isRestorable(QRect(300, -50, 800, 600), screens));
    }

    void isRestorableRejectsWindowBelowScreen()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        QVERIFY(!WindowGeometry::isRestorable(QRect(300, 1100, 800, 600), screens));
    }

    void isRestorableRejectsInvalidInput()
    {
        QVERIFY(!WindowGeometry::isRestorable(QRect(0, 0, 800, 600), {}));
        QVERIFY(!WindowGeometry::isRestorable(QRect(), {QRect(0, 0, 1920, 1080)}));
    }

    void isRestorableHandlesTinyWindow()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        // A 20px tall window has no full-height caption strip to show, but it
        // is still fully visible, so it must stay restorable.
        QVERIFY(WindowGeometry::isRestorable(QRect(100, 100, 400, 20), screens));
    }

    void clampLeavesRestorableGeometryUntouched()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        const QRect saved(100, 100, 800, 600);
        QCOMPARE(WindowGeometry::clampToNearestScreen(saved, screens), saved);
    }

    void clampCentersWindowFromMissingMonitor()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        const QRect clamped = WindowGeometry::clampToNearestScreen(QRect(2000, 100, 800, 600), screens);
        QCOMPARE(clamped.size(), QSize(800, 600));
        QCOMPARE(clamped, QRect(560, 240, 800, 600));
    }

    void clampKeepsPartiallyVisibleWindowInPlace()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        // Only a sliver of the caption strip is left on screen: push it back
        // inside while keeping the vertical offset the user left it at.
        const QRect clamped = WindowGeometry::clampToNearestScreen(QRect(1915, 300, 800, 600), screens);
        QCOMPARE(clamped, QRect(1120, 300, 800, 600));
    }

    void clampLeavesReachableOverhangAlone()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        // Overhanging the right edge is fine as long as the caption is grabbable.
        const QRect saved(1500, 100, 800, 600);
        QCOMPARE(WindowGeometry::clampToNearestScreen(saved, screens), saved);
    }

    void clampShrinksWindowLargerThanScreen()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080)};
        const QRect clamped = WindowGeometry::clampToNearestScreen(QRect(2000, 100, 3000, 2000), screens);
        QCOMPARE(clamped, QRect(0, 0, 1920, 1080));
    }

    void clampPicksNearestScreen()
    {
        const QList<QRect> screens{QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)};
        // Saved on a monitor that is gone (further right), nearest is screen 2.
        const QRect clamped = WindowGeometry::clampToNearestScreen(QRect(3900, 200, 800, 600), screens);
        QCOMPARE(clamped.size(), QSize(800, 600));
        QCOMPARE(clamped, QRect(2480, 240, 800, 600));
    }

    void clampReturnsInputWithoutScreens()
    {
        const QRect saved(2000, 100, 800, 600);
        QCOMPARE(WindowGeometry::clampToNearestScreen(saved, {}), saved);
    }
};

QTEST_MAIN(TestWindowGeometry)
#include "test_windowgeometry.moc"
