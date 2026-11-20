#include <QtTest>
#include <win32/windowhittest.h>

class TestWindowHitTest : public QObject
{
    Q_OBJECT

private slots:
    void resizeBorderThicknessWithNullHwnd()
    {
        const int thickness = WindowHitTest::resizeBorderThickness(nullptr);
        // Logical pixels, so the grab area keeps the same physical size on
        // every DPI. Windows itself uses ~8px.
        QVERIFY(thickness >= 4);
        QVERIFY(thickness <= 12);
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
        QVERIFY(!context.fullScreen);
        QVERIFY(context.snapLayoutEnabled);
    }

    void edgesAtLocalPos_data()
    {
        QTest::addColumn<QPoint>("localPos");
        QTest::addColumn<int>("expected");

        const Qt::Edges none;
        const Qt::Edges left = Qt::LeftEdge;
        const Qt::Edges right = Qt::RightEdge;
        const Qt::Edges top = Qt::TopEdge;
        const Qt::Edges bottom = Qt::BottomEdge;

        QTest::newRow("center")
            << QPoint(400, 300) << static_cast<int>(none);
        QTest::newRow("left edge")
            << QPoint(0, 300) << static_cast<int>(left);
        QTest::newRow("left edge inner bound")
            << QPoint(7, 300) << static_cast<int>(left);
        QTest::newRow("left edge outer bound")
            << QPoint(8, 300) << static_cast<int>(none);
        QTest::newRow("right edge")
            << QPoint(799, 300) << static_cast<int>(right);
        QTest::newRow("top edge")
            << QPoint(400, 0) << static_cast<int>(top);
        QTest::newRow("bottom edge")
            << QPoint(400, 599) << static_cast<int>(bottom);
        QTest::newRow("top left corner")
            << QPoint(2, 2) << static_cast<int>(Qt::TopEdge | Qt::LeftEdge);
        QTest::newRow("bottom right corner")
            << QPoint(797, 597) << static_cast<int>(Qt::BottomEdge | Qt::RightEdge);
        QTest::newRow("outside left")
            << QPoint(-1, 300) << static_cast<int>(none);
        QTest::newRow("outside bottom")
            << QPoint(400, 600) << static_cast<int>(none);
    }

    // The Qt-side cursor/resize path and the native WM_NCHITTEST path used to
    // carry separate thresholds (6px vs 12px native), so the cursor could show
    // a resize arrow where dragging did nothing. Both now share this function.
    void edgesAtLocalPos()
    {
        QFETCH(QPoint, localPos);
        QFETCH(int, expected);

        const Qt::Edges edges = WindowHitTest::edgesAtLocalPos(localPos,
                                                               QSize(800, 600),
                                                               8,
                                                               false,
                                                               false);
        QCOMPARE(static_cast<int>(edges), expected);
    }

    void edgesAtLocalPosWithoutResizeChrome()
    {
        const QPoint corner(1, 1);
        QCOMPARE(static_cast<int>(WindowHitTest::edgesAtLocalPos(
                     corner, QSize(800, 600), 8, true, false)),
                 static_cast<int>(Qt::Edges()));
        QCOMPARE(static_cast<int>(WindowHitTest::edgesAtLocalPos(
                     corner, QSize(800, 600), 8, false, true)),
                 static_cast<int>(Qt::Edges()));
    }

    void edgesAtLocalPosIgnoresNonPositiveBorder()
    {
        QCOMPARE(static_cast<int>(WindowHitTest::edgesAtLocalPos(
                     QPoint(0, 300), QSize(800, 600), 0, false, false)),
                 static_cast<int>(Qt::Edges()));
        QCOMPARE(static_cast<int>(WindowHitTest::edgesAtLocalPos(
                     QPoint(0, 300), QSize(0, 0), 8, false, false)),
                 static_cast<int>(Qt::Edges()));
    }
};

QTEST_MAIN(TestWindowHitTest)
#include "test_windowhittest.moc"
