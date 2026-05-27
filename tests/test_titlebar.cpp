#include <QtTest>
#include <titlebar.h>

class TestTitleBar : public QObject
{
    Q_OBJECT

private slots:
    void defaultButtonsVisible()
    {
        TitleBar tb;
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideMinimizeButton()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideMaximizeButton()
    {
        TitleBar tb;
        tb.setMaximizeButtonVisible(false);
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(!tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideCloseButton()
    {
        TitleBar tb;
        tb.setCloseButtonVisible(false);
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(!tb.isCloseButtonVisible());
    }

    void hideAllButtons()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        tb.setMaximizeButtonVisible(false);
        tb.setCloseButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        QVERIFY(!tb.isMaximizeButtonVisible());
        QVERIFY(!tb.isCloseButtonVisible());
    }

    void toggleButtonVisibility()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        tb.setMinimizeButtonVisible(true);
        QVERIFY(tb.isMinimizeButtonVisible());
    }
};

QTEST_MAIN(TestTitleBar)
#include "test_titlebar.moc"
