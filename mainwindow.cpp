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
#include <QPainter>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    applyDarkTheme();
    resize(1200, 800);
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
            m_branchCombo->setVisible(false);
            setWindowTitle("LoopGit");
        }
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
    QPixmap logoPix(":/resources/logo.png");
    if (!logoPix.isNull()) {
        logoLabel->setPixmap(logoPix.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", this, &MainWindow::showAboutDialog);
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

    m_toolBar->addSeparator();

    QWidget *actionGap = new QWidget(this);
    actionGap->setFixedWidth(16);
    actionGap->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(actionGap);

    m_actFetch = new QAction(QIcon(":/resources/icons/refresh.svg"), "Fetch", this);
    m_toolBar->addAction(m_actFetch);

    m_actPull = new QAction(QIcon(":/resources/icons/pull.svg"), "Pull", this);
    m_toolBar->addAction(m_actPull);

    m_actPush = new QAction(QIcon(":/resources/icons/push.svg"), "Push", this);
    m_toolBar->addAction(m_actPush);

    m_toolBar->addSeparator();

    m_actRefresh = new QAction(QIcon(":/resources/icons/refresh.svg"), "Refresh", this);
    m_toolBar->addAction(m_actRefresh);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;");
    m_toolBar->addWidget(spacer);

    m_syncStatusLabel = new QLabel(this);
    m_syncStatusLabel->setStyleSheet("color: #7A7A7A; padding-right: 16px; font-weight: bold;");
    m_toolBar->addWidget(m_syncStatusLabel);

    QToolButton *gitFlowBtn = new QToolButton(this);
    gitFlowBtn->setIcon(QIcon(":/resources/icons/branch.svg"));
    gitFlowBtn->setText("GitFlow");
    gitFlowBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    gitFlowBtn->setPopupMode(QToolButton::InstantPopup);
    QMenu *gitFlowMenu = new QMenu(gitFlowBtn);
    gitFlowMenu->addAction("Feature...", this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("feature/"); });
    gitFlowMenu->addAction("Bugfix...", this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("bugfix/"); });
    gitFlowMenu->addAction("Release...", this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("release/"); });
    gitFlowMenu->addAction("Hotfix...", this, [this]() { if(auto rw = currentRepoWidget()) rw->startGitFlowBranch("hotfix/"); });
    gitFlowBtn->setMenu(gitFlowMenu);
    m_toolBar->addWidget(gitFlowBtn);

    m_branchCombo = new QComboBox(this);
    m_branchCombo->setMinimumWidth(120);
    m_toolBar->addWidget(m_branchCombo);
    m_branchCombo->setVisible(false);

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

    connect(m_branchCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onBranchComboActivated);

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
            background-color: #007ACC;
            color: #FFFFFF;
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

    RepoWidget *rw = new RepoWidget(path, this);
    connect(rw, &RepoWidget::statusMessage, this, [this](const QString &msg) {
        if (currentRepoWidget() == sender()) {
            m_syncStatusLabel->setText(msg);
        }
    });
    connect(rw, &RepoWidget::errorOccurred, this, &MainWindow::showError);
    connect(rw, &RepoWidget::branchListChanged, this, [this](const QStringList &branches, const QString &currentBranch) {
        if (currentRepoWidget() == sender()) {
            m_updatingBranch = true;
            m_branchCombo->clear();
            m_branchCombo->addItems(branches);
            m_branchCombo->setCurrentText(currentBranch);
            m_updatingBranch = false;
        }
    });

    QString tabName = QFileInfo(path).fileName();
    if (tabName.isEmpty()) tabName = path;
    
    int index = m_tabWidget->addTab(rw, tabName);
    m_tabWidget->setCurrentIndex(index);
    
    m_welcomeWidget->hide();
    m_tabWidget->show();
    m_toolBar->setEnabled(true);
    m_branchCombo->setVisible(true);
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
            m_branchCombo->setVisible(false);
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
        connect(rw, &RepoWidget::statusMessage, this, [this, rw](const QString &msg) {
            if (currentRepoWidget() == rw) {
                m_syncStatusLabel->setText(msg);
            }
        });
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

void MainWindow::onBranchComboActivated(int index)
{
    if (m_updatingBranch) return;
    QString branchName = m_branchCombo->itemText(index);
    if (branchName.isEmpty()) return;

    if (RepoWidget *rw = currentRepoWidget()) {
        // Since git() is not accessible easily without making it public or adding a wrapper,
        // we added checkoutBranch/fetch to RepoWidget directly if needed.
        // For now, this is a placeholder.
    }
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
    QMessageBox::about(this, "About LoopGit",
        "<h3>LoopGit v0.1</h3>"
        "<p>A modern Git GUI client built with Qt and libgit2.</p>");
}

void MainWindow::showStatusMessage(const QString &msg)
{
    statusBar()->showMessage(msg, 5000);
}

void MainWindow::showError(const QString &msg)
{
    QMessageBox::critical(this, "Error", msg);
}
