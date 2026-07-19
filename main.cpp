#include "mainwindow.h"
#include "gitmanager.h"

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QColor>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LoopGit"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("MyCompany"));
    app.setWindowIcon(QIcon(":/icons/app_icon.ico"));

    // Make the white background transparent by masking it to a perfect circle
    QImage logo(":/resources/logo.png");
    logo = logo.convertToFormat(QImage::Format_ARGB32);
    float cx = logo.width() / 2.0f;
    float cy = logo.height() / 2.0f;
    float radius = cx - 2.0f; // slight padding
    
    for (int y = 0; y < logo.height(); ++y) {
        for (int x = 0; x < logo.width(); ++x) {
            float dx = x - cx;
            float dy = y - cy;
            if (dx*dx + dy*dy > radius*radius) {
                logo.setPixelColor(x, y, Qt::transparent);
            }
        }
    }
    
    app.setWindowIcon(QIcon(QPixmap::fromImage(logo)));

    // Initialize libgit2
    if (!GitManager::initialize()) {
        qCritical("Failed to initialize libgit2");
        return 1;
    }

    MainWindow w;
    w.show();

    int ret = app.exec();

    // Shutdown libgit2
    GitManager::shutdown();

    return ret;
}
