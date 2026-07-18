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

    // Branch combo
    QLabel *brLbl = new QLabel(QStringLiteral("  🌿 Branch: "));
    m_toolBar->addWidget(brLbl);

    m_branchCombo = new QComboBox;
    m_branchCombo->setMinimumWidth(180);
    m_branchCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_toolBar->addWidget(m_branchCombo);
}

// ─── Central Widget ────────────────────────────────────────────────

void MainWindow::setupCentralWidget()
{
    // ── Left panel: file tree + commit area ─────────────
    m_fileTree = new QTreeWidget;
    m_fileTree->setHeaderLabels({QStringLiteral("Status"), QStringLiteral("File")});
    m_fileTree->setColumnWidth(0, 60);
    m_fileTree->setRootIsDecorated(true);
    m_fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTree->setAlternatingRowColors(true);

    // Root items for staged / unstaged groups
    m_stagedRoot = new QTreeWidgetItem(m_fileTree,
                       {QStringLiteral(""), QStringLiteral("Staged Changes")});
    m_stagedRoot->setExpanded(true);
    m_stagedRoot->setFlags(Qt::ItemIsEnabled);

    m_unstagedRoot = new QTreeWidgetItem(m_fileTree,
                        {QStringLiteral(""), QStringLiteral("Unstaged Changes")});
    m_unstagedRoot->setExpanded(true);
    m_unstagedRoot->setFlags(Qt::ItemIsEnabled);

    // Stage / unstage buttons
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

    // Commit message
    QLabel *commitLabel = new QLabel(QStringLiteral("Commit Message:"));
    m_commitEdit = new QTextEdit;
    m_commitEdit->setPlaceholderText(QStringLiteral("Enter commit message..."));
    m_commitEdit->setMaximumHeight(100);

    m_commitBtn = new QPushButton(QStringLiteral("✓ Commit"));
    m_commitBtn->setMinimumHeight(32);

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(4);
    leftLayout->addWidget(m_fileTree, 1);
    leftLayout->addLayout(stageBtnLayout);
    leftLayout->addWidget(commitLabel);
    leftLayout->addWidget(m_commitEdit);
    leftLayout->addWidget(m_commitBtn);

    QWidget *leftPanel = new QWidget;
    leftPanel->setLayout(leftLayout);

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

    // ── Splitters ───────────────────────────────────────
    // Right area: top (log table) | bottom (diff view)
    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_rightSplitter->addWidget(m_logTable);
    m_rightSplitter->addWidget(m_diffView);
    m_rightSplitter->setStretchFactor(0, 5); // Log gets more space
    m_rightSplitter->setStretchFactor(1, 3); // Diff gets less

    // Main area: left (file tree + commit) | right (rightSplitter)
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 1); // Left panel (files)
    m_mainSplitter->setStretchFactor(1, 4); // Log + Diff gets most space

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
    connect(m_fileTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onFileItemClicked);
    connect(m_fileTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onFileItemDoubleClicked);

    connect(m_commitBtn, &QPushButton::clicked,
            this, &MainWindow::doCommit);

    connect(m_branchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBranchChanged);

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
                m_diffView->clearDiff();
                m_logModel->setCommits(QVector<CommitInfo>());
                m_branchCombo->clear();
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
    // Save current selection
    QTreeWidgetItem *prev = m_fileTree->currentItem();
    QString prevPath = prev ? prev->data(0, Qt::UserRole).toString() : QString();

    // Clear children but keep root items
    qDeleteAll(m_stagedRoot->takeChildren());
    qDeleteAll(m_unstagedRoot->takeChildren());

    QVector<FileStatusEntry> entries = m_git->getFileStatus();
    QTreeWidgetItem *toSelect = nullptr;

    for (const auto &entry : entries) {
        // Staged
        if (entry.hasIndexChanges()) {
            auto *item = new QTreeWidgetItem(m_stagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.indexStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, true);  // isStaged

            // Color-code the status character
            QColor color;
            switch (entry.indexStatus) {
                case FileStatusEntry::Added:    color = QColor("#4EC9B0"); break;
                case FileStatusEntry::Deleted:  color = QColor("#F14C4C"); break;
                case FileStatusEntry::Modified: color = QColor("#E2C08D"); break;
                case FileStatusEntry::Renamed:  color = QColor("#569CD6"); break;
                default:                        color = QColor("#D4D4D4"); break;
            }
            item->setForeground(0, color);

            if (entry.path == prevPath) toSelect = item;
        }

        // Unstaged
        if (entry.hasWorktreeChanges()) {
            auto *item = new QTreeWidgetItem(m_unstagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.worktreeStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, false);  // isStaged

            QColor color;
            switch (entry.worktreeStatus) {
                case FileStatusEntry::Untracked: color = QColor("#73C991"); break;
                case FileStatusEntry::Deleted:   color = QColor("#F14C4C"); break;
                case FileStatusEntry::Modified:  color = QColor("#E2C08D"); break;
                default:                         color = QColor("#D4D4D4"); break;
            }
            item->setForeground(0, color);

            if (entry.path == prevPath && !toSelect) toSelect = item;
        }
    }

    // Update root item labels
    m_stagedRoot->setText(1,
        QStringLiteral("Staged Changes (%1)").arg(m_stagedRoot->childCount()));
    m_unstagedRoot->setText(1,
        QStringLiteral("Unstaged Changes (%1)").arg(m_unstagedRoot->childCount()));

    // Restore selection
    if (toSelect) {
        m_fileTree->setCurrentItem(toSelect);
        showDiffForItem(toSelect);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Commit log
// ═══════════════════════════════════════════════════════════════════

void MainWindow::updateCommitLog()
{
    m_logModel->setCommits(m_git->getLog(200));
}

// ═══════════════════════════════════════════════════════════════════
//  Branch combo
// ═══════════════════════════════════════════════════════════════════

void MainWindow::updateBranchCombo()
{
    m_updatingBranch = true;
    m_branchCombo->clear();

    QVector<BranchInfo> branches = m_git->getBranches();
    int headIdx = -1;

    for (int i = 0; i < branches.size(); ++i) {
        const auto &bi = branches[i];
        if (bi.isRemote) continue;  // only show local branches in combo

        QString display = bi.name;
        if (bi.isHead) {
            display = QStringLiteral("● %1").arg(bi.name);
            headIdx = m_branchCombo->count();
        }
        m_branchCombo->addItem(display, bi.name);
    }

    if (headIdx >= 0)
        m_branchCombo->setCurrentIndex(headIdx);

    m_updatingBranch = false;
}

// ═══════════════════════════════════════════════════════════════════
//  Diff display
// ═══════════════════════════════════════════════════════════════════

void MainWindow::showDiffForItem(QTreeWidgetItem *item)
{
    if (!item || item == m_stagedRoot || item == m_unstagedRoot) {
        m_diffView->clearDiff();
        return;
    }

    QString path    = item->data(0, Qt::UserRole).toString();
    bool    isStaged = item->data(0, Qt::UserRole + 1).toBool();

    QString diff = isStaged ? m_git->getStagedDiff(path)
                            : m_git->getWorkdirDiff(path);

    if (diff.isEmpty()) {
        m_diffView->setDiffText(
            QStringLiteral("(No diff available for %1)").arg(path));
    } else {
        m_diffView->setDiffText(diff);
    }
}

void MainWindow::onFileItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    showDiffForItem(item);
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
    auto items = m_fileTree->selectedItems();
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
    auto items = m_fileTree->selectedItems();
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

void MainWindow::onBranchChanged(int index)
{
    if (m_updatingBranch || index < 0) return;

    QString branchName = m_branchCombo->itemData(index).toString();
    QString currentBranch = m_git->getCurrentBranch();

    if (branchName == currentBranch) return;

    if (m_git->checkoutBranch(branchName)) {
        statusBar()->showMessage(
            QStringLiteral("Switched to branch '%1'").arg(branchName), 3000);
        refreshAll();
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
    m_branchCombo->setEnabled(enabled);
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
