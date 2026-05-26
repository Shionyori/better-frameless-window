#include <QtTest>
#include <thememanager.h>

class TestThemeManager : public QObject
{
    Q_OBJECT

private slots:
    void defaultStateIsLight()
    {
        ThemeManager tm;
        QCOMPARE(tm.themeMode(), ThemeManager::ThemeMode::Light);
        QVERIFY(!tm.isDarkMode());
    }

    void setAndGetThemeMode()
    {
        ThemeManager tm;
        tm.setThemeMode(ThemeManager::ThemeMode::Dark);
        QCOMPARE(tm.themeMode(), ThemeManager::ThemeMode::Dark);
        QVERIFY(tm.isDarkMode());

        tm.setThemeMode(ThemeManager::ThemeMode::Light);
        QCOMPARE(tm.themeMode(), ThemeManager::ThemeMode::Light);
        QVERIFY(!tm.isDarkMode());
    }

    void defaultAccentColor()
    {
        ThemeManager tm;
        QCOMPARE(tm.accentColor(), QColor(0, 120, 215));
    }

    void setAccentColor()
    {
        ThemeManager tm;
        tm.setAccentColor(QColor(255, 0, 0));
        QCOMPARE(tm.accentColor(), QColor(255, 0, 0));
    }

    void setInvalidAccentColorIsIgnored()
    {
        ThemeManager tm;
        tm.setAccentColor(QColor());
        QCOMPARE(tm.accentColor(), QColor(0, 120, 215));
    }

    void windowBackgroundColor()
    {
        ThemeManager tm;
        QCOMPARE(tm.windowBackgroundColor(), QColor(245, 245, 245));

        tm.setThemeMode(ThemeManager::ThemeMode::Dark);
        QCOMPARE(tm.windowBackgroundColor(), QColor(25, 25, 25));
    }

    void transitionState()
    {
        ThemeManager tm;
        QVERIFY(!tm.isTransitioning());
        QCOMPARE(tm.transitionProgress(), 1.0);

        tm.startTransition(ThemeManager::ThemeMode::Dark);
        QCOMPARE(tm.previousMode(), ThemeManager::ThemeMode::Light);
        QCOMPARE(tm.themeMode(), ThemeManager::ThemeMode::Dark);
        QCOMPARE(tm.transitionProgress(), 0.0);
        QVERIFY(tm.isTransitioning());

        tm.setTransitionProgress(0.5);
        QCOMPARE(tm.transitionProgress(), 0.5);
        QVERIFY(tm.isTransitioning());

        tm.setTransitionProgress(1.0);
        QCOMPARE(tm.transitionProgress(), 1.0);
        QVERIFY(!tm.isTransitioning());
    }

    void transitionProgressIsClamped()
    {
        ThemeManager tm;
        tm.startTransition(ThemeManager::ThemeMode::Dark);

        tm.setTransitionProgress(-0.5);
        QCOMPARE(tm.transitionProgress(), 0.0);

        tm.setTransitionProgress(1.5);
        QCOMPARE(tm.transitionProgress(), 1.0);
    }

    void buildStyleSheetReturnsNonEmpty()
    {
        ThemeManager tm;
        QVERIFY(!tm.buildStyleSheet().isEmpty());
        QVERIFY(!tm.buildStyleSheet(true).isEmpty());

        tm.setThemeMode(ThemeManager::ThemeMode::Dark);
        QVERIFY(!tm.buildStyleSheet().isEmpty());
        QVERIFY(!tm.buildStyleSheet(true).isEmpty());
    }
};

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
