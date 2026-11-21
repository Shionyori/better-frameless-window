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
            true,
            ThemeManager::ThemeMode::Dark,
            false, false,
            QColor(255, 0, 0));

        QVERIFY(options.shadowEnabled);
        QVERIFY(!options.systemBackdropEnabled);
        QCOMPARE(options.systemBackdropPreference,
                 WindowEffect::SystemBackdropPreference::Mica);
        QVERIFY(options.roundedCornersEnabled);
        QVERIFY(options.useDarkMode);
        QVERIFY(!options.maximized);
        QVERIFY(!options.minimized);
        QCOMPARE(options.borderColor, QColor(255, 0, 0));
    }

    void buildVisualStateTokenIsDeterministic()
    {
        const quint64 a = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false, true,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        const quint64 b = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false, true,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        QCOMPARE(a, b);
    }

    void buildVisualStateTokenChangesWithInput()
    {
        const quint64 tokenLight = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false, false,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Light, false);
        const quint64 tokenDark = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false, false,
            WindowEffect::SystemBackdropPreference::Auto,
            ThemeManager::ThemeMode::Dark, false);
        QVERIFY(tokenLight != tokenDark);
    }

    void shouldClearBackdropBeforeApply_data()
    {
        QTest::addColumn<int>("appliedMode");
        QTest::addColumn<int>("requestedMode");
        QTest::addColumn<bool>("expected");

        QTest::newRow("none -> none")
            << static_cast<int>(WindowEffect::SystemBackdropMode::None)
            << static_cast<int>(WindowEffect::SystemBackdropMode::None) << false;
        QTest::newRow("none -> mica")
            << static_cast<int>(WindowEffect::SystemBackdropMode::None)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Mica) << false;
        QTest::newRow("none -> acrylic")
            << static_cast<int>(WindowEffect::SystemBackdropMode::None)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic) << false;

        // Stored DWM attributes (Mica) can only be replaced by resetting them.
        QTest::newRow("mica -> mica")
            << static_cast<int>(WindowEffect::SystemBackdropMode::Mica)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Mica) << true;
        QTest::newRow("mica -> acrylic")
            << static_cast<int>(WindowEffect::SystemBackdropMode::Mica)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic) << true;

        // Acrylic lives in the accent policy: re-applying it is idempotent, so
        // clearing first would drop the blur for a frame.
        QTest::newRow("acrylic -> acrylic")
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic) << false;
        QTest::newRow("acrylic -> mica")
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic)
            << static_cast<int>(WindowEffect::SystemBackdropMode::Mica) << true;
        QTest::newRow("acrylic -> none")
            << static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic)
            << static_cast<int>(WindowEffect::SystemBackdropMode::None) << true;
    }

    void shouldClearBackdropBeforeApply()
    {
        QFETCH(int, appliedMode);
        QFETCH(int, requestedMode);
        QFETCH(bool, expected);

        QCOMPARE(WindowVisualState::shouldClearBackdropBeforeApply(
                     static_cast<WindowEffect::SystemBackdropMode>(appliedMode),
                     static_cast<WindowEffect::SystemBackdropMode>(requestedMode)),
                 expected);
    }

    void isBackdropApplyRedundant_data()
    {
        QTest::addColumn<int>("requestedMode");
        QTest::addColumn<bool>("requestedUseDarkMode");
        QTest::addColumn<int>("appliedMode");
        QTest::addColumn<bool>("appliedUseDarkMode");
        QTest::addColumn<bool>("appliedSuccessfully");
        QTest::addColumn<bool>("expected");

        const int acrylic = static_cast<int>(WindowEffect::SystemBackdropMode::Acrylic);
        const int mica = static_cast<int>(WindowEffect::SystemBackdropMode::Mica);
        const int none = static_cast<int>(WindowEffect::SystemBackdropMode::None);

        QTest::newRow("same everything")
            << acrylic << true << acrylic << true << true << true;
        QTest::newRow("previous apply failed")
            << acrylic << true << acrylic << true << false << false;
        QTest::newRow("mode changed")
            << mica << true << acrylic << true << true << false;
        QTest::newRow("nothing applied yet")
            << none << false << none << false << true << true;

        // Acrylic tint is baked into the accent gradient, so a theme switch must
        // re-apply it even though the backdrop mode itself did not change.
        QTest::newRow("acrylic theme switch")
            << acrylic << false << acrylic << true << true << false;

        // Mica carries no colour of its own (dark mode is a separate attribute),
        // so a theme switch must not force a redundant backdrop round-trip.
        QTest::newRow("mica theme switch")
            << mica << false << mica << true << true << true;
    }

    void isBackdropApplyRedundant()
    {
        QFETCH(int, requestedMode);
        QFETCH(bool, requestedUseDarkMode);
        QFETCH(int, appliedMode);
        QFETCH(bool, appliedUseDarkMode);
        QFETCH(bool, appliedSuccessfully);
        QFETCH(bool, expected);

        QCOMPARE(WindowVisualState::isBackdropApplyRedundant(
                     static_cast<WindowEffect::SystemBackdropMode>(requestedMode),
                     requestedUseDarkMode,
                     static_cast<WindowEffect::SystemBackdropMode>(appliedMode),
                     appliedUseDarkMode,
                     appliedSuccessfully),
                 expected);
    }
};

QTEST_MAIN(TestVisualState)
#include "test_visualstate.moc"
