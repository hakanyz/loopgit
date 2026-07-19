#include "mainwindow.h"
#include "repowidget.h"
#include "gitmanager.h"

#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QSettings>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDialog>
#include "settingsdialog.h"
#include "reflogdialog.h"
#include <QScrollArea>
#include <QPainter>
#include <QTime>
#include <QFrame>
#include <QCheckBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QProcess>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressDialog>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    applyDarkTheme();
    resize(1440, 900);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    
    QWidget *centralContainer = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(centralContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_tabWidget);
    
    setCentralWidget(centralContainer);

    setupWelcomeScreen();
    setupMenuBar();
    setupToolBar();

    statusBar()->showMessage("Ready");

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *w = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        w->deleteLater();
        if (m_tabWidget->count() == 0) {
            m_welcomeWidget->show();
            m_tabWidget->hide();
            m_toolBar->setEnabled(false);
            setWindowTitle("LoopGit");
        }
    });

    m_trayIcon = new QSystemTrayIcon(qApp->windowIcon(), this);
    m_trayMenu = new QMenu(this);
    
    QAction *actShow = m_trayMenu->addAction("Show LoopGit");
    connect(actShow, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
    });
    
    m_trayMenu->addSeparator();
    
    QAction *actQuit = m_trayMenu->addAction("Quit LoopGit");
    connect(actQuit, &QAction::triggered, this, [this]() {
        m_reallyQuit = true;
        qApp->quit();
    });
    
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });

    m_netManager = new QNetworkAccessManager(this);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::onTrayMessageClicked);

    QTimer::singleShot(2000, this, [this]() {
        checkForUpdates(true);
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    m_tabWidget->hide();
    m_welcomeWidget->show();
    m_welcomeWidget->setParent(centralContainer);
    containerLayout->addWidget(m_welcomeWidget);
}

void MainWindow::setupWelcomeScreen()
{
    m_welcomeWidget = new QWidget(this);
    QVBoxLayout *welcomeLayout = new QVBoxLayout(m_welcomeWidget);
    welcomeLayout->setAlignment(Qt::AlignCenter);

    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPix = QIcon(":/resources/icons/logo.svg").pixmap(400, 400);
    if (!logoPix.isNull()) {
        logoLabel->setPixmap(logoPix);
        logoLabel->setAlignment(Qt::AlignCenter);
        logoLabel->setStyleSheet("background: transparent; border: none;");
        welcomeLayout->addWidget(logoLabel);
    }

    welcomeLayout->addSpacing(30);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->setSpacing(20);

    QPushButton *btnOpen = new QPushButton("Open Repository", this);
    btnOpen->setFixedSize(160, 40);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::openRepository);

    QPushButton *btnClone = new QPushButton("Clone Repository", this);
    btnClone->setFixedSize(160, 40);
    connect(btnClone, &QPushButton::clicked, this, &MainWindow::cloneRepository);

    btnLayout->addWidget(btnOpen);
    btnLayout->addWidget(btnClone);

    welcomeLayout->addLayout(btnLayout);
    
    welcomeLayout->addSpacing(30);

    QSettings settings;
    QStringList recentRepos = settings.value("app/recent_repos").toStringList();
    
    if (!recentRepos.isEmpty()) {
        QLabel *recentLabel = new QLabel("Recent Repositories:");
        recentLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #CCCCCC;");
        recentLabel->setAlignment(Qt::AlignCenter);
        welcomeLayout->addWidget(recentLabel);
        
        QVBoxLayout *recentLayout = new QVBoxLayout;
        recentLayout->setAlignment(Qt::AlignCenter);
        recentLayout->setSpacing(5);
        
        for (const QString &repo : recentRepos) {
            QPushButton *btnRepo = new QPushButton(repo, this);
            btnRepo->setStyleSheet("text-align: left; padding: 8px; background-color: #252526; border: 1px solid #3C3C3C; border-radius: 4px;");
            btnRepo->setCursor(Qt::PointingHandCursor);
            btnRepo->setMinimumWidth(400);
            btnRepo->setMaximumWidth(600);
            connect(btnRepo, &QPushButton::clicked, this, [this, repo]() {
                openRepositoryPath(repo);
            });
            recentLayout->addWidget(btnRepo);
        }
        
        welcomeLayout->addLayout(recentLayout);
    }
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("File");
    m_actOpen = fileMenu->addAction("Open Repository...", this, &MainWindow::openRepository);
    m_actOpen->setShortcut(QKeySequence::Open);

    m_actClone = fileMenu->addAction("Clone Repository...", this, &MainWindow::cloneRepository);

    fileMenu->addSeparator();
    m_actClose = fileMenu->addAction("Close Current Tab", this, &MainWindow::closeCurrentRepository);
    m_actClose->setShortcut(QKeySequence::Close);

    fileMenu->addSeparator();
    m_actCredentials = fileMenu->addAction("Credentials...", this, &MainWindow::openCredentials);
    
    QAction *actSettings = fileMenu->addAction("Settings...", this, [this]() {
        if (auto rw = currentRepoWidget()) {
            SettingsDialog dlg(rw->gitManager(), this);
            dlg.exec();
            rw->refreshAll();
        } else {
            QMessageBox::information(this, "Settings", "Please open a repository to manage its settings.");
        }
    });

    QAction *actReflog = fileMenu->addAction("Show Reflog...", this, [this]() {
        if (auto rw = currentRepoWidget()) {
            ReflogDialog dlg(rw->gitManager(), this);
            dlg.exec();
        } else {
            QMessageBox::information(this, "Reflog", "Please open a repository to view its reflog.");
        }
    });
    
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);

    QMenu *menuHelp = menuBar()->addMenu("Help");
    QAction *actShortcuts = menuHelp->addAction("Keyboard Shortcuts...");
    connect(actShortcuts, &QAction::triggered, this, &MainWindow::showShortcutsDialog);

    QAction *actUpdate = menuHelp->addAction("Check for Updates...");
    connect(actUpdate, &QAction::triggered, this, [this]() {
        checkForUpdates(false);
    });

    menuHelp->addSeparator();
    menuHelp->addAction("About LoopGit", this, &MainWindow::showAboutDialog);
}

void MainWindow::setupToolBar()
{
    m_toolBar = new QToolBar(this);
    m_toolBar->setMovable(false);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_toolBar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, m_toolBar);

    m_actLocalFiles = new QAction(QIcon(":/resources/icons/local_files.svg"), "Local Files", this);
    m_actLocalFiles->setCheckable(true);
    m_actLocalFiles->setChecked(true);
    m_toolBar->addAction(m_actLocalFiles);

    m_actHistory = new QAction(QIcon(":/resources/icons/history.svg"), "History", this);
    m_actHistory->setCheckable(true);
    m_toolBar->addAction(m_actHistory);

    // ── Group 1: Views ──
    m_toolBar->addSeparator();

    // ── Gap between Views and Sync ──
    QWidget *gap1 = new QWidget(this);
    gap1->setFixedWidth(24);
    gap1->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(gap1);

    // ── Group 2: Sync Operations ──
    m_actFetch = new QAction(QIcon(":/resources/icons/fetch.svg"), "Fetch", this);
    m_actFetch->setShortcut(QKeySequence("Ctrl+F"));
    m_actFetch->setToolTip("Fetch from remote (Ctrl+F)");
    m_toolBar->addAction(m_actFetch);

    m_actPull = new QAction(QIcon(":/resources/icons/pull.svg"), "Pull", this);
    m_actPull->setShortcut(QKeySequence("Ctrl+Shift+P"));
    m_actPull->setToolTip("Pull from remote (Ctrl+Shift+P)");
    m_toolBar->addAction(m_actPull);

    m_actPush = new QAction(QIcon(":/resources/icons/push.svg"), "Push", this);
    m_actPush->setShortcut(QKeySequence("Ctrl+P"));
    m_actPush->setToolTip("Push to remote (Ctrl+P)");
    QMenu *pushMenu = new QMenu(this);
    QAction *actForcePush = pushMenu->addAction("Force Push (with lease)");
    connect(actForcePush, &QAction::triggered, this, [this]() {
        if (auto rw = currentRepoWidget()) {
            if (QMessageBox::warning(this, "Force Push", "Are you sure you want to force push? This may overwrite remote history.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                rw->doPush(true);
            }
        }
    });
    m_actPush->setMenu(pushMenu);
    m_toolBar->addAction(m_actPush);

    m_actRefresh = new QAction(QIcon(":/resources/icons/refresh.svg"), "Refresh", this);
    m_actRefresh->setToolTip("Refresh repository status");
    m_toolBar->addAction(m_actRefresh);

    m_actTerminal = new QAction(QIcon(":/resources/icons/terminal.svg"), "Terminal", this);
    m_actTerminal->setShortcut(QKeySequence("Ctrl+T"));
    m_actTerminal->setToolTip("Open Terminal in repository directory (Ctrl+T)");
    m_toolBar->addAction(m_actTerminal);

    // ── Wide gap between Sync and Branch tools ──
    QWidget *gap2 = new QWidget(this);
    gap2->setFixedWidth(32);
    gap2->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(gap2);

    // ── Visible divider line ──
    QFrame *divider = new QFrame(this);
    divider->setFrameShape(QFrame::VLine);
    divider->setStyleSheet("color: #505050; background: transparent;");
    divider->setFixedHeight(40);
    m_toolBar->addWidget(divider);

    QWidget *gap3 = new QWidget(this);
    gap3->setFixedWidth(32);
    gap3->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(gap3);

    // ── Group 3: Branch / GitFlow Operations ──
    m_actFeature = new QAction(QIcon(":/resources/icons/feature.svg"), "Feature", this);
    m_toolBar->addAction(m_actFeature);

    m_actBugfix = new QAction(QIcon(":/resources/icons/bugfix.svg"), "Bugfix", this);
    m_toolBar->addAction(m_actBugfix);

    m_actRelease = new QAction(QIcon(":/resources/icons/release.svg"), "Release", this);
    m_toolBar->addAction(m_actRelease);

    m_actHotfix = new QAction(QIcon(":/resources/icons/hotfix.svg"), "Hotfix", this);
    m_toolBar->addAction(m_actHotfix);

    m_actFinish = new QAction(QIcon(":/resources/icons/finish.svg"), "Finish", this);
    m_toolBar->addAction(m_actFinish);



    // ── Spacer pushes status label to far right ──
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(spacer);

    m_syncStatusLabel = new QLabel(this);
    m_syncStatusLabel->setStyleSheet("background: transparent; color: #7A7A7A; padding-right: 16px; font-weight: bold;");
    m_toolBar->addWidget(m_syncStatusLabel);

    connect(m_actLocalFiles, &QAction::triggered, this, [this]() {
        m_actHistory->setChecked(false);
        m_actLocalFiles->setChecked(true);
        if (auto rw = currentRepoWidget()) rw->switchPerspective(0);
    });

    connect(m_actHistory, &QAction::triggered, this, [this]() {
        m_actLocalFiles->setChecked(false);
        m_actHistory->setChecked(true);
        if (auto rw = currentRepoWidget()) rw->switchPerspective(1);
    });

    connect(m_actRefresh, &QAction::triggered, this, [this]() { if (auto rw = currentRepoWidget()) rw->refreshAll(); });
    connect(m_actFetch, &QAction::triggered, this, [this]() { if (auto rw = currentRepoWidget()) rw->doFetch(); });
    connect(m_actPull, &QAction::triggered, this, [this]() { if (auto rw = currentRepoWidget()) rw->doPull(); });
    connect(m_actPush, &QAction::triggered, this, [this]() { if (auto rw = currentRepoWidget()) rw->doPush(); });

    connect(m_actTerminal, &QAction::triggered, this, [this]() {
        if (auto rw = currentRepoWidget()) {
            QString workingDir = rw->repoPath();
            if (workingDir.isEmpty()) return;
#ifdef Q_OS_WIN
            QString gitBashPath1 = "C:\\Program Files\\Git\\git-bash.exe";
            QString gitBashPath2 = "C:\\Program Files (x86)\\Git\\git-bash.exe";
            if (QFile::exists(gitBashPath1)) {
                QProcess::startDetached(gitBashPath1, QStringList() << "--cd=" + workingDir);
            } else if (QFile::exists(gitBashPath2)) {
                QProcess::startDetached(gitBashPath2, QStringList() << "--cd=" + workingDir);
            } else {
                QProcess::startDetached("cmd.exe", QStringList() << "/c" << "start" << "powershell.exe", workingDir);
            }
#elif defined(Q_OS_MAC)
            QStringList args;
            args << "-e" << QString("tell application \"Terminal\" to do script \"cd '%1'\"").arg(workingDir)
                 << "-e" << "tell application \"Terminal\" to activate";
            QProcess::startDetached("osascript", args);
#else
            QProcess::startDetached("x-terminal-emulator", QStringList(), workingDir);
#endif
        }
    });

    connect(m_actFeature, &QAction::triggered, this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("feature/"); });
    connect(m_actBugfix, &QAction::triggered, this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("bugfix/"); });
    connect(m_actRelease, &QAction::triggered, this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("release/"); });
    connect(m_actHotfix, &QAction::triggered, this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("hotfix/"); });
    connect(m_actFinish, &QAction::triggered, this, [this]() { if(auto rw = currentRepoWidget()) rw->finishGitFlowBranch(); });

    m_toolBar->setEnabled(false);
}

void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #1E1E1E;
            color: #D4D4D4;
        }
        QMenuBar {
            background-color: #2D2D2D;
            color: #D4D4D4;
            border-bottom: 1px solid #3C3C3C;
        }
        QMenuBar::item:selected {
            background-color: #094771;
        }
        QMenu {
            background-color: #2D2D2D;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
        }
        QMenu::item:selected {
            background-color: #094771;
        }
        QToolBar {
            background-color: #252526;
            border-bottom: 1px solid #3C3C3C;
            spacing: 4px;
            padding: 2px;
        }
        QToolButton {
            background-color: transparent;
            color: #D4D4D4;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 13px;
        }
        QToolButton:hover {
            background-color: #3C3C3C;
            border: 1px solid #505050;
        }
        QToolButton:checked {
            background-color: #094771;
            border: 1px solid #007ACC;
        }
        QToolButton:pressed {
            background-color: #0E639C;
        }
        QTabWidget::pane {
            border: 1px solid #3C3C3C;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #2D2D2D;
            color: #808080;
            border: 1px solid #3C3C3C;
            border-bottom-color: #3C3C3C;
            padding: 6px 12px;
        }
        QTabBar::tab:selected {
            background-color: #1E1E1E;
            color: #D4D4D4;
            border-bottom-color: #1E1E1E;
        }
        QTabBar::tab:hover {
            background-color: #3C3C3C;
        }
        QPushButton {
            background-color: #0E639C;
            color: #FFFFFF;
            border: none;
            border-radius: 3px;
            padding: 6px 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1177BB;
        }
        QPushButton:pressed {
            background-color: #094771;
        }
        QPushButton:disabled {
            background-color: #3C3C3C;
            color: #808080;
        }
        QTreeWidget, QTreeView {
            background-color: #252526;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
            alternate-background-color: #2A2A2A;
        }
        QTreeWidget::item:selected, QTreeView::item:selected {
            background-color: #094771;
        }
        QTreeWidget::item:hover, QTreeView::item:hover {
            background-color: #2A2D2E;
        }
        QHeaderView::section {
            background-color: #2D2D2D;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
            padding: 4px;
        }
        QTableView {
            background-color: #252526;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
            alternate-background-color: #2A2A2A;
            gridline-color: #3C3C3C;
        }
        QTableView::item:selected {
            background-color: #094771;
        }
        QTextEdit {
            background-color: #1E1E1E;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
            border-radius: 3px;
            padding: 4px;
        }
        QComboBox {
            background-color: transparent;
            color: #D4D4D4;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox:hover {
            border: 1px solid #007ACC;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #2D2D2D;
            color: #D4D4D4;
            selection-background-color: #094771;
            border: 1px solid #3C3C3C;
        }
        QStatusBar {
            background-color: #2D2D2D;
            color: #D4D4D4;
            border-top: 1px solid #3C3C3C;
        }
        QLabel {
            color: #D4D4D4;
        }
        QSplitter::handle {
            background-color: #3C3C3C;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
    )"));
}

RepoWidget* MainWindow::currentRepoWidget() const
{
    if (m_tabWidget->count() == 0) return nullptr;
    return qobject_cast<RepoWidget*>(m_tabWidget->currentWidget());
}

void MainWindow::openRepository()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Git Repository");
    if (!dir.isEmpty()) {
        openRepositoryPath(dir);
    }
}

void MainWindow::openRepositoryPath(const QString &path)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        RepoWidget *rw = qobject_cast<RepoWidget*>(m_tabWidget->widget(i));
        if (rw && rw->repoPath() == path) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    QSettings settings;
    QStringList recentRepos = settings.value("app/recent_repos").toStringList();
    recentRepos.removeAll(path);
    recentRepos.prepend(path);
    if (recentRepos.size() > 10) {
        recentRepos = recentRepos.mid(0, 10);
    }
    settings.setValue("app/recent_repos", recentRepos);

    RepoWidget *rw = new RepoWidget(path, this);
    connect(rw, &RepoWidget::statusMessage, this, [this](const QString &msg) {
        statusBar()->showMessage(msg, 5000);
    });
    connect(rw, &RepoWidget::errorOccurred, this, &MainWindow::showError);
    connect(rw, &RepoWidget::branchListChanged, this, [this](const QStringList &branches, const QString &currentBranch) {
        Q_UNUSED(branches);
        if (currentRepoWidget() == sender()) {
            m_syncStatusLabel->setText(QString("Branch: %1").arg(currentBranch));
        }
    });

    QString tabName = QFileInfo(path).fileName();
    if (tabName.isEmpty()) tabName = path;
    
    int index = m_tabWidget->addTab(rw, tabName);
    m_tabWidget->setCurrentIndex(index);
    
    m_welcomeWidget->hide();
    m_tabWidget->show();
    m_toolBar->setEnabled(true);
    m_syncStatusLabel->setVisible(true);
}

void MainWindow::closeCurrentRepository()
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0) {
        QWidget *w = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        w->deleteLater();
        if (m_tabWidget->count() == 0) {
            m_welcomeWidget->show();
            m_tabWidget->hide();
            m_toolBar->setEnabled(false);
            m_syncStatusLabel->setVisible(false);
            setWindowTitle("LoopGit");
        }
    }
}

void MainWindow::onTabChanged(int index)
{
    if (index < 0) return;
    
    RepoWidget *rw = qobject_cast<RepoWidget*>(m_tabWidget->widget(index));
    if (rw) {
        setWindowTitle(QString("LoopGit - %1").arg(rw->repoPath()));
    }
}

void MainWindow::cloneRepository()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Clone Repository");
    QFormLayout *layout = new QFormLayout(&dialog);

    QLineEdit *urlEdit = new QLineEdit(&dialog);
    urlEdit->setPlaceholderText("https://github.com/user/repo.git");
    
    QWidget *pathWidget = new QWidget(&dialog);
    QHBoxLayout *pathLayout = new QHBoxLayout(pathWidget);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    QLineEdit *pathEdit = new QLineEdit(pathWidget);
    QPushButton *browseBtn = new QPushButton("Browse...", pathWidget);
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseBtn);
    
    connect(browseBtn, &QPushButton::clicked, [&](){
        QString dir = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!dir.isEmpty()) pathEdit->setText(dir);
    });

    layout->addRow("Repository URL:", urlEdit);
    layout->addRow("Destination:", pathWidget);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString url = urlEdit->text().trimmed();
        QString dest = pathEdit->text().trimmed();

        if (url.isEmpty() || dest.isEmpty()) return;

        QSettings settings("MyCompany", "LoopGit");
        
        GitManager tempGit;
        tempGit.setCredentials(settings.value("github/username", "").toString(), settings.value("github/token", "").toString());
        
        statusBar()->showMessage("Cloning repository...");
        QApplication::processEvents();

        if (tempGit.cloneRepository(url, dest)) {
            statusBar()->showMessage("Clone successful", 3000);
            openRepositoryPath(dest);
        } else {
            QMessageBox::critical(this, "Clone Failed", tempGit.lastError());
            statusBar()->showMessage("Clone failed", 3000);
        }
    }
}

void MainWindow::updateBranchCombo()
{
    // Triggered externally
}



void MainWindow::openCredentials()
{
    QDialog dialog(this);
    dialog.setWindowTitle("GitHub Credentials");
    QFormLayout *layout = new QFormLayout(&dialog);

    QSettings settings("MyCompany", "LoopGit");

    QLineEdit *userEdit = new QLineEdit(&dialog);
    userEdit->setText(settings.value("github/username", "").toString());
    QLineEdit *tokenEdit = new QLineEdit(&dialog);
    tokenEdit->setEchoMode(QLineEdit::Password);
    tokenEdit->setText(settings.value("github/token", "").toString());

    layout->addRow("Username:", userEdit);
    layout->addRow("Personal Access Token:", tokenEdit);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        settings.setValue("github/username", userEdit->text().trimmed());
        settings.setValue("github/token", tokenEdit->text().trimmed());
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About LoopGit");
    aboutBox.setIconPixmap(windowIcon().pixmap(64, 64));
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(
        "<h2 style='margin-bottom:2px;'>LoopGit</h2>"
        "<p style='color:#888; margin-top:0;'>Version " + qApp->applicationVersion() + "</p>"
        "<p>A fast, modern Git GUI client built with <b>C++ / Qt6</b> and <b>libgit2</b>.</p>"
        "<p>Built with ❤️ for developers who love speed and simplicity.</p>"
        "<hr>"
        "<p><b>Author:</b> Hakan</p>"
        "<p><a href='https://github.com/hakanyz/loopgit'>github.com/hakanyz/loopgit</a></p>"
    );
    aboutBox.exec();
}

void MainWindow::showShortcutsDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Keyboard Shortcuts");
    dlg.resize(480, 400);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel("<h3>LoopGit Keyboard Shortcuts</h3>");
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget;
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Shortcut", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet("QTableWidget { background-color: #1E1E1E; color: #CCCCCC; }"
                         "QHeaderView::section { background: #2D2D2D; color: #E0E0E0; padding: 6px; }");

    QVector<QPair<QString, QString>> shortcuts = {
        {"Ctrl+O",         "Open Repository"},
        {"Ctrl+W",         "Close Current Tab"},
        {"Ctrl+F",         "Fetch from Remote"},
        {"Ctrl+P",         "Push to Remote"},
        {"Ctrl+Shift+P",   "Pull from Remote"},
        {"Ctrl+Shift+C",   "Focus Commit Message Box"},
        {"Ctrl+Return",    "Commit (when message box is focused)"},
    };

    table->setRowCount(shortcuts.size());
    for (int i = 0; i < shortcuts.size(); ++i) {
        QTableWidgetItem *keyItem = new QTableWidgetItem(shortcuts[i].first);
        keyItem->setFont(QFont("Consolas", 10));
        keyItem->setForeground(QColor("#569CD6"));
        table->setItem(i, 0, keyItem);
        table->setItem(i, 1, new QTableWidgetItem(shortcuts[i].second));
    }
    table->resizeColumnsToContents();
    table->setColumnWidth(0, 160);

    layout->addWidget(table);

    QPushButton *btnClose = new QPushButton("Close");
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    dlg.exec();
}

void MainWindow::showStatusMessage(const QString &msg)
{
    statusBar()->showMessage(msg, 5000);
}

void MainWindow::showError(const QString &msg)
{
    QMessageBox::critical(this, "Error", msg);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_reallyQuit || !m_trayIcon || !m_trayIcon->isVisible()) {
        event->accept();
        qApp->quit();
        return;
    }

    QSettings settings;
    QString closeAction = settings.value("app/closeAction", "").toString();
    
    if (closeAction == "tray") {
        hide();
        event->ignore();
        return;
    } else if (closeAction == "quit") {
        event->accept();
        qApp->quit();
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("LoopGit");
    msgBox.setText("What do you want to do?");
    msgBox.setInformativeText("You can minimize the application to the system tray or exit completely.");
    
    QPushButton *btnTray = msgBox.addButton("Minimize to Tray", QMessageBox::AcceptRole);
    QPushButton *btnQuit = msgBox.addButton("Exit Application", QMessageBox::RejectRole);
    
    QCheckBox *cbRemember = new QCheckBox("Don't ask again");
    msgBox.setCheckBox(cbRemember);

    msgBox.exec();

    if (msgBox.clickedButton() == btnTray) {
        if (cbRemember->isChecked()) {
            settings.setValue("app/closeAction", "tray");
        }
        hide();
        event->ignore();
    } else {
        if (cbRemember->isChecked()) {
            settings.setValue("app/closeAction", "quit");
        }
        event->accept();
        qApp->quit();
    }
}

void MainWindow::checkForUpdates(bool silent)
{
    QNetworkRequest request(QUrl("https://api.github.com/repos/hakanyz/loopgit/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "LoopGit");
    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
        onUpdateCheckFinished(reply, silent);
    });
}

void MainWindow::onUpdateCheckFinished(QNetworkReply *reply, bool silent)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (!silent) {
            QMessageBox::warning(this, "Update Check Failed", "Failed to check for updates:\n" + reply->errorString());
        }
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    
    QString latestVersion = obj["tag_name"].toString();
    if (latestVersion.startsWith("v")) latestVersion = latestVersion.mid(1);
    
    QString currentVersion = qApp->applicationVersion();
    if (currentVersion.endsWith("-beta")) currentVersion.replace("-beta", "");
    
    QJsonArray assets = obj["assets"].toArray();
    QString downloadUrl;
    for (const QJsonValue &val : assets) {
        QJsonObject asset = val.toObject();
        if (asset["name"].toString().endsWith(".exe")) {
            downloadUrl = asset["browser_download_url"].toString();
            break;
        }
    }

    if (latestVersion > currentVersion && !downloadUrl.isEmpty()) {
        m_latestUpdateVersion = latestVersion;
        m_latestUpdateUrl = downloadUrl;
        
        if (silent) {
            m_trayIcon->showMessage("LoopGit Update Available", "Version " + latestVersion + " is available! Click here to download and install.", QSystemTrayIcon::Information, 10000);
        } else {
            if (QMessageBox::question(this, "Update Available", "LoopGit version " + latestVersion + " is available!\nDo you want to download and install it now?") == QMessageBox::Yes) {
                startUpdateDownload();
            }
        }
    } else {
        if (!silent) {
            QMessageBox::information(this, "Up to Date", "You are using the latest version of LoopGit.");
        }
    }
}

void MainWindow::onTrayMessageClicked()
{
    if (!m_latestUpdateUrl.isEmpty()) {
        if (QMessageBox::question(this, "Update Available", "LoopGit version " + m_latestUpdateVersion + " is available!\nDo you want to download and install it now?") == QMessageBox::Yes) {
            startUpdateDownload();
        }
    }
}

void MainWindow::startUpdateDownload()
{
    QProgressDialog *progressDialog = new QProgressDialog("Downloading update...", "Cancel", 0, 100, this);
    progressDialog->setWindowTitle("Update");
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);

    QUrl url(m_latestUpdateUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LoopGit");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_netManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, progressDialog, [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            progressDialog->setMaximum(bytesTotal);
            progressDialog->setValue(bytesReceived);
        }
    });

    connect(progressDialog, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/LoopGit_Update_Setup.exe";

    connect(reply, &QNetworkReply::finished, this, [this, reply, tempPath, progressDialog]() {
        progressDialog->deleteLater();
        onDownloadFinished(reply, tempPath);
    });
}

void MainWindow::onDownloadFinished(QNetworkReply *reply, const QString &downloadPath)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            QMessageBox::warning(this, "Download Failed", "Failed to download update:\n" + reply->errorString());
        }
        return;
    }

    QFile file(downloadPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();
        
        QMessageBox::information(this, "Download Complete", "The update has been downloaded. LoopGit will now restart to install the update.");
        
        QProcess::startDetached(downloadPath, QStringList() << "/SILENT");
        m_reallyQuit = true;
        qApp->quit();
    } else {
        QMessageBox::warning(this, "Error", "Could not save the update file to " + downloadPath);
    }
}

