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
    app.setQuitOnLastWindowClosed(false);

    // Load the pre-rendered squircle (rounded rectangle) icon directly
    app.setWindowIcon(QIcon(":/resources/app_icon.ico"));

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
