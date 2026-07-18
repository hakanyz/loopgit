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
#include <QHeaderView>
#include "commitgraphmodel.h"
#include "commitgraphdelegate.h"
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
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
    setWindowTitle(QStringLiteral("QtGitClient"));
    resize(1280, 800);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
}

// ─── Menu Bar ──────────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();

    // File
    QMenu *fileMenu = mb->addMenu(QStringLiteral("&File"));
    m_actOpen  = fileMenu->addAction(QStringLiteral("📂 &Open Repository..."),
                                     QKeySequence::Open,
                                     this, &MainWindow::openRepository);
    m_actClose = fileMenu->addAction(QStringLiteral("Close Repository"),
                                     this, &MainWindow::closeRepository);
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
    m_actPush = repoMenu->addAction(QStringLiteral("⬆ &Push"),
                                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P),
                                    this, &MainWindow::doPush);
    m_actPull = repoMenu->addAction(QStringLiteral("⬇ Pu&ll"),
                                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L),
                                    this, &MainWindow::doPull);

    // Branch
    QMenu *branchMenu = mb->addMenu(QStringLiteral("&Branch"));
    m_actNewBranch    = branchMenu->addAction(QStringLiteral("New Branch..."),
                                              this, &MainWindow::createBranch);
    m_actDeleteBranch = branchMenu->addAction(QStringLiteral("Delete Branch..."),
                                              this, &MainWindow::deleteBranch);
    m_actMergeBranch  = branchMenu->addAction(QStringLiteral("Merge Branch..."),
                                              this, &MainWindow::mergeBranch);
}

// ─── Tool Bar ──────────────────────────────────────────────────────

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(QStringLiteral("Main"));
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(32, 32));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_toolBar->addAction(m_actOpen);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actPush);
    m_toolBar->addAction(m_actPull);
    m_toolBar->addAction(m_actRefresh);
    m_toolBar->addSeparator();

    // Branch combo removed (now using Branches Tree on the left)
}

// ─── Central Widget ────────────────────────────────────────────────

void MainWindow::setupCentralWidget()
{
    // ── Branches Tree (Left Panel) ──────────────────────
    m_branchesTree = new QTreeWidget;
    m_branchesTree->setHeaderLabels({QStringLiteral("Branches")});
    m_branchesTree->setRootIsDecorated(true);
    m_branchesTree->setAlternatingRowColors(true);

    // ── Commit Files Panel (Right Top - Right side) ─────
    m_commitFilesTree = new QTreeWidget;
    m_commitFilesTree->setHeaderLabels({QStringLiteral("Status"), QStringLiteral("File")});
    m_commitFilesTree->setColumnWidth(0, 80);
    m_commitFilesTree->setRootIsDecorated(true);
    m_commitFilesTree->setAlternatingRowColors(true);
    m_commitFilesTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Roots for local changes (only visible when WORKING_TREE is selected)
    m_stagedRoot = new QTreeWidgetItem(m_commitFilesTree, {QStringLiteral(""), QStringLiteral("Staged Changes")});
    m_stagedRoot->setExpanded(true);
    m_stagedRoot->setFlags(Qt::ItemIsEnabled);
    m_stagedRoot->setHidden(true);

    m_unstagedRoot = new QTreeWidgetItem(m_commitFilesTree, {QStringLiteral(""), QStringLiteral("Unstaged Changes")});
    m_unstagedRoot->setExpanded(true);
    m_unstagedRoot->setFlags(Qt::ItemIsEnabled);
    m_unstagedRoot->setHidden(true);

    // ── Commit Panel (Buttons & Input) ──────────────────
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
    m_commitEdit->setMaximumHeight(100);

    m_commitBtn = new QPushButton(QStringLiteral("✓ Commit"));
    m_commitBtn->setMinimumHeight(32);

    QVBoxLayout *commitPanelLayout = new QVBoxLayout;
    commitPanelLayout->setContentsMargins(4, 4, 4, 4);
    commitPanelLayout->setSpacing(4);
    commitPanelLayout->addLayout(stageBtnLayout);
    commitPanelLayout->addWidget(commitLabel);
    commitPanelLayout->addWidget(m_commitEdit);
    commitPanelLayout->addWidget(m_commitBtn);

    m_commitPanelWidget = new QWidget;
    m_commitPanelWidget->setLayout(commitPanelLayout);
    m_commitPanelWidget->setVisible(false); // Hidden by default

    QWidget *rightTopWidget = new QWidget;
    QVBoxLayout *rightTopLayout = new QVBoxLayout;
    rightTopLayout->setContentsMargins(0, 0, 0, 0);
    rightTopLayout->setSpacing(0);
    rightTopLayout->addWidget(m_commitFilesTree, 1);
    rightTopLayout->addWidget(m_commitPanelWidget, 0);
    rightTopWidget->setLayout(rightTopLayout);

    // ── Right panel: diff view ──────────────────────────
    m_diffView = new DiffViewWidget;

    // ── Bottom panel: commit log ────────────────────────
    m_logTable = new QTableView;
    m_logTable->setModel(m_logModel);
    m_logTable->setItemDelegate(m_logDelegate);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->verticalHeader()->setVisible(false);
    
    // Stretch message column, fix graph and hash widths
    QHeaderView *hHeader = m_logTable->horizontalHeader();
    hHeader->setStretchLastSection(true);
    hHeader->setSectionResizeMode(CommitGraphModel::ColGraph, QHeaderView::Fixed);
    hHeader->setSectionResizeMode(CommitGraphModel::ColMessage, QHeaderView::Stretch);
    
    m_logTable->setColumnWidth(CommitGraphModel::ColGraph, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColHash, 80);
    m_logTable->setColumnWidth(CommitGraphModel::ColAuthor, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColDate, 140);

    // ── Center Top Splitter (Graph | Commit Files) ──────
    m_centerTopSplitter = new QSplitter(Qt::Horizontal);
    m_centerTopSplitter->addWidget(m_logTable);
    m_centerTopSplitter->addWidget(rightTopWidget);
    m_centerTopSplitter->setStretchFactor(0, 5); // Graph is wider
    m_centerTopSplitter->setStretchFactor(1, 2); // Files list is narrower

    // ── Right Splitter (Top Center | Diff) ──────────────
    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_rightSplitter->addWidget(m_centerTopSplitter);
    m_rightSplitter->addWidget(m_diffView);
    m_rightSplitter->setStretchFactor(0, 6); // Graph+Files gets more vertical space
    m_rightSplitter->setStretchFactor(1, 3); // Diff gets less

    // ── Main Splitter (Branches | Right Splitter) ───────
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_branchesTree);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 1); // Branches panel
    m_mainSplitter->setStretchFactor(1, 6); // Center/Right panels

    setCentralWidget(m_mainSplitter);
}

// ─── Status Bar ────────────────────────────────────────────────────

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(QStringLiteral("No repository open"));
    statusBar()->addPermanentWidget(m_statusLabel, 1);
}

// ─── Connections ───────────────────────────────────────────────────

void MainWindow::connectSignals()
{
    connect(m_commitFilesTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onFileItemDoubleClicked);

    connect(m_commitFilesTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onCommitFileClicked);

    connect(m_logTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onCommitSelected);

    connect(m_commitBtn, &QPushButton::clicked,
            this, &MainWindow::doCommit);

    connect(m_branchesTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onBranchItemDoubleClicked);

    connect(m_git, &GitManager::repositoryOpened,
            this, [this](const QString &path) {
                setWindowTitle(QStringLiteral("QtGitClient — %1").arg(path));
                setRepoActionsEnabled(true);

                // Watch for changes
                m_watcher->addPath(path);
                refreshAll();
            });

    connect(m_git, &GitManager::repositoryClosed,
            this, [this]() {
                setWindowTitle(QStringLiteral("QtGitClient"));
                setRepoActionsEnabled(false);
                m_stagedRoot->takeChildren();
                m_unstagedRoot->takeChildren();
                m_commitFilesTree->clear();
                m_diffView->clearDiff();
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
            border-radius: 3px;
            padding: 4px 8px;
        }
        QToolButton:hover {
            background-color: #3C3C3C;
            border: 1px solid #505050;
        }
        QToolButton:pressed {
            background-color: #094771;
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
    
    // Clear previously added commit file items, but KEEP the staged/unstaged roots
    for (int i = m_commitFilesTree->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem *item = m_commitFilesTree->topLevelItem(i);
        if (item != m_stagedRoot && item != m_unstagedRoot) {
            delete item;
        }
    }
    m_diffView->clearDiff();
    
    if (selected.indexes().isEmpty()) {
        m_selectedCommitId.clear();
        m_commitPanelWidget->hide();
        m_stagedRoot->setHidden(true);
        m_unstagedRoot->setHidden(true);
        return;
    }

    int row = selected.indexes().first().row();
    QModelIndex hashIndex = m_logModel->index(row, CommitGraphModel::ColHash);
    m_selectedCommitId = m_logModel->data(hashIndex).toString();
    
    if (m_selectedCommitId == "WORKING_TREE") {
        m_commitPanelWidget->show();
        m_stagedRoot->setHidden(false);
        m_unstagedRoot->setHidden(false);
        return;
    }
    
    m_commitPanelWidget->hide();
    m_stagedRoot->setHidden(true);
    m_unstagedRoot->setHidden(true);
    
    // Fetch changed files for this commit
    QVector<FileStatusEntry> entries = m_git->getCommitChangedFiles(m_selectedCommitId);
    
    for (const auto &entry : entries) {
        auto *item = new QTreeWidgetItem(m_commitFilesTree);
        QString statusText = FileStatusEntry::statusChar(entry.worktreeStatus);
        item->setText(0, statusText);
        item->setText(1, entry.path);
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
    
    if (m_selectedCommitId == "WORKING_TREE") {
        bool isStaged = item->data(0, Qt::UserRole + 1).toBool();
        QString diff = isStaged ? m_git->getStagedDiff(path)
                                : m_git->getWorkdirDiff(path);
        if (diff.isEmpty()) {
            m_diffView->setDiffText(QStringLiteral("(No diff available for %1)").arg(path));
        } else {
            m_diffView->setDiffText(diff);
        }
        return;
    }

    QString diff = m_git->getCommitDiff(m_selectedCommitId, path);
    if (diff.isEmpty()) {
        m_diffView->setDiffText(QStringLiteral("(No diff available for %1 in commit %2)").arg(path, m_selectedCommitId));
    } else {
        m_diffView->setDiffText(diff);
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
    auto items = m_commitFilesTree->selectedItems();
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
    auto items = m_commitFilesTree->selectedItems();
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
    statusBar()->showMessage(QStringLiteral("Pushing..."));
    QApplication::processEvents();

    if (m_git->push()) {
        statusBar()->showMessage(QStringLiteral("Push successful!"), 3000);
    }
    refreshAll();
}

void MainWindow::doPull()
{
    statusBar()->showMessage(QStringLiteral("Pulling..."));
    QApplication::processEvents();

    if (m_git->pull()) {
        statusBar()->showMessage(QStringLiteral("Pull successful!"), 3000);
    }
    refreshAll();
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
