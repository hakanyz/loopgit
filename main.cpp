#include "mainwindow.h"
#include "gitmanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QtGitClient"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("QtGitClient"));

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
