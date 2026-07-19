#include "mainwindow.h"
#include "gitmanager.h"
#include "diffviewwidget.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTableView>
#include <QMenu>
#include <QHeaderView>
#include <QStackedWidget>
#include <QFileSystemModel>
#include <QActionGroup>
#include "commitgraphmodel.h"
#include "commitgraphdelegate.h"
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QDir>
#include <QTimer>

// ═══════════════════════════════════════════════════════════════════
//  Construction / Destruction
// ═══════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_git(new GitManager(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_logModel(new CommitGraphModel(this))
    , m_logDelegate(new CommitGraphDelegate(this))
{
    setupUi();
    connectSignals();
    applyDarkTheme();
    setRepoActionsEnabled(false);
}

MainWindow::~MainWindow() = default;

// ═══════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("GitZen"));
    resize(1280, 800);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
}

#include <QSettings>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QProgressDialog>
#include <QDateTime>

// ─── Menu Bar ──────────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();

    // File
    QMenu *fileMenu = mb->addMenu(QStringLiteral("&File"));
    m_actOpen  = fileMenu->addAction(QStringLiteral("📂 &Open Repository..."),
                                     QKeySequence::Open,
                                     this, &MainWindow::openRepository);
    m_actClone = fileMenu->addAction(QStringLiteral("🌐 &Clone Repository..."),
                                     this, &MainWindow::cloneRepository);
    m_actClose = fileMenu->addAction(QStringLiteral("Close Repository"),
                                     this, &MainWindow::closeRepository);
    fileMenu->addSeparator();
    m_actCredentials = fileMenu->addAction(QStringLiteral("🔑 GitHub Credentials..."),
                                           this, &MainWindow::openCredentials);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"),
                        QKeySequence::Quit,
                        this, &QWidget::close);

    // Repository
    QMenu *repoMenu = mb->addMenu(QStringLiteral("&Repository"));
    m_actRefresh    = repoMenu->addAction(QStringLiteral("🔄 &Refresh"),
                                          QKeySequence::Refresh,
                                          this, &MainWindow::refreshAll);
    repoMenu->addSeparator();
    m_actStageAll   = repoMenu->addAction(QStringLiteral("Stage All"),
                                          this, &MainWindow::stageAll);
    m_actUnstageAll = repoMenu->addAction(QStringLiteral("Unstage All"),
                                          this, &MainWindow::unstageAll);
    repoMenu->addSeparator();
    m_actFetch = repoMenu->addAction(QStringLiteral("🔄 Fe&tch"),
                                     this, &MainWindow::doFetch);
    m_actPull = repoMenu->addAction(QStringLiteral("⬇ Pu&ll"),
                                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L),
                                    this, &MainWindow::doPull);
    m_actPush = repoMenu->addAction(QStringLiteral("⬆ &Push"),
                                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P),
                                    this, &MainWindow::doPush);

    // Branch
    QMenu *branchMenu = mb->addMenu(QStringLiteral("&Branch"));
    m_actNewBranch    = branchMenu->addAction(QStringLiteral("New Branch..."),
                                              this, &MainWindow::createBranch);
    m_actDeleteBranch = branchMenu->addAction(QStringLiteral("Delete Branch..."),
                                              this, &MainWindow::deleteBranch);
    m_actMergeBranch  = branchMenu->addAction(QStringLiteral("Merge Branch..."),
                                              this, &MainWindow::mergeBranch);

    // Help
    QMenu *helpMenu = mb->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("About GitZen"), this, &MainWindow::showAboutDialog);
}

// ─── Tool Bar ──────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(QStringLiteral("Main"));
    m_toolBar->setMovable(false);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_toolBar->setStyleSheet(QStringLiteral(
        "QToolBar { padding: 5px; border-bottom: 1px solid #333; }"
        "QToolButton { margin: 0 2px; padding: 4px 8px; font-size: 13px; border-radius: 4px; }"
        "QToolButton:hover { background-color: #2D2D2D; }"
        "QComboBox { margin: 0 10px; padding: 4px; border: 1px solid #555; border-radius: 4px; min-width: 150px; }"
    ));

    m_actLocalFiles = new QAction(QStringLiteral("Local Files"), this);
    m_actLocalFiles->setCheckable(true);
    m_actLocalFiles->setChecked(true); // Default

    m_actHistory = new QAction(QStringLiteral("History"), this);
    m_actHistory->setCheckable(true);

    QActionGroup *perspectiveGroup = new QActionGroup(this);
    perspectiveGroup->addAction(m_actLocalFiles);
    perspectiveGroup->addAction(m_actHistory);
    perspectiveGroup->setExclusive(true);

    m_toolBar->addAction(m_actLocalFiles);
    m_toolBar->addAction(m_actHistory);
    m_toolBar->addSeparator();

    // Branch Control
    m_branchCombo = new QComboBox(this);
    m_toolBar->addWidget(m_branchCombo);
    
    connect(m_branchCombo, &QComboBox::textActivated, this, [this](const QString &branchName){
        if (branchName.isEmpty() || branchName == m_git->getCurrentBranch()) return;
        if (m_git->checkoutBranch(branchName)) {
            refreshAll();
        } else {
            QMessageBox::warning(this, "Checkout Failed", "Failed to checkout branch. Do you have uncommitted changes?");
            updateBranchCombo(); // Reset combo to current branch
        }
    });

    m_toolBar->addSeparator();

    m_actFetch->setText("Fetch");
    m_toolBar->addAction(m_actFetch);

    m_actPull->setText("Pull");
    m_toolBar->addAction(m_actPull);

    m_actPush->setText("Push");
    m_toolBar->addAction(m_actPush);
    m_toolBar->addSeparator();

    QAction *actStash = new QAction(QStringLiteral("Stash"), this);
    m_toolBar->addAction(actStash);
    
    QAction *actPop = new QAction(QStringLiteral("Pop Stash"), this);
    m_toolBar->addAction(actPop);
    m_toolBar->addSeparator();

    QAction *actCherry = new QAction(QStringLiteral("Cherry-Pick"), this);
    m_toolBar->addAction(actCherry);

    QAction *actRevertBtn = new QAction(QStringLiteral("Revert"), this);
    m_toolBar->addAction(actRevertBtn);
    m_toolBar->addSeparator();

    m_actRefresh->setText("Refresh");
    m_toolBar->addAction(m_actRefresh);

    // Push Sync Status Label to the right
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);
    
    // We already created m_syncStatusLabel in setupStatusBar, but let's re-parent it here
    // Wait, setupStatusBar is called AFTER setupToolBar. We should move its initialization here.
    // I will remove it from setupStatusBar in another chunk.
    m_syncStatusLabel = new QLabel(QStringLiteral("Sync: Offline"));
    m_syncStatusLabel->setStyleSheet(QStringLiteral("color: #888888; padding: 0 10px; background: transparent; border: none; font-weight: bold;"));
    m_toolBar->addWidget(m_syncStatusLabel);

    connect(actStash, &QAction::triggered, this, [this](){
        bool ok;
        QString msg = QInputDialog::getText(this, "Stash", "Stash message:", QLineEdit::Normal, "", &ok);
        if (ok) {
            if (m_git->stashSave(msg)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });

    connect(actPop, &QAction::triggered, this, [this](){
        if (QMessageBox::question(this, "Pop Stash", "Apply and drop the latest stash?") == QMessageBox::Yes) {
            if (m_git->stashPop()) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });

    connect(actCherry, &QAction::triggered, this, [this](){
        if (m_selectedCommitId.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Please select a commit to cherry-pick.");
            return;
        }
        if (QMessageBox::question(this, "Cherry-Pick", "Cherry-pick selected commit?") == QMessageBox::Yes) {
            if (m_git->cherryPick(m_selectedCommitId)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });

    connect(actRevertBtn, &QAction::triggered, this, [this](){
        if (m_selectedCommitId.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Please select a commit to revert.");
            return;
        }
        if (QMessageBox::question(this, "Revert", "Revert selected commit?") == QMessageBox::Yes) {
            if (m_git->revertCommit(m_selectedCommitId)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });

    connect(m_actLocalFiles, &QAction::triggered, this, [this](){ switchPerspective(0); });
    connect(m_actHistory, &QAction::triggered, this, [this](){ switchPerspective(1); });
}

// ─── Central Widget ────────────────────────────────────────────────

void MainWindow::setupCentralWidget()
{
    // ==========================================
    // 1. LOCAL FILES PERSPECTIVE (Index 0)
    // ==========================================
    m_dirModel = new QFileSystemModel(this);
    m_dirTree = new QTreeView;
    m_dirTree->setModel(m_dirModel);
    m_dirTree->setColumnHidden(1, true); // size
    m_dirTree->setColumnHidden(2, true); // type
    m_dirTree->setColumnHidden(3, true); // date
    m_dirTree->setHeaderHidden(true);
    m_dirTree->setAlternatingRowColors(true);

    m_localChangesTree = new QTreeWidget;
    m_localChangesTree->setHeaderLabels({QStringLiteral("Status"), QStringLiteral("File")});
    m_localChangesTree->setColumnWidth(0, 80);
    m_localChangesTree->setRootIsDecorated(true);
    m_localChangesTree->setAlternatingRowColors(true);
    m_localChangesTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_stagedRoot = new QTreeWidgetItem(m_localChangesTree, {QStringLiteral(""), QStringLiteral("Staged Changes")});
    m_stagedRoot->setExpanded(true);
    m_stagedRoot->setFlags(Qt::ItemIsEnabled);

    m_unstagedRoot = new QTreeWidgetItem(m_localChangesTree, {QStringLiteral(""), QStringLiteral("Unstaged Changes")});
    m_unstagedRoot->setExpanded(true);
    m_unstagedRoot->setFlags(Qt::ItemIsEnabled);

    // Commit Panel
    QPushButton *stageBtn   = new QPushButton(QStringLiteral("▲ Stage"));
    QPushButton *unstageBtn = new QPushButton(QStringLiteral("▼ Unstage"));
    QPushButton *stageAllBtn   = new QPushButton(QStringLiteral("▲▲ Stage All"));
    QPushButton *unstageAllBtn = new QPushButton(QStringLiteral("▼▼ Unstage All"));

    connect(stageBtn,      &QPushButton::clicked, this, &MainWindow::stageSelected);
    connect(unstageBtn,    &QPushButton::clicked, this, &MainWindow::unstageSelected);
    connect(stageAllBtn,   &QPushButton::clicked, this, &MainWindow::stageAll);
    connect(unstageAllBtn, &QPushButton::clicked, this, &MainWindow::unstageAll);

    QHBoxLayout *stageBtnLayout = new QHBoxLayout;
    stageBtnLayout->setContentsMargins(0, 0, 0, 0);
    stageBtnLayout->addWidget(stageBtn);
    stageBtnLayout->addWidget(unstageBtn);
    stageBtnLayout->addWidget(stageAllBtn);
    stageBtnLayout->addWidget(unstageAllBtn);

    QLabel *commitLabel = new QLabel(QStringLiteral("Commit Message:"));
    m_commitEdit = new QTextEdit;
    m_commitEdit->setPlaceholderText(QStringLiteral("Enter commit message..."));
    m_commitBtn = new QPushButton(QStringLiteral("✓ Commit"));
    m_commitBtn->setMinimumHeight(32);

    QVBoxLayout *commitPanelLayout = new QVBoxLayout;
    commitPanelLayout->setContentsMargins(4, 4, 4, 4);
    commitPanelLayout->setSpacing(4);
    commitPanelLayout->addLayout(stageBtnLayout);
    commitPanelLayout->addWidget(commitLabel);
    commitPanelLayout->addWidget(m_commitEdit, 1);
    commitPanelLayout->addWidget(m_commitBtn);

    m_commitPanelWidget = new QWidget;
    m_commitPanelWidget->setLayout(commitPanelLayout);

    QSplitter *localTopSplitter = new QSplitter(Qt::Horizontal);
    localTopSplitter->addWidget(m_dirTree);
    localTopSplitter->addWidget(m_localChangesTree);
    localTopSplitter->addWidget(m_commitPanelWidget);
    localTopSplitter->setStretchFactor(0, 2);
    localTopSplitter->setStretchFactor(1, 4);
    localTopSplitter->setStretchFactor(2, 3);

    m_localDiffView = new DiffViewWidget;

    QSplitter *localMainSplitter = new QSplitter(Qt::Vertical);
    localMainSplitter->addWidget(localTopSplitter);
    localMainSplitter->addWidget(m_localDiffView);
    localMainSplitter->setStretchFactor(0, 6);
    localMainSplitter->setStretchFactor(1, 4);

    // ==========================================
    // 2. HISTORY PERSPECTIVE (Index 1)
    // ==========================================
    m_branchesTree = new QTreeWidget;
    m_branchesTree->setHeaderHidden(true);
    m_branchesTree->setRootIsDecorated(true);
    m_branchesTree->setAlternatingRowColors(true);

    m_logTable = new QTableView;
    m_logTable->setModel(m_logModel);
    m_logTable->setItemDelegate(m_logDelegate);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->setShowGrid(false);
    m_logTable->setFocusPolicy(Qt::NoFocus);
    m_logTable->setStyleSheet(
        "QTableView {"
        "   border: none;"
        "   selection-background-color: #062f4a;"
        "   selection-color: white;"
        "   outline: none;"
        "}"
        "QTableView::item {"
        "   border: none;"
        "   border-bottom: 1px solid transparent;"
        "}"
        "QTableView::item:selected {"
        "   background-color: #062f4a;"
        "}"
    );
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->verticalHeader()->setDefaultSectionSize(28);
    
    QHeaderView *hHeader = m_logTable->horizontalHeader();
    hHeader->setStretchLastSection(true);
    hHeader->setSectionResizeMode(CommitGraphModel::ColGraph, QHeaderView::Fixed);
    hHeader->setSectionResizeMode(CommitGraphModel::ColMessage, QHeaderView::Stretch);
    
    m_logTable->setColumnWidth(CommitGraphModel::ColGraph, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColHash, 80);
    m_logTable->setColumnWidth(CommitGraphModel::ColAuthor, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColDate, 140);

    m_historyFilesTree = new QTreeWidget;
    m_historyFilesTree->setHeaderLabels({QStringLiteral("Status"), QStringLiteral("Name"), QStringLiteral("Path")});
    m_historyFilesTree->setColumnWidth(0, 80);
    m_historyFilesTree->setColumnWidth(1, 150);
    m_historyFilesTree->setRootIsDecorated(false);
    m_historyFilesTree->setAlternatingRowColors(true);
    
    m_commitDetailsLabel = new QLabel;
    m_commitDetailsLabel->setWordWrap(true);
    m_commitDetailsLabel->setTextFormat(Qt::RichText);
    m_commitDetailsLabel->setMinimumHeight(60);
    m_commitDetailsLabel->setStyleSheet(QStringLiteral("padding: 8px; background-color: #252526; border-bottom: 1px solid #3C3C3C;"));
    m_commitDetailsLabel->setText(QStringLiteral("<span style='color:#808080;'>Select a commit to view details</span>"));

    QWidget *historyRightWidget = new QWidget;
    QVBoxLayout *hrLayout = new QVBoxLayout(historyRightWidget);
    hrLayout->setContentsMargins(0, 0, 0, 0);
    hrLayout->setSpacing(0);
    hrLayout->addWidget(m_commitDetailsLabel);
    hrLayout->addWidget(m_historyFilesTree);

    QSplitter *historyTopSplitter = new QSplitter(Qt::Horizontal);
    historyTopSplitter->addWidget(m_branchesTree);
    historyTopSplitter->addWidget(m_logTable);
    historyTopSplitter->addWidget(historyRightWidget);
    historyTopSplitter->setStretchFactor(0, 2);
    historyTopSplitter->setStretchFactor(1, 6);
    historyTopSplitter->setStretchFactor(2, 3);

    m_historyDiffView = new DiffViewWidget;

    QSplitter *historyMainSplitter = new QSplitter(Qt::Vertical);
    historyMainSplitter->addWidget(historyTopSplitter);
    historyMainSplitter->addWidget(m_historyDiffView);
    historyMainSplitter->setStretchFactor(0, 6);
    historyMainSplitter->setStretchFactor(1, 4);

    // ==========================================
    // 3. STACKED WIDGET (Main Container)
    // ==========================================
    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(localMainSplitter);   // Index 0: Local Files
    m_stackedWidget->addWidget(historyMainSplitter); // Index 1: History
    
    setCentralWidget(m_stackedWidget);
}

void MainWindow::switchPerspective(int index)
{
    if (m_stackedWidget) {
        m_stackedWidget->setCurrentIndex(index);
    }
}

// ─── Status Bar ────────────────────────────────────────────────────

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(QStringLiteral("No repository open"));
    m_statusLabel->setFrameStyle(QFrame::NoFrame);
    
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->setStyleSheet(QStringLiteral("QStatusBar::item { border: none; }"));
}

// ─── Connections ───────────────────────────────────────────────────

void MainWindow::connectSignals()
{
    m_localChangesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_localChangesTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showLocalFilesContextMenu);

    connect(m_localChangesTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onFileItemDoubleClicked);

    connect(m_localChangesTree, &QTreeWidget::itemClicked,
            this, [this](QTreeWidgetItem *item, int){
        if (!item || item == m_stagedRoot || item == m_unstagedRoot) {
            m_localDiffView->clearDiff();
            return;
        }
        QString path = item->data(0, Qt::UserRole).toString();
        
        static const QStringList binExts = {"pdf", "png", "jpg", "jpeg", "gif", "exe", "dll", "so", "bin", "zip", "tar", "gz", "mp4", "mp3", "obj", "o", "a", "lib", "ttf", "woff"};
        if (binExts.contains(QFileInfo(path).suffix().toLower())) {
            m_localDiffView->setDiffText(QStringLiteral("(Binary file: %1)").arg(path));
            return;
        }

        bool isStaged = item->data(0, Qt::UserRole + 1).toBool();
        QString diff = isStaged ? m_git->getStagedDiff(path)
                                : m_git->getWorkdirDiff(path);
        if (diff.isEmpty()) m_localDiffView->setDiffText(QStringLiteral("(No diff available for %1)").arg(path));
        else m_localDiffView->setDiffText(diff);
    });

    connect(m_historyFilesTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onCommitFileClicked);
            
    connect(m_localDiffView, &DiffViewWidget::stageHunkRequested, this, [this](const QString &patch) {
        if (m_git->stageHunk(patch)) {
            refreshAll();
        } else {
            QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });
    
    // Log
    connect(m_logTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onCommitSelected);
    
    // Branches context menus
    m_branchesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_branchesTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showBranchContextMenu);
    connect(m_branchesTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onBranchItemDoubleClicked);

    // Commit context menus
    m_logTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_logTable, &QTableView::customContextMenuRequested, this, &MainWindow::showCommitContextMenu);

    connect(m_commitBtn, &QPushButton::clicked,
            this, &MainWindow::doCommit);

    connect(m_git, &GitManager::repositoryOpened,
            this, [this](const QString &path) {
                setWindowTitle(QStringLiteral("GitZen — %1").arg(path));
                setRepoActionsEnabled(true);
                m_syncStatusLabel->setText(QStringLiteral("Sync: Ready"));
                
                m_dirModel->setRootPath(path);
                m_dirTree->setRootIndex(m_dirModel->index(path));

                QSettings settings("MyCompany", "GitZen");
                QString user = settings.value("github/username", "").toString();
                QString token = settings.value("github/token", "").toString();
                m_git->setCredentials(user, token);

                // Watch for changes
                m_watcher->addPath(path);
                refreshAll();
            });

    connect(m_git, &GitManager::repositoryClosed,
            this, [this]() {
                setWindowTitle(QStringLiteral("GitZen"));
                setRepoActionsEnabled(false);
                m_syncStatusLabel->setText(QStringLiteral("Sync: Offline"));
                
                m_dirModel->setRootPath(QString());
                m_stagedRoot->takeChildren();
                m_unstagedRoot->takeChildren();
                m_localDiffView->clearDiff();

                m_historyFilesTree->clear();
                m_historyDiffView->clearDiff();

                m_logModel->setCommits(QVector<CommitInfo>());
                m_selectedCommitId.clear();
                m_branchesTree->clear();
                
                m_statusLabel->setText(QStringLiteral("No repository open"));
                if (!m_watcher->directories().isEmpty())
                    m_watcher->removePaths(m_watcher->directories());
            });

    connect(m_git, &GitManager::errorOccurred,
            this, &MainWindow::onGitError);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &MainWindow::onRepoChanged);
}

// ─── Dark Theme ────────────────────────────────────────────────────

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
        QTreeWidget {
            background-color: #252526;
            color: #D4D4D4;
            border: 1px solid #3C3C3C;
            alternate-background-color: #2A2A2A;
        }
        QTreeWidget::item:selected {
            background-color: #094771;
        }
        QTreeWidget::item:hover {
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
            background-color: #3C3C3C;
            color: #D4D4D4;
            border: 1px solid #505050;
            border-radius: 3px;
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
        QScrollBar:vertical {
            background-color: #1E1E1E;
            width: 12px;
        }
        QScrollBar::handle:vertical {
            background-color: #424242;
            border-radius: 4px;
            min-height: 30px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4F4F4F;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #1E1E1E;
            height: 12px;
        }
        QScrollBar::handle:horizontal {
            background-color: #424242;
            border-radius: 4px;
            min-width: 30px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4F4F4F;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )"));
}

// ═══════════════════════════════════════════════════════════════════
//  Repository actions
// ═══════════════════════════════════════════════════════════════════

void MainWindow::openRepository()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Select Git Repository"));
    if (dir.isEmpty()) return;

    if (!m_git->openRepository(dir)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("'%1' is not a valid Git repository.\n\n%2")
                .arg(dir, m_git->lastError()));
    }
}

void MainWindow::closeRepository()
{
    m_git->closeRepository();
}

void MainWindow::refreshAll()
{
    if (!m_git->isOpen()) return;

    updateFileList();
    updateCommitLog();
    updateBranchesTree();
    updateBranchCombo();

    // Status bar summary
    int staged = m_stagedRoot->childCount();
    int unstaged = m_unstagedRoot->childCount();
    m_statusLabel->setText(
        QStringLiteral("📁 %1  |  %2 staged, %3 unstaged")
            .arg(m_git->repoPath())
            .arg(staged)
            .arg(unstaged));
}

// ═══════════════════════════════════════════════════════════════════
//  File list
// ═══════════════════════════════════════════════════════════════════

void MainWindow::updateFileList()
{
    // Clear children but keep root items
    qDeleteAll(m_stagedRoot->takeChildren());
    qDeleteAll(m_unstagedRoot->takeChildren());

    QVector<FileStatusEntry> entries = m_git->getFileStatus();

    for (const auto &entry : entries) {
        // Staged
        if (entry.hasIndexChanges()) {
            auto *item = new QTreeWidgetItem(m_stagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.indexStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, true);  // isStaged
            item->setTextAlignment(0, Qt::AlignCenter);
            
            QColor bgColor = QColor("#238636"); // default green
            if (entry.indexStatus == FileStatusEntry::Deleted) bgColor = QColor("#DA3633");
            else if (entry.indexStatus == FileStatusEntry::Modified) bgColor = QColor("#007ACC");
            else if (entry.indexStatus == FileStatusEntry::Renamed) bgColor = QColor("#8957E5");
            
            item->setBackground(0, bgColor);
            item->setForeground(0, Qt::white);
        }

        // Unstaged
        if (entry.hasWorktreeChanges()) {
            auto *item = new QTreeWidgetItem(m_unstagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.worktreeStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, false);  // isStaged
            item->setTextAlignment(0, Qt::AlignCenter);
            
            QColor bgColor = QColor("#238636"); // default green
            if (entry.worktreeStatus == FileStatusEntry::Deleted) bgColor = QColor("#DA3633");
            else if (entry.worktreeStatus == FileStatusEntry::Modified) bgColor = QColor("#007ACC");
            else if (entry.worktreeStatus == FileStatusEntry::Renamed) bgColor = QColor("#8957E5");
            else if (entry.worktreeStatus == FileStatusEntry::Untracked) bgColor = QColor("#7A7A7A"); // Gray for untracked
            
            item->setBackground(0, bgColor);
            item->setForeground(0, Qt::white);
        }
    }

    // Update root item labels
    m_stagedRoot->setText(1, QStringLiteral("Staged Changes (%1)").arg(m_stagedRoot->childCount()));
    m_unstagedRoot->setText(1, QStringLiteral("Unstaged Changes (%1)").arg(m_unstagedRoot->childCount()));
}

// ═══════════════════════════════════════════════════════════════════
//  Commit log
// ═══════════════════════════════════════════════════════════════════

void MainWindow::updateCommitLog()
{
    m_logModel->setCommits(m_git->getLog(200));
}

// ═══════════════════════════════════════════════════════════════════
//  Branches Tree
// ═══════════════════════════════════════════════════════════════════

void MainWindow::updateBranchesTree()
{
    m_branchesTree->clear();
    QVector<BranchInfo> branches = m_git->getBranches();

    QTreeWidgetItem *localRoot = new QTreeWidgetItem(m_branchesTree, {QStringLiteral("Local Branches")});
    localRoot->setExpanded(true);
    QTreeWidgetItem *remoteRoot = new QTreeWidgetItem(m_branchesTree, {QStringLiteral("Remote Branches")});
    remoteRoot->setExpanded(true);

    for (const auto &bi : branches) {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        QString display = bi.name;
        if (bi.isHead) {
            display = QStringLiteral("● %1").arg(bi.name);
            item->setData(0, Qt::FontRole, QFont(m_branchesTree->font().family(), -1, QFont::Bold));
        }
        item->setText(0, display);
        item->setData(0, Qt::UserRole, bi.name); // Store full branch name

        if (bi.isRemote) {
            remoteRoot->addChild(item);
        } else {
            localRoot->addChild(item);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Diff display
// ═══════════════════════════════════════════════════════════════════

    // Show diff for item is now handled directly via onCommitFileClicked

// ═══════════════════════════════════════════════════════════════════
//  Commit Graph Selection & Diff (Faz 5)
// ═══════════════════════════════════════════════════════════════════

void MainWindow::onCommitSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    
    m_historyFilesTree->clear();
    m_historyDiffView->clearDiff();
    
    if (selected.indexes().isEmpty()) {
        m_selectedCommitId.clear();
        return;
    }

    int row = selected.indexes().first().row();
    QModelIndex hashIndex = m_logModel->index(row, CommitGraphModel::ColHash);
    m_selectedCommitId = m_logModel->data(hashIndex, Qt::UserRole).toString();

    QModelIndex msgIndex = m_logModel->index(row, CommitGraphModel::ColMessage);
    QString msg = m_logModel->data(msgIndex, Qt::UserRole).toString();
    
    QModelIndex authorIndex = m_logModel->index(row, CommitGraphModel::ColAuthor);
    QString author = m_logModel->data(authorIndex, Qt::UserRole).toString();
    
    QModelIndex dateIndex = m_logModel->index(row, CommitGraphModel::ColDate);
    QDateTime date = m_logModel->data(dateIndex, Qt::UserRole).toDateTime();

    QString detailsHtml = QStringLiteral(
        "<div style='font-family: sans-serif;'>"
        "<div style='font-size: 14px; font-weight: bold; color: #E0E0E0; margin-bottom: 4px;'>%1</div>"
        "<div style='font-size: 12px; color: #808080; margin-bottom: 8px;'>"
        "<b>%2</b> committed on %3 &nbsp;&nbsp;&nbsp; <code>%4</code>"
        "</div>"
        "</div>"
    ).arg(msg.toHtmlEscaped(), author.toHtmlEscaped(), date.toString("yyyy-MM-dd hh:mm:ss"), m_selectedCommitId);
    
    m_commitDetailsLabel->setText(detailsHtml);
    
    // Fetch changed files for this commit
    QVector<FileStatusEntry> entries = m_git->getCommitChangedFiles(m_selectedCommitId);
    
    for (const auto &entry : entries) {
        auto *item = new QTreeWidgetItem(m_historyFilesTree);
        QString statusText = FileStatusEntry::statusString(entry.worktreeStatus);
        
        QFileInfo fi(entry.path);
        
        item->setText(0, statusText);
        item->setText(1, fi.fileName());
        item->setText(2, fi.path() == "." ? "" : fi.path());
        
        item->setData(0, Qt::UserRole, entry.path);
        item->setTextAlignment(0, Qt::AlignCenter);
        
        QColor bgColor;
        QColor fgColor = Qt::white;
        
        switch (entry.worktreeStatus) {
            case FileStatusEntry::Added:    bgColor = QColor("#238636"); break; // Green
            case FileStatusEntry::Deleted:  bgColor = QColor("#DA3633"); break; // Red
            case FileStatusEntry::Modified: bgColor = QColor("#007ACC"); break; // Blue
            case FileStatusEntry::Renamed:  bgColor = QColor("#8957E5"); break; // Purple
            default:                        bgColor = QColor("#7A7A7A"); break; // Gray
        }
        
        item->setBackground(0, bgColor);
        item->setForeground(0, fgColor);
        item->setForeground(1, bgColor.lighter(130));
    }
}

void MainWindow::onCommitFileClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return; // Ignored root items
    
    static const QStringList binExts = {"pdf", "png", "jpg", "jpeg", "gif", "exe", "dll", "so", "bin", "zip", "tar", "gz", "mp4", "mp3", "obj", "o", "a", "lib", "ttf", "woff"};
    if (binExts.contains(QFileInfo(path).suffix().toLower())) {
        m_historyDiffView->setDiffText(QStringLiteral("(Binary file: %1)").arg(path));
        return;
    }

    QString diff = m_git->getCommitDiff(m_selectedCommitId, path);
    if (diff.isEmpty()) {
        m_historyDiffView->setDiffText(QStringLiteral("(No diff available for %1 in commit %2)").arg(path, m_selectedCommitId));
    } else {
        m_historyDiffView->setDiffText(diff);
    }
}

void MainWindow::onFileItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!item || item == m_stagedRoot || item == m_unstagedRoot)
        return;

    QString path    = item->data(0, Qt::UserRole).toString();
    bool    isStaged = item->data(0, Qt::UserRole + 1).toBool();

    if (isStaged) {
        m_git->unstageFile(path);
    } else {
        m_git->stageFile(path);
    }
    refreshAll();
}

// ═══════════════════════════════════════════════════════════════════
//  Stage / Unstage
// ═══════════════════════════════════════════════════════════════════

void MainWindow::stageSelected()
{
    auto items = m_localChangesTree->selectedItems();
    for (auto *item : items) {
        if (item == m_stagedRoot || item == m_unstagedRoot) continue;
        bool isStaged = item->data(0, Qt::UserRole + 1).toBool();
        if (!isStaged) {
            m_git->stageFile(item->data(0, Qt::UserRole).toString());
        }
    }
    refreshAll();
}

void MainWindow::unstageSelected()
{
    auto items = m_localChangesTree->selectedItems();
    for (auto *item : items) {
        if (item == m_stagedRoot || item == m_unstagedRoot) continue;
        bool isStaged = item->data(0, Qt::UserRole + 1).toBool();
        if (isStaged) {
            m_git->unstageFile(item->data(0, Qt::UserRole).toString());
        }
    }
    refreshAll();
}

void MainWindow::stageAll()
{
    m_git->stageAll();
    refreshAll();
}

void MainWindow::unstageAll()
{
    m_git->unstageAll();
    refreshAll();
}

// ═══════════════════════════════════════════════════════════════════
//  Commit
// ═══════════════════════════════════════════════════════════════════

void MainWindow::doCommit()
{
    QString msg = m_commitEdit->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Commit"),
            QStringLiteral("Please enter a commit message."));
        return;
    }

    if (m_git->commit(msg)) {
        m_commitEdit->clear();
        refreshAll();
        statusBar()->showMessage(QStringLiteral("Commit successful!"), 3000);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Push / Pull
// ═══════════════════════════════════════════════════════════════════

void MainWindow::doPush()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Pushing to remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->push()) {
        m_syncStatusLabel->setText(QStringLiteral("Sync: Pushed at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully pushed to remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Push failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
    }
}

void MainWindow::doPull()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Pulling from remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->pull()) {
        m_syncStatusLabel->setText(QStringLiteral("Sync: Pulled at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully pulled from remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Pull failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
    }
}

void MainWindow::doFetch()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Fetching from remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->fetch()) {
        m_syncStatusLabel->setText(QStringLiteral("Sync: Fetched at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully fetched from remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Fetch failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
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

        QSettings settings("MyCompany", "GitZen");
        m_git->setCredentials(settings.value("github/username", "").toString(), settings.value("github/token", "").toString());

        QProgressDialog progress("Cloning repository...", "Cancel", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.show();
        QApplication::processEvents();

        if (m_git->cloneRepository(url, dest)) {
            QMessageBox::information(this, "Success", "Repository cloned successfully.");
            m_dirModel->setRootPath(dest);
            m_dirTree->setRootIndex(m_dirModel->index(dest));
            refreshAll();
            setRepoActionsEnabled(true);
        } else {
            QMessageBox::critical(this, "Error", QStringLiteral("Clone failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
        }
    }
}

void MainWindow::openCredentials()
{
    QSettings settings("MyCompany", "GitZen");

    QDialog dialog(this);
    dialog.setWindowTitle("GitHub Credentials");
    QFormLayout *layout = new QFormLayout(&dialog);

    QLineEdit *userEdit = new QLineEdit(&dialog);
    userEdit->setText(settings.value("github/username", "").toString());
    
    QLineEdit *tokenEdit = new QLineEdit(&dialog);
    tokenEdit->setText(settings.value("github/token", "").toString());
    tokenEdit->setEchoMode(QLineEdit::Password);
    
    layout->addRow("Username:", userEdit);
    layout->addRow("Personal Access Token (PAT):", tokenEdit);

    QLabel *info = new QLabel("Note: Passwords are no longer supported by GitHub over HTTPS.\nPlease generate a Personal Access Token (PAT) with 'repo' scope.", &dialog);
    info->setWordWrap(true);
    info->setStyleSheet("color: gray;");
    layout->addRow(info);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        settings.setValue("github/username", userEdit->text().trimmed());
        settings.setValue("github/token", tokenEdit->text().trimmed());
        m_git->setCredentials(userEdit->text().trimmed(), tokenEdit->text().trimmed());
        QMessageBox::information(this, "Saved", "Credentials saved successfully.");
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Branch operations
// ═══════════════════════════════════════════════════════════════════

void MainWindow::onBranchItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || item->parent() == nullptr) return; // Ignore clicks on Root items

    QString branchName = item->data(0, Qt::UserRole).toString();
    if (branchName.isEmpty()) return;

    QString currentBranch = m_git->getCurrentBranch();
    if (branchName == currentBranch) return;

    if (m_git->checkoutBranch(branchName)) {
        statusBar()->showMessage(
            QStringLiteral("Switched to branch '%1'").arg(branchName), 3000);
        refreshAll();
    } else {
        QMessageBox::warning(this, QStringLiteral("Checkout Failed"),
                             QStringLiteral("Failed to checkout branch. Do you have uncommitted changes?"));
    }
}

void MainWindow::createBranch()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, QStringLiteral("Create Branch"),
        QStringLiteral("Branch name:"),
        QLineEdit::Normal, QString(), &ok);

    if (ok && !name.isEmpty()) {
        if (m_git->createBranch(name)) {
            statusBar()->showMessage(
                QStringLiteral("Branch '%1' created").arg(name), 3000);
            refreshAll();
        }
    }
}

void MainWindow::deleteBranch()
{
    QVector<BranchInfo> branches = m_git->getBranches();
    QStringList branchNames;
    for (const auto &b : branches) {
        if (!b.isRemote && !b.isHead)
            branchNames << b.name;
    }

    if (branchNames.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Delete Branch"),
            QStringLiteral("No branches available to delete."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getItem(
        this, QStringLiteral("Delete Branch"),
        QStringLiteral("Select branch to delete:"),
        branchNames, 0, false, &ok);

    if (ok && !name.isEmpty()) {
        int ret = QMessageBox::question(this, QStringLiteral("Confirm Delete"),
            QStringLiteral("Delete branch '%1'?").arg(name));
        if (ret == QMessageBox::Yes) {
            if (m_git->deleteBranch(name)) {
                statusBar()->showMessage(
                    QStringLiteral("Branch '%1' deleted").arg(name), 3000);
                refreshAll();
            }
        }
    }
}

void MainWindow::mergeBranch()
{
    QVector<BranchInfo> branches = m_git->getBranches();
    QStringList branchNames;
    for (const auto &b : branches) {
        if (!b.isRemote && !b.isHead)
            branchNames << b.name;
    }

    if (branchNames.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Merge Branch"),
            QStringLiteral("No branches available to merge."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getItem(
        this, QStringLiteral("Merge Branch"),
        QStringLiteral("Select branch to merge into current:"),
        branchNames, 0, false, &ok);

    if (ok && !name.isEmpty()) {
        if (m_git->mergeBranch(name)) {
            statusBar()->showMessage(
                QStringLiteral("Merge successful!"), 3000);
            refreshAll();
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Misc
// ═══════════════════════════════════════════════════════════════════

void MainWindow::setRepoActionsEnabled(bool enabled)
{
    m_actClose->setEnabled(enabled);
    m_actRefresh->setEnabled(enabled);
    m_actPush->setEnabled(enabled);
    m_actPull->setEnabled(enabled);
    m_actStageAll->setEnabled(enabled);
    m_actUnstageAll->setEnabled(enabled);
    m_actNewBranch->setEnabled(enabled);
    m_actDeleteBranch->setEnabled(enabled);
    m_actMergeBranch->setEnabled(enabled);
    m_commitBtn->setEnabled(enabled);
    m_commitEdit->setEnabled(enabled);
}

void MainWindow::onRepoChanged(const QString & /*path*/)
{
    // Debounce: refresh after a short delay to avoid rapid re-reads
    QTimer::singleShot(500, this, &MainWindow::refreshAll);
}

void MainWindow::onGitError(const QString &message)
{
    statusBar()->showMessage(QStringLiteral("Error: %1").arg(message), 5000);
}


void MainWindow::showBranchContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_branchesTree->itemAt(pos);
    if (!item || item->parent() == nullptr) return;

    QString branchName = item->data(0, Qt::UserRole).toString();
    if (branchName.isEmpty()) return;

    QMenu menu(this);
    QAction *actCheckout = menu.addAction(QStringLiteral("Checkout %1").arg(branchName));
    QAction *actDelete   = menu.addAction(QStringLiteral("Delete %1").arg(branchName));

    QAction *res = menu.exec(m_branchesTree->viewport()->mapToGlobal(pos));
    if (res == actCheckout) {
        if (m_git->checkoutBranch(branchName)) refreshAll();
        else QMessageBox::critical(this, "Error", m_git->lastError());
    } else if (res == actDelete) {
        if (QMessageBox::question(this, "Delete Branch", QStringLiteral("Are you sure you want to delete branch '%1'?").arg(branchName)) == QMessageBox::Yes) {
            if (m_git->deleteBranch(branchName)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    }
}

void MainWindow::showLocalFilesContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_localChangesTree->itemAt(pos);
    if (!item || item == m_stagedRoot || item == m_unstagedRoot) return;

    QString path = item->data(0, Qt::UserRole).toString();

    QMenu menu(this);
    
    QFileInfo fi(path);
    QString fileName = fi.fileName();
    QString ext = fi.suffix();
    QString dir = fi.path();

    QAction *actIgnoreExact = menu.addAction(QStringLiteral("Ignore exact file (%1)").arg(fileName));
    QAction *actIgnoreExt = nullptr;
    if (!ext.isEmpty()) {
        actIgnoreExt = menu.addAction(QStringLiteral("Ignore all .%1 files").arg(ext));
    }
    QAction *actIgnoreFolder = nullptr;
    if (dir != ".") {
        actIgnoreFolder = menu.addAction(QStringLiteral("Ignore folder (%1/)").arg(dir));
    }

    menu.addSeparator();
    QAction *actDiscard = menu.addAction(QStringLiteral("Discard Changes"));
    menu.addSeparator();
    QAction *actOpenDir = menu.addAction(QStringLiteral("Open in Explorer"));

    QAction *res = menu.exec(m_localChangesTree->viewport()->mapToGlobal(pos));
    
    QString ignoreStr;
    if (res == actIgnoreExact) ignoreStr = path;
    else if (res && res == actIgnoreExt) ignoreStr = "*." + ext;
    else if (res && res == actIgnoreFolder) ignoreStr = dir + "/";

    if (!ignoreStr.isEmpty()) {
        if (m_git->addToGitignore(ignoreStr)) refreshAll();
        else QMessageBox::critical(this, "Error", m_git->lastError());
    } else if (res == actDiscard) {
        if (QMessageBox::question(this, "Discard", QStringLiteral("Discard local changes in '%1'?").arg(path)) == QMessageBox::Yes) {
            if (m_git->discardFileChanges(path)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    } else if (res == actOpenDir) {
        QString absPath = QDir(m_git->repoPath()).absoluteFilePath(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(absPath).absolutePath()));
    }
}

void MainWindow::showCommitContextMenu(const QPoint &pos)
{
    QModelIndex index = m_logTable->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();
    QModelIndex hashIndex = m_logModel->index(row, CommitGraphModel::ColHash);
    QString commitId = m_logModel->data(hashIndex, Qt::UserRole).toString();
    
    QModelIndex msgIndex = m_logModel->index(row, CommitGraphModel::ColMessage);
    QString commitMsg = m_logModel->data(msgIndex, Qt::DisplayRole).toString();

    QMenu menu(this);
    QAction *actBranch = menu.addAction(QStringLiteral("Create Branch Here..."));
    menu.addSeparator();
    QAction *actCherry = menu.addAction(QStringLiteral("Cherry-Pick this commit"));
    QAction *actRevert = menu.addAction(QStringLiteral("Revert this commit"));
    
    QAction *res = menu.exec(m_logTable->viewport()->mapToGlobal(pos));
    if (res == actBranch) {
        bool ok;
        QString name = QInputDialog::getText(this, "Create Branch", 
            QStringLiteral("Enter new branch name at commit:\n%1").arg(commitMsg), 
            QLineEdit::Normal, "", &ok);
            
        if (ok && !name.isEmpty()) {
            if (m_git->createBranchAt(name, commitId)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    } else if (res == actCherry) {
        if (QMessageBox::question(this, "Cherry-Pick", QStringLiteral("Cherry-pick commit %1?").arg(commitId.left(7))) == QMessageBox::Yes) {
            if (m_git->cherryPick(commitId)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    } else if (res == actRevert) {
        if (QMessageBox::question(this, "Revert", QStringLiteral("Revert commit %1?").arg(commitId.left(7))) == QMessageBox::Yes) {
            if (m_git->revertCommit(commitId)) refreshAll();
            else QMessageBox::critical(this, "Error", m_git->lastError());
        }
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(QStringLiteral("About GitZen"));
    QImage logo(":/resources/logo.png");
    logo = logo.convertToFormat(QImage::Format_ARGB32);
    float cx = logo.width() / 2.0f;
    float cy = logo.height() / 2.0f;
    float radius = cx - 2.0f;
    for (int y = 0; y < logo.height(); ++y) {
        for (int x = 0; x < logo.width(); ++x) {
            float dx = x - cx;
            float dy = y - cy;
            if (dx*dx + dy*dy > radius*radius) {
                logo.setPixelColor(x, y, Qt::transparent);
            }
        }
    }

    aboutBox.setIconPixmap(QPixmap::fromImage(logo).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(QStringLiteral(
        "<h3>GitZen v0.1.0</h3>"
        "<p>A fast, native, and zen-like Git GUI Client.</p>"
        "<p>Built with <b>C++</b>, <b>Qt6</b>, and <b>libgit2</b>.</p>"
        "<p><i>Built with a lot of coffee ☕ and ❤️ by Hakan.</i></p>"
    ));
    aboutBox.exec();
}

void MainWindow::updateBranchCombo()
{
    if (!m_git->isOpen()) {
        m_branchCombo->clear();
        m_branchCombo->setEnabled(false);
        return;
    }
    
    m_branchCombo->blockSignals(true);
    m_branchCombo->clear();
    
    QVector<BranchInfo> branches = m_git->getBranches();
    for (const auto &b : branches) {
        if (!b.isRemote) { // Only show local branches for checkout in the combo
            m_branchCombo->addItem(b.name);
            if (b.isHead) {
                m_branchCombo->setCurrentText(b.name);
            }
        }
    }
    m_branchCombo->setEnabled(true);
    m_branchCombo->blockSignals(false);
}
