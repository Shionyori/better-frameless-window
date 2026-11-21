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
#include <QCheckBox>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QSlider>
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

    // System backdrop is the focus here: Auto lets the library pick the best
    // mode this machine supports, the buttons below force a specific one.
    window.setSystemBackdropEnabled(true);
    window.setSystemBackdropPreference(WindowEffect::SystemBackdropPreference::Auto);

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

    // Theme: animated Light / Dark switch, plus Windows theme following.
    addRow(QStringLiteral("Theme"),
           {makeButton(QStringLiteral("Light"), [&window]() {
                window.setThemeMode(ThemeManager::ThemeMode::Light);
            }),
            makeButton(QStringLiteral("Dark"), [&window]() {
                window.setThemeMode(ThemeManager::ThemeMode::Dark);
            }),
            makeSwitch(QStringLiteral("Follow system"), window.followsSystemTheme(),
                       [&window](bool enabled) {
                           window.setFollowSystemTheme(enabled);
                           // Call syncThemeWithSystemIfFollowing() when Windows
                           // reports a colour change while following.
                       })});

    // Accent colour drives titlebar hover/pressed states; border colour is the
    // Win11 DWMWA_BORDER_COLOR (invalid QColor restores the system default).
    addRow(QStringLiteral("Accent"),
           {makeButton(QStringLiteral("Choose…"), [&window]() {
                const QColor color = QColorDialog::getColor(window.accentColor(), &window,
                                                            QStringLiteral("Accent color"));
                if (color.isValid()) {
                    window.setAccentColor(color);
                }
            })});
    addRow(QStringLiteral("Border"),
           {makeButton(QStringLiteral("Choose…"), [&window]() {
                const QColor color = QColorDialog::getColor(window.borderColor(), &window,
                                                            QStringLiteral("Border color"));
                if (color.isValid()) {
                    window.setBorderColor(color);
                }
            }),
            makeButton(QStringLiteral("Default"), [&window]() {
                window.setBorderColor(QColor());
            })});

    // System backdrop: Auto -> Mica -> Acrylic -> None fallback chain. "None"
    // also drops the translucent surface, so the window looks exactly like the
    // background-image build.
    addRow(QStringLiteral("Backdrop"),
           {makeButton(QStringLiteral("Auto"), [&window]() {
                window.setSystemBackdropPreference(WindowEffect::SystemBackdropPreference::Auto);
            }),
            makeButton(QStringLiteral("None"), [&window]() {
                window.setSystemBackdropPreference(WindowEffect::SystemBackdropPreference::None);
            }),
            makeButton(QStringLiteral("Mica"), [&window]() {
                window.setSystemBackdropPreference(WindowEffect::SystemBackdropPreference::Mica);
            }),
            makeButton(QStringLiteral("Acrylic"), [&window]() {
                window.setSystemBackdropPreference(WindowEffect::SystemBackdropPreference::Acrylic);
            })});

    // ── Background image (commented out while testing the backdrop) ─────
    // A background image and a system backdrop write to the same surface (#10),
    // so only one of them can be live at a time. To go back to the image demo:
    // comment out the two setSystemBackdrop*() calls above.
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

    // DWM visual effects and diagnostics.
    addRow(QStringLiteral("Effects"),
           {makeSwitch(QStringLiteral("Shadow"), window.isShadowEnabled(),
                       [&window](bool enabled) { window.setSystemShadowEnabled(enabled); }),
            makeSwitch(QStringLiteral("Rounded corners"), window.isRoundedCornersEnabled(),
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
