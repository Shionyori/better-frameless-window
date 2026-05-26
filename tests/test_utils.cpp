#include <QtTest>
#include <win32/utils.h>

class TestUtils : public QObject
{
    Q_OBJECT

private slots:
    void toLocalPosWithEmptyRectReturnsZero()
    {
        const QPoint result = Utils::toLocalPos(QPoint(100, 100), QRect(), 800, 600);
        QCOMPARE(result, QPoint(0, 0));
    }

    void toLocalPosWithValidRect()
    {
        const QRect nativeRect(0, 0, 1920, 1080);
        const QPoint result = Utils::toLocalPos(QPoint(960, 540), nativeRect, 800, 600);
        QCOMPARE(result, QPoint(400, 300));
    }

    void toLocalPosClampsToBounds()
    {
        const QRect nativeRect(0, 0, 100, 100);
        const QPoint result = Utils::toLocalPos(QPoint(-50, -50), nativeRect, 100, 100);
        QCOMPARE(result, QPoint(0, 0));
    }

    void hitFromEdgesNonZeroForValidInput()
    {
        QVERIFY(Utils::hitFromEdges(Qt::TopEdge | Qt::LeftEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::TopEdge | Qt::RightEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::BottomEdge | Qt::LeftEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::BottomEdge | Qt::RightEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::TopEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::BottomEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::LeftEdge) != 0);
        QVERIFY(Utils::hitFromEdges(Qt::RightEdge) != 0);
    }

    void hitFromEdgesEmptyReturnsClient()
    {
        const int hit = Utils::hitFromEdges(Qt::Edges());
        QVERIFY(hit == 0 || hit == 1);
    }

    void detectWindowsCapabilitiesDoesNotCrash()
    {
        const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
        Q_UNUSED(caps);
    }

    void isSystemDarkModeEnabledDoesNotCrash()
    {
        const bool enabled = Utils::isSystemDarkModeEnabled();
        Q_UNUSED(enabled);
    }
};

QTEST_MAIN(TestUtils)
#include "test_utils.moc"
