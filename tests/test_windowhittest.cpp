#include <QtTest>
#include <win32/windowhittest.h>

class TestWindowHitTest : public QObject
{
    Q_OBJECT

private slots:
    void resizeBorderThicknessWithNullHwnd()
    {
        const int thickness = WindowHitTest::resizeBorderThickness(nullptr);
        QVERIFY(thickness >= 0);
        QVERIFY(thickness <= 6);
    }

    void nonClientHitTestWithNullHwnd()
    {
        WindowHitTest::Context context;
        context.hwnd = nullptr;
        context.logicalWidth = 800;
        context.logicalHeight = 600;

        const int result = WindowHitTest::nonClientHitTest(context, QPoint(400, 300));
        QVERIFY(result == 0 || result == 1);
    }

    void nonClientHitTestWithNullTitleRegionResolver()
    {
        WindowHitTest::Context context;
        context.hwnd = nullptr;
        context.logicalWidth = 800;
        context.logicalHeight = 600;
        context.titleRegionResolver = nullptr;

        const int result = WindowHitTest::nonClientHitTest(context, QPoint(400, 300));
        QVERIFY(result == 0 || result == 1);
    }

    void contextDefaultValues()
    {
        WindowHitTest::Context context;
        QVERIFY(context.hwnd == nullptr);
        QCOMPARE(context.logicalWidth, 0);
        QCOMPARE(context.logicalHeight, 0);
        QVERIFY(!context.maximized);
        QVERIFY(context.snapLayoutEnabled);
    }
};

QTEST_MAIN(TestWindowHitTest)
#include "test_windowhittest.moc"
