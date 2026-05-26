#include <QtTest>
#include <core/windowvisualstate.h>

class TestVisualState : public QObject
{
    Q_OBJECT

private slots:
    void shouldUseDarkMode_data()
    {
        QTest::addColumn<int>("themeMode");
        QTest::addColumn<bool>("expected");

        QTest::newRow("light") << static_cast<int>(ThemeManager::ThemeMode::Light) << false;
        QTest::newRow("dark")  << static_cast<int>(ThemeManager::ThemeMode::Dark)  << true;
    }

    void shouldUseDarkMode()
    {
        QFETCH(int, themeMode);
        QFETCH(bool, expected);
        QCOMPARE(WindowVisualState::shouldUseDarkMode(
                     static_cast<ThemeManager::ThemeMode>(themeMode)),
                 expected);
    }

    void shouldUseTranslucentBackgroundWhenDisabled()
    {
        QVERIFY(!WindowVisualState::shouldUseTranslucentBackground(
            false, false, WindowEffect::SystemBackdropPreference::Auto));
        QVERIFY(!WindowVisualState::shouldUseTranslucentBackground(
            false, false, WindowEffect::SystemBackdropPreference::Mica));
        QVERIFY(!WindowVisualState::shouldUseTranslucentBackground(
            false, false, WindowEffect::SystemBackdropPreference::Acrylic));
    }

    void shouldUseTranslucentBackgroundWhenMinimized()
    {
        QVERIFY(!WindowVisualState::shouldUseTranslucentBackground(
            true, true, WindowEffect::SystemBackdropPreference::Auto));
    }

    void shouldUseTranslucentBackgroundPreferenceNone()
    {
        QVERIFY(!WindowVisualState::shouldUseTranslucentBackground(
            true, false, WindowEffect::SystemBackdropPreference::None));
    }

    void buildVisualEffectOptionsPropagatesFields()
    {
        const auto options = WindowVisualState::buildVisualEffectOptions(
            true, false,
            WindowEffect::SystemBackdropPreference::Mica,
            true, true,
            ThemeManager::ThemeMode::Dark,
            false, false,
            QColor(255, 0, 0));

        QVERIFY(options.shadowEnabled);
        QVERIFY(!options.systemBackdropEnabled);
        QCOMPARE(options.systemBackdropPreference,
                 WindowEffect::SystemBackdropPreference::Mica);
        QVERIFY(options.roundedCornersEnabled);
        QVERIFY(options.systemDarkModeEnabled);
        QVERIFY(options.useDarkMode);
        QVERIFY(!options.maximized);
        QVERIFY(!options.minimized);
        QCOMPARE(options.borderColor, QColor(255, 0, 0));
    }

    void buildVisualStateTokenIsDeterministic()
    {
        const quint64 a = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false, true, true,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        const quint64 b = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false, true, true,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        QCOMPARE(a, b);
    }

    void buildVisualStateTokenChangesWithInput()
    {
        const quint64 tokenLight = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false, false, false,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        const quint64 tokenDark = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false, false, false,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Dark, false);
        QVERIFY(tokenLight != tokenDark);
    }
};

QTEST_MAIN(TestVisualState)
#include "test_visualstate.moc"
