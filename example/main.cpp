// better-frameless-window — a single-window tour of the library features.
//
// Every row in the panel maps to one feature group of FramelessWindow:
// theme, accent/border colours, system backdrop, titlebar and DWM visual
// effects. Window geometry and theme mode are persisted by the library.
//
// The background image and opacity blocks are commented out for now so the
// system backdrop can be inspected on its own (image and backdrop fight over
// the same surface, #10).

#include <QApplication>
#include <QButtonGroup>
#include <QGuiApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QSlider>
#include <QStyleHints>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <utility>

#include "framelesswindow.h"
#include "thememanager.h"
#include "version.h"

namespace {

// Plain QLabels / QCheckBoxes are not covered by the library style sheet, so
// borrow the themed #ContentLabel colour and shrink it to control size.
QLabel *makeLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("ContentLabel"));
    label->setStyleSheet(QStringLiteral("font-size: 12px;"));
    label->setMinimumWidth(100);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

QPushButton *makeButton(const QString &text, std::function<void()> onClick)
{
    auto *button = new QPushButton(text);
    button->setFixedHeight(26);
    QObject::connect(button, &QPushButton::clicked, std::move(onClick));
    return button;
}

// A checkable button: mutually exclusive groups use these so the current
// selection is visible, unlike a plain push button that looks identical in
// every state.
QPushButton *makeChoice(const QString &text, std::function<void()> onClick)
{
    auto *button = new QPushButton(text);
    button->setFixedHeight(26);
    button->setCheckable(true);
    QObject::connect(button, &QPushButton::clicked, std::move(onClick));
    return button;
}

// Shows the current colour on the button itself; an invalid colour means the
// platform default and falls back to a plain label.
void applyColorSwatch(QPushButton *button, const QColor &color, const QString &fallbackText)
{
    if (!color.isValid()) {
        button->setText(fallbackText);
        button->setStyleSheet(QString());
        return;
    }

    button->setText(color.name().toUpper());
    const QString label = color.lightnessF() > 0.55 ? QStringLiteral("#1A1A1A")
                                                    : QStringLiteral("#FFFFFF");
    button->setStyleSheet(QStringLiteral("background-color: %1; color: %2;")
                              .arg(color.name(), label));
}

QCheckBox *makeSwitch(const QString &text, bool checked, std::function<void(bool)> onToggled)
{
    auto *box = new QCheckBox(text);
    box->setObjectName(QStringLiteral("ContentLabel"));
    box->setStyleSheet(QStringLiteral("font-size: 12px;"));
    box->setChecked(checked);
    QObject::connect(box, &QCheckBox::toggled, std::move(onToggled));
    return box;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("better-frameless-window example"));

    FramelessWindow window;
    window.setWindowTitle(QStringLiteral("better-frameless-window example"));
    window.setWindowIcon(QIcon(QStringLiteral(":/images/icon.png")));
    window.setWindowSizeLimits(QSize(640, 520));

    // BackdropMode::Auto lets the library pick the best backdrop this machine
    // supports; BackdropMode::None is the "off" switch. The buttons in the
    // panel force a specific mode.
    window.setBackdropMode(WindowEffect::BackdropMode::Auto);

    // The application-painted surface: a photo blended 30% over the theme
    // colour. Dormant while a material owns the surface, painted on None.
    const QPixmap backgroundImage(QStringLiteral(":/images/bg.jpg"));
    window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Cover);
    window.setBackgroundOpacity(0.30);

    window.restoreWindowGeometry(); // saved automatically on close

    // ── Titlebar extension slot ─────────────────────────────────────────
    auto *versionLabel = new QLabel(QStringLiteral("v%1.%2.%3")
                                        .arg(BETTER_FRAMELESS_WINDOW_VERSION_MAJOR)
                                        .arg(BETTER_FRAMELESS_WINDOW_VERSION_MINOR)
                                        .arg(BETTER_FRAMELESS_WINDOW_VERSION_PATCH));
    versionLabel->setObjectName(QStringLiteral("TitleBarLabel"));
    versionLabel->setStyleSheet(QStringLiteral("padding-left: 12px;"));
    window.addTitleBarWidget(versionLabel);

    // ── Control panel ───────────────────────────────────────────────────
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 20, 28, 20);
    layout->setSpacing(10);

    auto addRow = [&layout](const QString &title, const QList<QWidget *> &widgets) {
        auto *row = new QHBoxLayout;
        row->setSpacing(8);
        row->addWidget(makeLabel(title));
        for (QWidget *widget : widgets) {
            row->addWidget(widget);
        }
        row->addStretch();
        layout->addLayout(row);
    };

    auto addSliderRow = [&addRow](const QString &title, int minimum, int maximum, int value,
                                  const QString &suffix,
                                  const std::function<void(int)> &onChange) -> QList<QWidget *> {
        auto *valueLabel = new QLabel(QString::number(value) + suffix);
        valueLabel->setObjectName(QStringLiteral("ContentLabel"));
        valueLabel->setStyleSheet(QStringLiteral("font-size: 12px;"));
        valueLabel->setFixedWidth(48);

        auto *slider = new QSlider(Qt::Horizontal);
        slider->setRange(minimum, maximum);
        slider->setValue(value);
        slider->setFixedWidth(200);

        QObject::connect(slider, &QSlider::valueChanged, valueLabel, [valueLabel, suffix](int v) {
            valueLabel->setText(QString::number(v) + suffix);
        });
        QObject::connect(slider, &QSlider::valueChanged, onChange);

        addRow(title, {slider, valueLabel});
        return {slider, valueLabel};
    };

    // Theme: exactly one of System / Light / Dark is active, so they are one
    // exclusive group instead of two buttons plus a "follow system" switch that
    // silently overrides them.
    std::function<void()> syncThemeButtons;
    auto *systemThemeButton = makeChoice(QStringLiteral("System"), [&]() {
        window.setFollowSystemTheme(true);
        window.syncThemeWithSystemIfFollowing();
        syncThemeButtons();
    });
    auto *lightThemeButton = makeChoice(QStringLiteral("Light"), [&]() {
        window.setFollowSystemTheme(false);
        window.setThemeMode(ThemeManager::ThemeMode::Light);
        syncThemeButtons();
    });
    auto *darkThemeButton = makeChoice(QStringLiteral("Dark"), [&]() {
        window.setFollowSystemTheme(false);
        window.setThemeMode(ThemeManager::ThemeMode::Dark);
        syncThemeButtons();
    });

    auto *themeGroup = new QButtonGroup(&window);
    themeGroup->setExclusive(true);
    themeGroup->addButton(systemThemeButton);
    themeGroup->addButton(lightThemeButton);
    themeGroup->addButton(darkThemeButton);

    syncThemeButtons = [&window, systemThemeButton, lightThemeButton, darkThemeButton]() {
        const bool following = window.followsSystemTheme();
        systemThemeButton->setChecked(following);
        lightThemeButton->setChecked(!following
                                     && window.themeMode() == ThemeManager::ThemeMode::Light);
        darkThemeButton->setChecked(!following
                                    && window.themeMode() == ThemeManager::ThemeMode::Dark);
    };
    syncThemeButtons();

    // Windows also reports its own colour-scheme changes while following.
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &window,
                     [&window, syncThemeButtons](Qt::ColorScheme) {
                         if (!window.followsSystemTheme()) {
                             return;
                         }
                         window.syncThemeWithSystemIfFollowing();
                         syncThemeButtons();
                     });

    addRow(QStringLiteral("Theme"),
           {systemThemeButton, lightThemeButton, darkThemeButton});

    // Accent colour drives the titlebar hover, the selected state of every
    // checkable button and the border colour picker. The button shows the value
    // that is currently in effect.
    auto *accentSwatch = new QPushButton;
    accentSwatch->setFixedHeight(26);
    accentSwatch->setMinimumWidth(96);
    applyColorSwatch(accentSwatch, window.accentColor(), QStringLiteral("Pick…"));
    QObject::connect(accentSwatch, &QPushButton::clicked, &window, [&window, accentSwatch]() {
        // No parent on purpose: a dialog parented to the window inherits its
        // style sheet (theme text colours) while it is painted with the OS
        // palette, which can make the OK/Cancel buttons invisible when the app
        // theme and the system theme disagree.
        const QColor color = QColorDialog::getColor(window.accentColor(), nullptr,
                                                    QStringLiteral("Accent color"));
        if (color.isValid()) {
            window.setAccentColor(color);
            applyColorSwatch(accentSwatch, window.accentColor(), QStringLiteral("Pick…"));
        }
    });
    addRow(QStringLiteral("Accent"), {accentSwatch});

    auto *borderSwatch = new QPushButton;
    borderSwatch->setFixedHeight(26);
    borderSwatch->setMinimumWidth(96);
    auto *borderDefaultButton = new QPushButton(QStringLiteral("Default"));
    borderDefaultButton->setFixedHeight(26);
    QObject::connect(borderDefaultButton, &QPushButton::clicked, &window,
                     [&window, borderSwatch, borderDefaultButton]() {
                         window.setBorderColor(QColor());
                         applyColorSwatch(borderSwatch, window.borderColor(),
                                          QStringLiteral("\u2014"));
                         borderDefaultButton->setEnabled(false);
                     });

    QObject::connect(borderSwatch, &QPushButton::clicked, &window,
                     [&window, borderSwatch, borderDefaultButton]() {
                         const QColor color = QColorDialog::getColor(window.borderColor(), nullptr,
                                                                     QStringLiteral("Border color"));
                         if (!color.isValid()) {
                             return;
                         }
                         window.setBorderColor(color);
                         applyColorSwatch(borderSwatch, window.borderColor(), QStringLiteral("\u2014"));
                         borderDefaultButton->setEnabled(true);
                     });

    applyColorSwatch(borderSwatch, window.borderColor(), QStringLiteral("\u2014"));
    borderDefaultButton->setEnabled(window.borderColor().isValid());
    addRow(QStringLiteral("Border"), {borderSwatch, borderDefaultButton});

    // Three surface families, kept apart on purpose:
    //   None    - the application paints the background (colour / image / opacity)
    //   Mica    - the system derives a near-solid colour from the wallpaper
    //   Acrylic - a translucent frosted surface
    // The material owns the surface, so the background controls are disabled
    // while one is active instead of silently doing nothing.
    QList<QWidget *> backgroundControlList;
    std::function<void()> syncBackgroundControls;

    auto *autoBackdropButton = makeChoice(QStringLiteral("Auto"), [&]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Auto);
        syncBackgroundControls();
    });
    auto *noBackdropButton = makeChoice(QStringLiteral("None"), [&]() {
        window.setBackdropMode(WindowEffect::BackdropMode::None);
        syncBackgroundControls();
    });
    auto *micaBackdropButton = makeChoice(QStringLiteral("Mica"), [&]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Mica);
        syncBackgroundControls();
    });
    auto *acrylicBackdropButton = makeChoice(QStringLiteral("Acrylic"), [&]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Acrylic);
        syncBackgroundControls();
    });

    auto *backdropGroup = new QButtonGroup(&window);
    backdropGroup->setExclusive(true);
    for (QPushButton *button : {autoBackdropButton, noBackdropButton,
                                micaBackdropButton, acrylicBackdropButton}) {
        backdropGroup->addButton(button);
    }

    switch (window.backdropMode()) {
    case WindowEffect::BackdropMode::Auto:
        autoBackdropButton->setChecked(true);
        break;
    case WindowEffect::BackdropMode::None:
        noBackdropButton->setChecked(true);
        break;
    case WindowEffect::BackdropMode::Mica:
        micaBackdropButton->setChecked(true);
        break;
    case WindowEffect::BackdropMode::Acrylic:
        acrylicBackdropButton->setChecked(true);
        break;
    }

    addRow(QStringLiteral("Backdrop"),
           {autoBackdropButton, noBackdropButton, micaBackdropButton, acrylicBackdropButton});

    // ── Application-painted background (BackdropMode::None only) ────────
    // One exclusive group, so the active image mode is visible; Off is part of
    // the group instead of a separate button that leaves the mode ambiguous.
    QList<QWidget *> backgroundButtons;
    QList<QPushButton *> imageModeButtons;
    const QList<QPair<QString, FramelessWindow::BackgroundImageMode>> backgroundModes = {
        {QStringLiteral("Cover"), FramelessWindow::BackgroundImageMode::Cover},
        {QStringLiteral("Fit"), FramelessWindow::BackgroundImageMode::Fit},
        {QStringLiteral("Stretch"), FramelessWindow::BackgroundImageMode::Stretch},
        {QStringLiteral("Tile"), FramelessWindow::BackgroundImageMode::Tile},
        {QStringLiteral("Center"), FramelessWindow::BackgroundImageMode::Center},
    };
    for (const auto &entry : backgroundModes) {
        auto *button = makeChoice(entry.first, [&window, backgroundImage, entry]() {
            window.setBackgroundImage(backgroundImage, entry.second);
        });
        imageModeButtons.append(button);
        backgroundButtons.append(button);
    }
    auto *noImageButton = makeChoice(QStringLiteral("Off"), [&window]() {
        window.clearBackgroundImage();
    });
    imageModeButtons.append(noImageButton);
    backgroundButtons.append(noImageButton);

    auto *imageGroup = new QButtonGroup(&window);
    imageGroup->setExclusive(true);
    for (QPushButton *button : imageModeButtons) {
        imageGroup->addButton(button);
    }

    if (window.backgroundImage().isNull()) {
        noImageButton->setChecked(true);
    } else {
        for (int i = 0; i < backgroundModes.size(); ++i) {
            if (backgroundModes.at(i).second == window.backgroundImageMode()) {
                imageModeButtons.at(i)->setChecked(true);
                break;
            }
        }
    }

    addRow(QStringLiteral("Image"), backgroundButtons);
    backgroundControlList.append(backgroundButtons);

    auto *backgroundSwatch = new QPushButton;
    backgroundSwatch->setFixedHeight(26);
    backgroundSwatch->setMinimumWidth(96);
    auto *backgroundThemeButton = makeButton(QStringLiteral("Theme"), [&]() {
        window.setBackgroundColor(QColor());
        applyColorSwatch(backgroundSwatch, window.backgroundColor(), QStringLiteral("\u2014"));
    });
    QObject::connect(backgroundSwatch, &QPushButton::clicked, &window,
                     [&window, backgroundSwatch]() {
                         const QColor color = QColorDialog::getColor(window.backgroundColor(),
                                                                     nullptr,
                                                                     QStringLiteral("Background color"));
                         if (!color.isValid()) {
                             return;
                         }
                         window.setBackgroundColor(color);
                         applyColorSwatch(backgroundSwatch, window.backgroundColor(),
                                          QStringLiteral("\u2014"));
                     });
    applyColorSwatch(backgroundSwatch, window.backgroundColor(), QStringLiteral("\u2014"));
    addRow(QStringLiteral("Color"), {backgroundSwatch, backgroundThemeButton});
    backgroundControlList << backgroundSwatch << backgroundThemeButton;

    backgroundControlList.append(addSliderRow(QStringLiteral("Image opacity"), 0, 100,
                                              qRound(window.backgroundOpacity() * 100),
                                              QStringLiteral("%"), [&window](int value) {
                                                  window.setBackgroundOpacity(value / 100.0);
                                              }));

    auto *backgroundHint = new QLabel;
    backgroundHint->setObjectName(QStringLiteral("ContentLabel"));
    backgroundHint->setStyleSheet(QStringLiteral("font-size: 11px;"));
    backgroundHint->setWordWrap(true);
    layout->addWidget(backgroundHint);

    syncBackgroundControls = [&window, backgroundControlList, backgroundHint]() {
        // Ask the window, not the requested mode: Auto may resolve to an opaque
        // surface on a machine without materials.
        const bool appOwned = !window.isBackdropActive();
        for (QWidget *widget : backgroundControlList) {
            widget->setEnabled(appOwned);
        }
        backgroundHint->setText(appOwned
                                    ? QStringLiteral("Background is painted by the application.")
                                    : QStringLiteral("Background is owned by the material "
                                                     "\u2014 select Backdrop None to edit it."));
    };
    syncBackgroundControls();

    // Window opacity is skipped as well: stacking QWidget::setWindowOpacity() on
    // top of a translucent backdrop makes the backdrop itself impossible to judge.
    // addSliderRow(QStringLiteral("Window opacity"), 0, 100,
    //              qRound(window.windowOpacity() * 100), QStringLiteral("%"),
    //              [&window](int value) { window.setWindowOpacity(value / 100.0); });

    // Titlebar: height, whole-bar visibility and the three control buttons.
    addSliderRow(QStringLiteral("Titlebar height"), 30, 64, window.titleBarHeight(),
                 QStringLiteral(" px"), [&window](int value) {
                     window.setTitleBarHeight(value);
                 });
    addRow(QStringLiteral("Titlebar"),
           {makeSwitch(QStringLiteral("Visible"), window.isTitleBarVisible(),
                       [&window](bool visible) { window.setTitleBarVisible(visible); }),
            makeSwitch(QStringLiteral("Minimize"), window.isMinimizeButtonVisible(),
                       [&window](bool visible) { window.setMinimizeButtonVisible(visible); }),
            makeSwitch(QStringLiteral("Maximize"), window.isMaximizeButtonVisible(),
                       [&window](bool visible) { window.setMaximizeButtonVisible(visible); }),
            makeSwitch(QStringLiteral("Close"), window.isCloseButtonVisible(),
                       [&window](bool visible) { window.setCloseButtonVisible(visible); })});

    // Not exposed in the panel:
    // - Snap Layout is on by default; setSnapLayoutEnabled() is for apps that
    //   draw their own maximize button and want to opt out.
    // - Diagnostics is a debugging aid, not a feature: off by default, switched
    //   on with setDiagnosticsEnabled(true) when a DWM effect misbehaves (see
    //   the README).
    // - The DWM shadow needs an opaque surface and a material is layered, so a
    //   toggle would look broken (setShadowEnabled() still works on None).
    // - The rounded corner preference is not cosmetic: DWM uses it for the whole
    //   frame it draws (border colour, shadow shape). It stays on.

    layout->addStretch();
    auto *hint = new QLabel(QStringLiteral("Drag the titlebar to move · double-click to "
                                           "maximize · right-click for the system menu"));
    hint->setObjectName(QStringLiteral("ContentLabel"));
    hint->setStyleSheet(QStringLiteral("font-size: 11px;"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    window.setCentralWidget(panel);
    window.show();
    return app.exec();
}
