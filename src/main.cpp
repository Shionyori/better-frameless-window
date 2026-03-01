#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "framelesswindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FramelessWindow window;
    window.setBackdropEnabled(true);
    window.setBackdropPreference(WindowEffectWin::BackdropPreference::Acrylic);

    // QWidget *businessPage = new QWidget();
    // businessPage->setAttribute(Qt::WA_TranslucentBackground, true);
    // businessPage->setStyleSheet("background: transparent;");
    // // QVBoxLayout *businessLayout = new QVBoxLayout(businessPage);
    // // QLabel *titleLabel = new QLabel("Business Page", businessPage);
    // // QLabel *descriptionLabel = new QLabel("Inject your own UI into FramelessWindow via setCentralWidget.", businessPage);
    // // titleLabel->setObjectName("ContentLabel");
    // // descriptionLabel->setObjectName("ContentLabel");
    // // titleLabel->setAlignment(Qt::AlignCenter);
    // // descriptionLabel->setAlignment(Qt::AlignCenter);
    // // businessLayout->addStretch();
    // // businessLayout->addWidget(titleLabel);
    // // businessLayout->addWidget(descriptionLabel);
    // // businessLayout->addStretch();

    // window.setCentralWidget(businessPage);
    window.show();
    return app.exec();
}