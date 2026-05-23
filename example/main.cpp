#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "framelesswindow.h"
#include "thememanager.h"
#include "win32/windoweffect.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Better Frameless Window - Example");

    FramelessWindow window;
    window.setWindowSizeLimits(QSize(520, 380), QSize());
    window.setWindowTitle("BFS - Example");
    window.setWindowIcon(QIcon(":/images/icon.png"));
    window.setBackgroundImage(QPixmap(":/images/bg.jpg"));
    window.setWindowOpacity(1.0);
    window.setBackgroundOpacity(0.30);
    window.restoreWindowGeometry();

    // ── Titlebar: theme toggle ──────────────────────────────────────────

    auto *themeBtn = new QPushButton("Dark");
    themeBtn->setFixedHeight(24);
    QObject::connect(themeBtn, &QPushButton::clicked, [&]() {
        const bool toDark = window.themeMode() != ThemeManager::ThemeMode::Dark;
        window.setThemeMode(toDark ? ThemeManager::ThemeMode::Dark
                                   : ThemeManager::ThemeMode::Light);
        themeBtn->setText(toDark ? "Light" : "Dark");
    });
    window.addTitleBarWidget(themeBtn);

    // ── Content area ────────────────────────────────────────────────────

    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(12);

    auto *heading = new QLabel("Better Frameless Window");
    heading->setObjectName("ContentLabel");
    heading->setAlignment(Qt::AlignCenter);
    QFont headingFont = heading->font();
    headingFont.setPointSize(22);
    headingFont.setBold(true);
    heading->setFont(headingFont);

    auto *subtitle = new QLabel("A Qt6 frameless window library for Windows");
    subtitle->setObjectName("ContentLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    QFont subFont = subtitle->font();
    subFont.setPointSize(11);
    subtitle->setFont(subFont);

    auto *infoLabel1 = new QLabel(
        "Use the buttons below to change the background image mode,\n" 
        "window and background opacity, and the system backdrop preference\n");
    infoLabel1->setObjectName("ContentLabel");
    infoLabel1->setAlignment(Qt::AlignCenter);
    infoLabel1->setWordWrap(true);

    auto *infoLabel2 = new QLabel(
        "By the way,\n"
        "system backdrop effects (Mica/Acrylic) only work on Windows 11.\n"
        "Also conflict with background image,\n"
        "it will only be enbled when the background image is off.");
    infoLabel2->setObjectName("ContentLabel");
    infoLabel2->setAlignment(Qt::AlignCenter);
    infoLabel2->setWordWrap(true);

    // ── Background image mode ───────────────────────────────────────────

    auto *bgRow = new QHBoxLayout();
    bgRow->setSpacing(8);
    auto *bgLabel = new QLabel("Background Mode:");
    bgLabel->setObjectName("ContentLabel");
    bgRow->addStretch();
    bgRow->addWidget(bgLabel);

    auto makeBgBtn = [&](const QString &text,
                         FramelessWindow::BackgroundImageMode mode) {
        auto *btn = new QPushButton(text);
        btn->setFixedHeight(28);
        btn->setFixedWidth(64);
        QObject::connect(btn, &QPushButton::clicked, [&window, mode]() {
            window.setBackgroundImage(QPixmap(":/images/bg.jpg"), mode);
        });
        return btn;
    };

    bgRow->addWidget(makeBgBtn("Cover", FramelessWindow::BackgroundImageMode::Cover));
    bgRow->addWidget(makeBgBtn("Fit", FramelessWindow::BackgroundImageMode::Fit));
    bgRow->addWidget(makeBgBtn("Stretch", FramelessWindow::BackgroundImageMode::Stretch));
    bgRow->addWidget(makeBgBtn("Tile", FramelessWindow::BackgroundImageMode::Tile));
    bgRow->addWidget(makeBgBtn("Center", FramelessWindow::BackgroundImageMode::Center));

    auto *bgOffBtn = new QPushButton("Off");
    bgOffBtn->setFixedHeight(28);
    bgOffBtn->setFixedWidth(48);
    QObject::connect(bgOffBtn, &QPushButton::clicked, [&window]() {
        window.clearBackgroundImage();
    });
    bgRow->addWidget(bgOffBtn);
    bgRow->addStretch();

    // ── Opacity sliders ─────────────────────────────────────────────────

    auto makeOpacityRow = [](const QString &label,
                             std::function<void(int)> onChange,
                             int initial, int fixedLabelWidth) -> QHBoxLayout * {
        auto *row = new QHBoxLayout();
        row->setSpacing(8);
        auto *lbl = new QLabel(label);
        lbl->setObjectName("ContentLabel");
        lbl->setFixedWidth(fixedLabelWidth);
        row->addStretch();
        row->addWidget(lbl);

        auto *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(initial);
        slider->setFixedWidth(120);
        QObject::connect(slider, &QSlider::valueChanged, onChange);
        row->addWidget(slider);

        auto *val = new QLabel(QStringLiteral("%1%").arg(initial));
        val->setObjectName("ContentLabel");
        val->setFixedWidth(50);
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QObject::connect(slider, &QSlider::valueChanged, [val](int v) {
            val->setText(QStringLiteral("%1%").arg(v));
        });
        row->addWidget(val);
        row->addStretch();
        return row;
    };

    auto *winOpacityRow = makeOpacityRow("Window:", [&window](int v) {
        window.setWindowOpacity(v / 100.0);
    }, 100, 110);

    auto *bgOpacityRow = makeOpacityRow("Background:", [&window](int v) {
        window.setBackgroundOpacity(v / 100.0);
    }, 30, 110);

    // ── Backdrop preference buttons ─────────────────────────────────────

    auto *bdpRow = new QHBoxLayout();
    bdpRow->setSpacing(8);
    auto *bdpLabel = new QLabel("Backdrop:");
    bdpLabel->setObjectName("ContentLabel");
    bdpRow->addStretch();
    bdpRow->addWidget(bdpLabel);

    auto makeBdpBtn = [&](const QString &text,
                          WindowEffect::SystemBackdropPreference pref) {
        auto *btn = new QPushButton(text);
        btn->setFixedHeight(28);
        btn->setFixedWidth(72);
        QObject::connect(btn, &QPushButton::clicked, [&window, pref]() {
            window.setSystemBackdropPreference(pref);
        });
        return btn;
    };

    auto *autoBtn = makeBdpBtn("Auto", WindowEffect::SystemBackdropPreference::Auto);
    auto *micaBtn = makeBdpBtn("Mica", WindowEffect::SystemBackdropPreference::Mica);
    auto *acrylicBtn = makeBdpBtn("Acrylic", WindowEffect::SystemBackdropPreference::Acrylic);
    auto *noneBtn = makeBdpBtn("None", WindowEffect::SystemBackdropPreference::None);

    bdpRow->addWidget(autoBtn);
    bdpRow->addWidget(micaBtn);
    bdpRow->addWidget(acrylicBtn);
    bdpRow->addWidget(noneBtn);
    bdpRow->addStretch();

    // ── Assemble ────────────────────────────────────────────────────────

    layout->addStretch();
    layout->addWidget(heading);
    layout->addSpacing(4);
    layout->addWidget(subtitle);
    layout->addSpacing(20);
    layout->addWidget(infoLabel1);
    layout->addSpacing(20);
    layout->addWidget(infoLabel2);
    layout->addSpacing(20);
    layout->addLayout(bgRow);
    layout->addSpacing(6);
    layout->addLayout(winOpacityRow);
    layout->addSpacing(4);
    layout->addLayout(bgOpacityRow);
    layout->addSpacing(6);
    layout->addLayout(bdpRow);
    layout->addStretch();

    window.setCentralWidget(page);
    window.show();
    return app.exec();
}
