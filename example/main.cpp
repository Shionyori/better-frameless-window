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

    auto addRow = [&layout](const QString &title,
                            const std::initializer_list<QWidget *> &widgets) {
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
                                  const std::function<void(int)> &onChange) {
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

    // Backdrop: Auto resolves to Mica -> Acrylic -> None on the running OS.
    // None also drops the translucent surface, so the window looks exactly like
    // the background-image build.
    auto *autoBackdropButton = makeChoice(QStringLiteral("Auto"), [&window]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Auto);
    });
    auto *noBackdropButton = makeChoice(QStringLiteral("None"), [&window]() {
        window.setBackdropMode(WindowEffect::BackdropMode::None);
    });
    auto *micaBackdropButton = makeChoice(QStringLiteral("Mica"), [&window]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Mica);
    });
    auto *acrylicBackdropButton = makeChoice(QStringLiteral("Acrylic"), [&window]() {
        window.setBackdropMode(WindowEffect::BackdropMode::Acrylic);
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

    // ── Background image (commented out while testing the backdrop) ─────
    // A background image and a backdrop write to the same surface (#10), so the
    // library gives the image precedence: setting one switches the surface to
    // opaque and drops the backdrop. Uncomment this block to go back to the
    // image demo.
    //
    // const QPixmap backgroundImage(QStringLiteral(":/images/bg.jpg"));
    // window.setBackgroundImage(backgroundImage);
    // window.setBackgroundOpacity(0.35);
    //
    // addRow(QStringLiteral("Background"),
    //        {makeButton(QStringLiteral("Cover"), [&window, backgroundImage]() {
    //             window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Cover);
    //         }),
    //         makeButton(QStringLiteral("Fit"), [&window, backgroundImage]() {
    //             window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Fit);
    //         }),
    //         makeButton(QStringLiteral("Stretch"), [&window, backgroundImage]() {
    //             window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Stretch);
    //         }),
    //         makeButton(QStringLiteral("Tile"), [&window, backgroundImage]() {
    //             window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Tile);
    //         }),
    //         makeButton(QStringLiteral("Center"), [&window, backgroundImage]() {
    //             window.setBackgroundImage(backgroundImage, FramelessWindow::BackgroundImageMode::Center);
    //         }),
    //         makeButton(QStringLiteral("Off"), [&window]() {
    //             window.clearBackgroundImage();
    //         })});
    // addSliderRow(QStringLiteral("Image opacity"), 0, 100,
    //              qRound(window.backgroundOpacity() * 100), QStringLiteral("%"),
    //              [&window](int value) { window.setBackgroundOpacity(value / 100.0); });

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

    // DWM visual effects and diagnostics. The shadow switch is left out on
    // purpose: the window surface is translucent by default (Mica), and DWM
    // draws no frame shadow for a layered window, so the toggle would look
    // broken. setShadowEnabled() still works for opaque surfaces.
    addRow(QStringLiteral("Effects"),
           {makeSwitch(QStringLiteral("Rounded corners"), window.isRoundedCornersEnabled(),
                       [&window](bool enabled) { window.setRoundedCornersEnabled(enabled); })});
    addRow(QStringLiteral("Extras"),
           {makeSwitch(QStringLiteral("Snap Layout"), window.isSnapLayoutEnabled(),
                       [&window](bool enabled) { window.setSnapLayoutEnabled(enabled); }),
            makeSwitch(QStringLiteral("Diagnostics"), window.isDiagnosticsEnabled(),
                       [&window](bool enabled) { window.setDiagnosticsEnabled(enabled); })});

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
