#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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
    window.setWindowTitle("Example");

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

    auto *subtitle = new QLabel("A Qt6 C++17 frameless window library for Windows");
    subtitle->setObjectName("ContentLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    QFont subFont = subtitle->font();
    subFont.setPointSize(11);
    subtitle->setFont(subFont);

    auto *infoLabel = new QLabel(
        "Features: custom titlebar  |  8-direction resize  |  Mica / Acrylic backdrop\n"
        "Rounded corners  |  System dark mode  |  HiDPI-aware hit testing\n"
        "Drag-to-snap  |  Right-click system menu  |  Shadow control");
    infoLabel->setObjectName("ContentLabel");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);

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
    layout->addWidget(infoLabel);
    layout->addSpacing(20);
    layout->addLayout(bdpRow);
    layout->addStretch();

    window.setCentralWidget(page);
    window.show();
    return app.exec();
}
