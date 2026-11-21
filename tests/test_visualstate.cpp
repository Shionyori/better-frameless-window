#include <QtTest>
#include <core/windowvisualstate.h>

class TestVisualState : public QObject
{
    Q_OBJECT

private:
    static WindowVisualState::BackdropCapabilities bothBackdrops()
    {
        WindowVisualState::BackdropCapabilities caps;
        caps.mica = true;
        caps.acrylic = true;
        return caps;
    }

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

    // ── Surface resolution ──────────────────────────────────────────────
    // The Qt surface and the DWM backdrop both come from resolveSurfacePolicy(),
    // so these cases pin down the boundaries the rest of the library relies on.

    void surfaceIsOpaqueWithoutBackdrop()
    {
        WindowVisualState::BackdropCapabilities none;
        for (auto mode : {WindowEffect::BackdropMode::Auto,
                          WindowEffect::BackdropMode::Mica,
                          WindowEffect::BackdropMode::Acrylic}) {
            const auto policy = WindowVisualState::resolveSurfacePolicy(mode, false, false, none);
            QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Opaque);
            QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::None);
        }
    }

    void surfaceStaysOpaqueForRequestedNone()
    {
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::None, false, false, bothBackdrops());
        QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Opaque);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::None);
    }

    void surfaceDropsBackdropWhileMinimized()
    {
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::Auto, true, false, bothBackdrops());
        QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Opaque);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::None);
    }

    void backgroundImageWinsOverBackdrop()
    {
        for (auto mode : {WindowEffect::BackdropMode::Auto,
                          WindowEffect::BackdropMode::Mica,
                          WindowEffect::BackdropMode::Acrylic}) {
            const auto policy = WindowVisualState::resolveSurfacePolicy(mode, false, true,
                                                                       bothBackdrops());
            QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Opaque);
            QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::None);
        }
    }

    void autoPrefersMica()
    {
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::Auto, false, false, bothBackdrops());
        QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Translucent);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::Mica);
    }

    void autoFallsBackToAcrylic()
    {
        WindowVisualState::BackdropCapabilities acrylicOnly;
        acrylicOnly.acrylic = true;
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::Auto, false, false, acrylicOnly);
        QCOMPARE(policy.kind, WindowVisualState::SurfaceKind::Translucent);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::Acrylic);
    }

    void requestedMicaFallsBackToAcrylic()
    {
        WindowVisualState::BackdropCapabilities acrylicOnly;
        acrylicOnly.acrylic = true;
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::Mica, false, false, acrylicOnly);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::Acrylic);
    }

    void requestedAcrylicFallsBackToMica()
    {
        WindowVisualState::BackdropCapabilities micaOnly;
        micaOnly.mica = true;
        const auto policy = WindowVisualState::resolveSurfacePolicy(
            WindowEffect::BackdropMode::Acrylic, false, false, micaOnly);
        QCOMPARE(policy.backdrop, WindowEffect::BackdropKind::Mica);
    }

    // A translucent surface always has a backdrop behind it; otherwise the
    // window would expose the desktop instead of blending with anything.
    void translucentImpliesBackdrop()
    {
        for (auto mode : {WindowEffect::BackdropMode::Auto,
                          WindowEffect::BackdropMode::None,
                          WindowEffect::BackdropMode::Mica,
                          WindowEffect::BackdropMode::Acrylic}) {
            for (bool minimized : {false, true}) {
                for (bool hasImage : {false, true}) {
                    const auto policy = WindowVisualState::resolveSurfacePolicy(
                        mode, minimized, hasImage, bothBackdrops());
                    const bool translucent = WindowVisualState::usesTranslucentSurface(policy);
                    QCOMPARE(translucent,
                             policy.backdrop != WindowEffect::BackdropKind::None);
                }
            }
        }
    }

    void buildVisualEffectOptionsPropagatesFields()
    {
        WindowVisualState::SurfacePolicy surface;
        surface.kind = WindowVisualState::SurfaceKind::Translucent;
        surface.backdrop = WindowEffect::BackdropKind::Mica;

        const auto options = WindowVisualState::buildVisualEffectOptions(
            true, true,
            ThemeManager::ThemeMode::Dark,
            false, false,
            surface,
            QColor(255, 0, 0));

        QVERIFY(options.shadowEnabled);
        QVERIFY(options.roundedCornersEnabled);
        QVERIFY(options.darkMode);
        QCOMPARE(options.backdrop, WindowEffect::BackdropKind::Mica);
        QVERIFY(!options.maximized);
        QVERIFY(!options.minimized);
        QCOMPARE(options.borderColor, QColor(255, 0, 0));
    }

    void buildVisualStateTokenIsDeterministic()
    {
        const quint64 a = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false,
            WindowEffect::BackdropKind::Mica,
            ThemeManager::ThemeMode::Light, true);
        const quint64 b = WindowVisualState::buildVisualStateToken(
            true, false, false, true, true, false,
            WindowEffect::BackdropKind::Mica,
            ThemeManager::ThemeMode::Light, true);
        QCOMPARE(a, b);
    }

    void buildVisualStateTokenChangesWithInput()
    {
        const quint64 tokenLight = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false,
            WindowEffect::BackdropKind::None,
            ThemeManager::ThemeMode::Light, false);
        const quint64 tokenDark = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false,
            WindowEffect::BackdropKind::None,
            ThemeManager::ThemeMode::Dark, false);
        QVERIFY(tokenLight != tokenDark);

        const quint64 tokenMica = WindowVisualState::buildVisualStateToken(
            false, false, false, false, false, false,
            WindowEffect::BackdropKind::Mica,
            ThemeManager::ThemeMode::Light, true);
        QVERIFY(tokenLight != tokenMica);
    }

    void shouldClearBackdropBeforeApply_data()
    {
        QTest::addColumn<int>("appliedKind");
        QTest::addColumn<int>("requestedKind");
        QTest::addColumn<bool>("expected");

        QTest::newRow("none -> none")
            << static_cast<int>(WindowEffect::BackdropKind::None)
            << static_cast<int>(WindowEffect::BackdropKind::None) << false;
        QTest::newRow("none -> mica")
            << static_cast<int>(WindowEffect::BackdropKind::None)
            << static_cast<int>(WindowEffect::BackdropKind::Mica) << false;
        QTest::newRow("none -> acrylic")
            << static_cast<int>(WindowEffect::BackdropKind::None)
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic) << false;

        // Stored DWM attributes (Mica) can only be replaced by resetting them.
        QTest::newRow("mica -> mica")
            << static_cast<int>(WindowEffect::BackdropKind::Mica)
            << static_cast<int>(WindowEffect::BackdropKind::Mica) << true;
        QTest::newRow("mica -> acrylic")
            << static_cast<int>(WindowEffect::BackdropKind::Mica)
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic) << true;

        // Acrylic lives in the accent policy: re-applying it is idempotent, so
        // clearing first would drop the blur for a frame.
        QTest::newRow("acrylic -> acrylic")
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic)
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic) << false;
        QTest::newRow("acrylic -> mica")
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic)
            << static_cast<int>(WindowEffect::BackdropKind::Mica) << true;
        QTest::newRow("acrylic -> none")
            << static_cast<int>(WindowEffect::BackdropKind::Acrylic)
            << static_cast<int>(WindowEffect::BackdropKind::None) << true;
    }

    void shouldClearBackdropBeforeApply()
    {
        QFETCH(int, appliedKind);
        QFETCH(int, requestedKind);
        QFETCH(bool, expected);

        QCOMPARE(WindowVisualState::shouldClearBackdropBeforeApply(
                     static_cast<WindowEffect::BackdropKind>(appliedKind),
                     static_cast<WindowEffect::BackdropKind>(requestedKind)),
                 expected);
    }

    void isBackdropApplyRedundant_data()
    {
        QTest::addColumn<int>("requestedKind");
        QTest::addColumn<bool>("requestedDarkMode");
        QTest::addColumn<int>("appliedKind");
        QTest::addColumn<bool>("appliedDarkMode");
        QTest::addColumn<bool>("appliedSuccessfully");
        QTest::addColumn<bool>("expected");

        const int acrylic = static_cast<int>(WindowEffect::BackdropKind::Acrylic);
        const int mica = static_cast<int>(WindowEffect::BackdropKind::Mica);
        const int none = static_cast<int>(WindowEffect::BackdropKind::None);

        QTest::newRow("same everything")
            << acrylic << true << acrylic << true << true << true;
        QTest::newRow("previous apply failed")
            << acrylic << true << acrylic << true << false << false;
        QTest::newRow("kind changed")
            << mica << true << acrylic << true << true << false;
        QTest::newRow("nothing applied yet")
            << none << false << none << false << true << true;

        // Acrylic tint is baked into the accent gradient, so a theme switch must
        // re-apply it even though the backdrop kind itself did not change.
        QTest::newRow("acrylic theme switch")
            << acrylic << false << acrylic << true << true << false;

        // Mica carries no colour of its own (dark mode is a separate attribute),
        // so a theme switch must not force a redundant backdrop round-trip.
        QTest::newRow("mica theme switch")
            << mica << false << mica << true << true << true;
    }

    void isBackdropApplyRedundant()
    {
        QFETCH(int, requestedKind);
        QFETCH(bool, requestedDarkMode);
        QFETCH(int, appliedKind);
        QFETCH(bool, appliedDarkMode);
        QFETCH(bool, appliedSuccessfully);
        QFETCH(bool, expected);

        QCOMPARE(WindowVisualState::isBackdropApplyRedundant(
                     static_cast<WindowEffect::BackdropKind>(requestedKind),
                     requestedDarkMode,
                     static_cast<WindowEffect::BackdropKind>(appliedKind),
                     appliedDarkMode,
                     appliedSuccessfully),
                 expected);
    }
};

QTEST_MAIN(TestVisualState)
#include "test_visualstate.moc"
