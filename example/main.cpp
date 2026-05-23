#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "framelesswindow.h"
#include "thememanager.h"
#include "win32/windoweffect.h"

static QPushButton *makeSmallBtn(const QString &text, int width = 72)
{
    auto *btn = new QPushButton(text);
    btn->setFixedHeight(26);
    btn->setFixedWidth(width);
    return btn;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Better Frameless Window - Example");

    FramelessWindow window;
    window.setWindowTitle("BFW - Example");
    window.setWindowIcon(QIcon(":/icon.png"));
    window.setBackgroundImage(QPixmap(":/bk.png"));
    window.setWindowSizeLimits(QSize(520, 380), QSize());
    window.restoreWindowGeometry();

    // ── Titlebar: theme toggle + snap layout toggle ───────────────────────

    auto *themeBtn = makeSmallBtn("Dark", 60);
    QObject::connect(themeBtn, &QPushButton::clicked, [&]() {
        const bool toDark = window.themeMode() != ThemeManager::ThemeMode::Dark;
        window.setThemeMode(toDark ? ThemeManager::ThemeMode::Dark
                                   : ThemeManager::ThemeMode::Light);
        themeBtn->setText(toDark ? "Light" : "Dark");
    });

    auto *snapBtn = makeSmallBtn("Snap: ON", 80);
    QObject::connect(snapBtn, &QPushButton::clicked, [&]() {
        const bool next = !window.isSnapLayoutEnabled();
        window.setSnapLayoutEnabled(next);
        snapBtn->setText(next ? "Snap: ON" : "Snap: OFF");
    });

    window.addTitleBarWidget(themeBtn);
    window.addTitleBarWidget(snapBtn);

    // ── Content area ────────────────────────────────────────────────────

    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(10);

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

    // ── Backdrop preference ─────────────────────────────────────────────

    auto makeBdpRow = [&](const QString &label) -> QWidget * {
        auto *w = new QWidget();
        auto *row = new QHBoxLayout(w);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *lbl = new QLabel(label);
        lbl->setObjectName("ContentLabel");
        lbl->setFixedWidth(56);
        row->addWidget(lbl);

        for (const auto &kv : {
                 std::pair{QStringLiteral("Auto"), WindowEffect::SystemBackdropPreference::Auto},
                 std::pair{QStringLiteral("Mica"), WindowEffect::SystemBackdropPreference::Mica},
                 std::pair{QStringLiteral("Acry"), WindowEffect::SystemBackdropPreference::Acrylic},
                 std::pair{QStringLiteral("None"), WindowEffect::SystemBackdropPreference::None},
             }) {
            auto *btn = makeSmallBtn(kv.first, 52);
            QObject::connect(btn, &QPushButton::clicked, [&window, p = kv.second]() {
                window.setSystemBackdropPreference(p);
            });
            row->addWidget(btn);
        }
        row->addStretch();
        return w;
    };

    // ── Background image mode ───────────────────────────────────────────

    auto makeBgRow = [&]() -> QWidget * {
        auto *w = new QWidget();
        auto *row = new QHBoxLayout(w);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *lbl = new QLabel("Bg:");
        lbl->setObjectName("ContentLabel");
        lbl->setFixedWidth(56);
        row->addWidget(lbl);

        for (const auto &kv : {
                 std::pair{QStringLiteral("Cover"), FramelessWindow::BackgroundImageMode::Cover},
                 std::pair{QStringLiteral("Fit"), FramelessWindow::BackgroundImageMode::Fit},
                 std::pair{QStringLiteral("Stretch"), FramelessWindow::BackgroundImageMode::Stretch},
                 std::pair{QStringLiteral("Tile"), FramelessWindow::BackgroundImageMode::Tile},
                 std::pair{QStringLiteral("Center"), FramelessWindow::BackgroundImageMode::Center},
             }) {
            auto *btn = makeSmallBtn(kv.first, 62);
            QObject::connect(btn, &QPushButton::clicked, [&window, mode = kv.second]() {
                window.setBackgroundImage(QPixmap(":/bk.png"), mode);
            });
            row->addWidget(btn);
        }

        auto *clearBtn = makeSmallBtn("Off", 52);
        QObject::connect(clearBtn, &QPushButton::clicked, [&window]() {
            window.clearBackgroundImage();
        });
        row->addWidget(clearBtn);
        row->addStretch();
        return w;
    };

    // ── Titlebar visibility / height ────────────────────────────────────

    auto makeTitleBarRow = [&]() -> QWidget * {
        auto *w = new QWidget();
        auto *row = new QHBoxLayout(w);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *lbl = new QLabel("TitleBar:");
        lbl->setObjectName("ContentLabel");
        lbl->setFixedWidth(56);
        row->addWidget(lbl);

        auto *visBtn = makeSmallBtn("Hide", 62);
        QObject::connect(visBtn, &QPushButton::clicked, [&window, visBtn]() {
            const bool vis = !window.isTitleBarVisible();
            window.setTitleBarVisible(vis);
            visBtn->setText(vis ? "Hide" : "Show");
        });
        row->addWidget(visBtn);

        const int heights[] = {32, 44, 56};
        int heightIdx = 1; // default 44
        auto *hBtn = makeSmallBtn("44px", 56);
        QObject::connect(hBtn, &QPushButton::clicked, [&window, hBtn, &heightIdx, &heights]() {
            heightIdx = (heightIdx + 1) % 3;
            const int h = heights[heightIdx];
            window.setTitleBarHeight(h);
            hBtn->setText(QStringLiteral("%1px").arg(h));
        });
        row->addWidget(hBtn);
        row->addStretch();
        return w;
    };

    // ── Misc toggles ────────────────────────────────────────────────────

    auto makeMiscRow = [&]() -> QWidget * {
        auto *w = new QWidget();
        auto *row = new QHBoxLayout(w);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *lbl = new QLabel("More:");
        lbl->setObjectName("ContentLabel");
        lbl->setFixedWidth(56);
        row->addWidget(lbl);

        auto *fsBtn = makeSmallBtn("FullScr", 68);
        QObject::connect(fsBtn, &QPushButton::clicked, [&window]() {
            if (window.isFullScreen()) {
                window.showNormal();
            } else {
                window.showFullScreen();
            }
        });
        row->addWidget(fsBtn);

        auto *shadowBtn = makeSmallBtn("Shadow: ON", 88);
        QObject::connect(shadowBtn, &QPushButton::clicked, [&window, shadowBtn]() {
            const bool next = !window.isShadowEnabled();
            window.setSystemShadowEnabled(next);
            shadowBtn->setText(next ? "Shadow: ON" : "Shadow: OFF");
        });
        row->addWidget(shadowBtn);

        auto *roundBtn = makeSmallBtn("Round: ON", 88);
        QObject::connect(roundBtn, &QPushButton::clicked, [&window, roundBtn]() {
            const bool next = !window.isRoundedCornersEnabled();
            window.setRoundedCornersEnabled(next);
            roundBtn->setText(next ? "Round: ON" : "Round: OFF");
        });
        row->addWidget(roundBtn);

        auto *dmBtn = makeSmallBtn("DarkFrm: ON", 92);
        QObject::connect(dmBtn, &QPushButton::clicked, [&window, dmBtn]() {
            const bool next = !window.isSystemDarkModeEnabled();
            window.setSystemDarkModeEnabled(next);
            dmBtn->setText(next ? "DarkFrm: ON" : "DarkFrm: OFF");
        });
        row->addWidget(dmBtn);

        row->addStretch();
        return w;
    };

    // ── Assemble ────────────────────────────────────────────────────────

    layout->addStretch();
    layout->addWidget(heading);
    layout->addSpacing(2);
    layout->addWidget(subtitle);
    layout->addSpacing(16);
    layout->addWidget(makeBdpRow("Backdrop:"), 0, Qt::AlignCenter);
    layout->addWidget(makeBgRow(), 0, Qt::AlignCenter);
    layout->addWidget(makeTitleBarRow(), 0, Qt::AlignCenter);
    layout->addWidget(makeMiscRow(), 0, Qt::AlignCenter);
    layout->addStretch();

    window.setCentralWidget(page);
    window.show();
    return app.exec();
}
