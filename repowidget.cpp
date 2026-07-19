#include <QProgressDialog>
#include <QSettings>
#include "repowidget.h"
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
#include "branchdialog.h"
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
#include <QCheckBox>

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Construction / Destruction
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

RepoWidget::RepoWidget(const QString &repoPath, QWidget *parent)
    : QWidget(parent), m_repoPath(repoPath)
    , m_git(new GitManager(this))
    , m_watcher(new QFileSystemWatcher(this))
    , m_logModel(new CommitGraphModel(this))
    , m_logDelegate(new CommitGraphDelegate(this))
{
    setupUi();
    connectSignals();
    
    if (m_git->openRepository(m_repoPath)) {
        refreshAll();
    } else {
        emit statusMessage(QStringLiteral("Failed to open repository: %1").arg(m_git->lastError()));
    }
}

RepoWidget::~RepoWidget() = default;

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  UI Setup
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

void RepoWidget::setupUi() {
    setupCentralWidget();
}
void RepoWidget::setupCentralWidget() {
    m_stackedWidget = new QStackedWidget(this);

    m_dirModel = new QFileSystemModel(this);
    m_dirTree = new QTreeView;
    m_dirTree->setModel(m_dirModel);
    m_dirTree->setColumnHidden(1, true);
    m_dirTree->setColumnHidden(2, true);
    m_dirTree->setColumnHidden(3, true);
    m_dirTree->setHeaderHidden(true);
    m_dirTree->setAlternatingRowColors(true);

    m_localChangesTree = new QTreeWidget;
    m_localChangesTree->setHeaderLabels({"Status", "File"});
    m_localChangesTree->setColumnWidth(0, 80);
    m_localChangesTree->setRootIsDecorated(true);
    m_localChangesTree->setAlternatingRowColors(true);
    m_localChangesTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_stagedRoot = new QTreeWidgetItem(m_localChangesTree, {"", "Staged Changes"});
    m_stagedRoot->setExpanded(true);
    m_stagedRoot->setFlags(Qt::ItemIsEnabled);

    m_unstagedRoot = new QTreeWidgetItem(m_localChangesTree, {"", "Unstaged Changes"});
    m_unstagedRoot->setExpanded(true);
    m_unstagedRoot->setFlags(Qt::ItemIsEnabled);

    QPushButton *stageBtn   = new QPushButton(QString::fromUtf8("\xe2\x96\xb2 Stage"));
    QPushButton *unstageBtn = new QPushButton(QString::fromUtf8("\xe2\x96\xbc Unstage"));
    QPushButton *stageAllBtn   = new QPushButton(QString::fromUtf8("\xe2\x96\xb2\xe2\x96\xb2 Stage All"));
    QPushButton *unstageAllBtn = new QPushButton(QString::fromUtf8("\xe2\x96\xbc\xe2\x96\xbc Unstage All"));
    
    QPushButton *stashBtn = new QPushButton("Stash");
    QPushButton *stashPopBtn = new QPushButton("Pop Stash");

    connect(stageBtn,      &QPushButton::clicked, this, &RepoWidget::stageSelected);
    connect(unstageBtn,    &QPushButton::clicked, this, &RepoWidget::unstageSelected);
    connect(stageAllBtn,   &QPushButton::clicked, this, &RepoWidget::stageAll);
    connect(unstageAllBtn, &QPushButton::clicked, this, &RepoWidget::unstageAll);
    connect(stashBtn,      &QPushButton::clicked, this, &RepoWidget::doStashSave);
    connect(stashPopBtn,   &QPushButton::clicked, this, &RepoWidget::doStashPop);

    QHBoxLayout *stageBtnLayout = new QHBoxLayout;
    stageBtnLayout->setContentsMargins(0, 0, 0, 0);
    stageBtnLayout->addWidget(stageBtn);
    stageBtnLayout->addWidget(unstageBtn);
    stageBtnLayout->addWidget(stageAllBtn);
    stageBtnLayout->addWidget(unstageAllBtn);
    stageBtnLayout->addStretch();
    stageBtnLayout->addWidget(stashBtn);
    stageBtnLayout->addWidget(stashPopBtn);

    // ── Left side: file changes tree + stage buttons ──
    QWidget *fileChangesWidget = new QWidget;
    QVBoxLayout *fileChangesLayout = new QVBoxLayout(fileChangesWidget);
    fileChangesLayout->setContentsMargins(0, 0, 0, 0);
    fileChangesLayout->setSpacing(2);
    fileChangesLayout->addWidget(m_localChangesTree, 1);
    fileChangesLayout->addLayout(stageBtnLayout);

    // ── Right side: compact commit panel ──
    QLabel *commitLabel = new QLabel("Commit Message:");
    commitLabel->setStyleSheet("font-weight: bold; padding: 2px;");

    m_commitEdit = new QTextEdit;
    m_commitEdit->setPlaceholderText("Enter commit message...");

    m_amendCheck = new QCheckBox("Amend last commit");
    m_pushAfterCommitCheck = new QCheckBox("Push after commit");

    m_commitBtn = new QPushButton(QString::fromUtf8("\xe2\x9c\x93 Commit"));
    m_commitBtn->setMinimumHeight(36);
    m_commitBtn->setStyleSheet("font-weight: bold; font-size: 14px;");

    QVBoxLayout *commitPanelLayout = new QVBoxLayout;
    commitPanelLayout->setContentsMargins(6, 6, 6, 6);
    commitPanelLayout->setSpacing(6);
    commitPanelLayout->addWidget(commitLabel);
    commitPanelLayout->addWidget(m_commitEdit, 1);
    commitPanelLayout->addWidget(m_amendCheck);
    commitPanelLayout->addWidget(m_pushAfterCommitCheck);
    commitPanelLayout->addWidget(m_commitBtn);

    m_commitPanelWidget = new QWidget;
    m_commitPanelWidget->setLayout(commitPanelLayout);
    m_commitPanelWidget->setMinimumWidth(220);
    m_commitPanelWidget->setMaximumWidth(320);

    // ── Horizontal splitter: file changes | commit panel ──
    QSplitter *localContentSplitter = new QSplitter(Qt::Horizontal);
    localContentSplitter->addWidget(fileChangesWidget);
    localContentSplitter->addWidget(m_commitPanelWidget);
    localContentSplitter->setStretchFactor(0, 7);
    localContentSplitter->setStretchFactor(1, 3);

    QSplitter *localTopSplitter = new QSplitter(Qt::Horizontal);
    localTopSplitter->addWidget(m_dirTree);
    localTopSplitter->addWidget(localContentSplitter);
    localTopSplitter->setStretchFactor(0, 2);
    localTopSplitter->setStretchFactor(1, 8);

    m_localDiffView = new DiffViewWidget;

    QSplitter *localMainSplitter = new QSplitter(Qt::Vertical);
    localMainSplitter->addWidget(localTopSplitter);
    localMainSplitter->addWidget(m_localDiffView);
    localMainSplitter->setStretchFactor(0, 6);
    localMainSplitter->setStretchFactor(1, 4);

    m_branchesTree = new QTreeWidget;
    m_branchesTree->setHeaderHidden(true);
    m_branchesTree->setRootIsDecorated(true);
    m_branchesTree->setAlternatingRowColors(true);
    m_branchesTree->setContextMenuPolicy(Qt::CustomContextMenu);

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
        "QTableView { border: none; selection-background-color: #062f4a; selection-color: white; outline: none; }"
        "QTableView::item { border: none; border-bottom: 1px solid transparent; }"
        "QTableView::item:selected { background-color: #062f4a; }"
    );
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->verticalHeader()->setDefaultSectionSize(28);
    
    QHeaderView *hHeader = m_logTable->horizontalHeader();
    hHeader->setStretchLastSection(true);
    hHeader->setSectionResizeMode(CommitGraphModel::ColGraph, QHeaderView::Fixed);
    hHeader->setSectionResizeMode(CommitGraphModel::ColMessage, QHeaderView::Stretch);
    
    m_logTable->setColumnWidth(CommitGraphModel::ColGraph, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColHash, 70);
    m_logTable->setColumnWidth(CommitGraphModel::ColAuthor, 120);
    m_logTable->setColumnWidth(CommitGraphModel::ColDate, 115);

    m_historyFilesTree = new QTreeWidget;
    m_historyFilesTree->setHeaderLabels({"Status", "Name", "Path"});
    m_historyFilesTree->setColumnWidth(0, 80);
    m_historyFilesTree->setColumnWidth(1, 150);
    m_historyFilesTree->setRootIsDecorated(false);
    m_historyFilesTree->setAlternatingRowColors(true);
    
    m_commitDetailsLabel = new QLabel;
    m_commitDetailsLabel->setWordWrap(true);
    m_commitDetailsLabel->setTextFormat(Qt::RichText);
    m_commitDetailsLabel->setMinimumHeight(60);
    m_commitDetailsLabel->setStyleSheet("padding: 8px; background-color: #252526; border-bottom: 1px solid #3C3C3C;");
    m_commitDetailsLabel->setText("<span style='color:#808080;'>Select a commit to view details</span>");

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

    m_stackedWidget->addWidget(localMainSplitter);
    m_stackedWidget->addWidget(historyMainSplitter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_stackedWidget);
}

void RepoWidget::switchPerspective(int index)
{
    if (m_stackedWidget) {
        if (m_git->isOpen()) {
            m_stackedWidget->setCurrentIndex(index);
        }
    }
}

void RepoWidget::connectSignals()
{
    m_localChangesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_localChangesTree, &QTreeWidget::customContextMenuRequested, this, &RepoWidget::showLocalFilesContextMenu);

    m_dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_dirTree, &QTreeView::customContextMenuRequested, this, &RepoWidget::showDirTreeContextMenu);

    connect(m_localChangesTree, &QTreeWidget::itemDoubleClicked,
            this, &RepoWidget::onFileItemDoubleClicked);

    connect(m_localChangesTree, &QTreeWidget::itemClicked,
            this, [this](QTreeWidgetItem *item, int){
        if (!item || item == m_stagedRoot || item == m_unstagedRoot) {
            m_localDiffView->clearDiff();
            return;
        }
        QString path = item->data(0, Qt::UserRole).toString();
        
        static const QStringList binExts = {"pdf", "png", "jpg", "jpeg", "gif", "exe", "dll", "so", "bin", "zip", "tar", "gz", "mp4", "mp3", "obj", "o", "a", "lib", "ttf", "woff", "hex", "elf", "map", "out"};
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
            this, &RepoWidget::onCommitFileClicked);
            
    connect(m_localDiffView, &DiffViewWidget::stageHunkRequested, this, [this](const QString &patch) {
        if (m_git->stageHunk(patch)) {
            refreshAll();
        } else {
            QMessageBox::critical(this, "Error", m_git->lastError());
        }
    });
    
    // Log
    connect(m_logTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RepoWidget::onCommitSelected);
    
    // Branches context menus
    m_branchesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_branchesTree, &QTreeWidget::itemDoubleClicked, this, &RepoWidget::onBranchItemDoubleClicked);
    connect(m_branchesTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = m_branchesTree->itemAt(pos);
        if (!item || item->parent() == nullptr) return; // Ignore root items like "Local" or "Remote"

        QString branchName = item->data(0, Qt::UserRole).toString();
        if (branchName.isEmpty()) return;

        QMenu menu(this);
        menu.addAction("Checkout", this, [this, branchName]() {
            if (m_git->checkoutBranch(branchName)) {
                emit statusMessage(QStringLiteral("Switched to branch '%1'").arg(branchName));
                refreshAll();
            } else {
                QMessageBox::warning(this, "Checkout Failed", "Failed to checkout branch. Do you have uncommitted changes?");
            }
        });
        
        // Don't show Merge/Delete for current branch
        if (branchName != m_git->getCurrentBranch()) {
            menu.addAction("Merge into current branch", this, [this, branchName]() {
                if (QMessageBox::question(this, "Merge", QString("Merge branch '%1' into current branch?").arg(branchName)) == QMessageBox::Yes) {
                    if (m_git->mergeBranch(branchName)) {
                        emit statusMessage(QStringLiteral("Merged branch '%1' successfully").arg(branchName));
                        refreshAll();
                    } else {
                        QMessageBox::warning(this, "Merge Failed", "Failed to merge branch or there are conflicts.");
                    }
                }
            });
            menu.addAction("Delete branch", this, [this, branchName]() {
                if (QMessageBox::question(this, "Delete", QString("Are you sure you want to delete branch '%1'?").arg(branchName)) == QMessageBox::Yes) {
                    if (m_git->deleteBranch(branchName)) {
                        emit statusMessage(QStringLiteral("Deleted branch '%1'").arg(branchName));
                        refreshAll();
                    }
                }
            });
        }
        menu.exec(m_branchesTree->viewport()->mapToGlobal(pos));
    });

    // Commit context menus
    m_logTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_logTable, &QTableView::customContextMenuRequested, this, &RepoWidget::showCommitContextMenu);

    connect(m_commitBtn, &QPushButton::clicked,
            this, &RepoWidget::doCommit);

    connect(m_git, &GitManager::repositoryOpened,
            this, [this](const QString &path) {
                setWindowTitle(QStringLiteral("LoopGit  %1").arg(path));
                emit statusMessage(QStringLiteral("Sync: Ready"));
                m_dirModel->setRootPath(path);
                m_dirTree->setRootIndex(m_dirModel->index(path));
                m_watcher->addPath(path);
                switchPerspective(0);
                refreshAll();
            });

    connect(m_git, &GitManager::repositoryClosed,
            this, [this]() {
                setWindowTitle(QStringLiteral("LoopGit"));
                emit statusMessage(QStringLiteral("Sync: Offline"));
                m_dirModel->setRootPath(QString());
                m_stagedRoot->takeChildren();
                m_unstagedRoot->takeChildren();
                m_localDiffView->clearDiff();
                m_historyFilesTree->clear();
                m_historyDiffView->clearDiff();
                m_logModel->setCommits(QVector<CommitInfo>());
                m_selectedCommitId.clear();
                m_branchesTree->clear();
                emit statusMessage(QStringLiteral("No repository open"));
                if (!m_watcher->directories().isEmpty())
                    m_watcher->removePaths(m_watcher->directories());
                switchPerspective(0);
            });

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &RepoWidget::onRepoChanged);
}

void RepoWidget::refreshAll()
{
    if (!m_git->isOpen()) return;

    updateFileList();
    updateCommitLog();
    updateBranchesTree();
    
    // Status bar summary
    int staged = m_stagedRoot->childCount();
    int unstaged = m_unstagedRoot->childCount();
    emit statusMessage(
        QStringLiteral("?? %1  |  %2 staged, %3 unstaged")
            .arg(m_git->repoPath())
            .arg(staged)
            .arg(unstaged));
}

void RepoWidget::updateFileList()
{
    qDeleteAll(m_stagedRoot->takeChildren());
    qDeleteAll(m_unstagedRoot->takeChildren());

    QVector<FileStatusEntry> entries = m_git->getFileStatus();

    for (const auto &entry : entries) {
        if (entry.hasIndexChanges()) {
            auto *item = new QTreeWidgetItem(m_stagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.indexStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, true);
            item->setTextAlignment(0, Qt::AlignCenter);
            
            QColor bgColor = QColor("#238636");
            if (entry.indexStatus == FileStatusEntry::Deleted) bgColor = QColor("#DA3633");
            else if (entry.indexStatus == FileStatusEntry::Modified) bgColor = QColor("#007ACC");
            else if (entry.indexStatus == FileStatusEntry::Renamed) bgColor = QColor("#8957E5");
            
            item->setBackground(0, bgColor);
            item->setForeground(0, Qt::white);
        }

        if (entry.hasWorktreeChanges()) {
            auto *item = new QTreeWidgetItem(m_unstagedRoot);
            item->setText(0, FileStatusEntry::statusChar(entry.worktreeStatus));
            item->setText(1, entry.path);
            item->setData(0, Qt::UserRole, entry.path);
            item->setData(0, Qt::UserRole + 1, false);
            item->setTextAlignment(0, Qt::AlignCenter);
            
            QColor bgColor = QColor("#238636");
            if (entry.worktreeStatus == FileStatusEntry::Deleted) bgColor = QColor("#DA3633");
            else if (entry.worktreeStatus == FileStatusEntry::Modified) bgColor = QColor("#007ACC");
            else if (entry.worktreeStatus == FileStatusEntry::Renamed) bgColor = QColor("#8957E5");
            else if (entry.worktreeStatus == FileStatusEntry::Untracked) bgColor = QColor("#7A7A7A");
            
            item->setBackground(0, bgColor);
            item->setForeground(0, Qt::white);
        }
    }

    m_stagedRoot->setText(1, QStringLiteral("Staged Changes (%1)").arg(m_stagedRoot->childCount()));
    m_unstagedRoot->setText(1, QStringLiteral("Unstaged Changes (%1)").arg(m_unstagedRoot->childCount()));
}

void RepoWidget::updateCommitLog()
{
    m_logModel->setCommits(m_git->getLog(200));
}

void RepoWidget::updateBranchesTree()
{
    m_branchesTree->clear();
    QVector<BranchInfo> branches = m_git->getBranches();

    QTreeWidgetItem *localRoot = new QTreeWidgetItem(m_branchesTree, {QStringLiteral("Local Branches")});
    localRoot->setExpanded(true);
    QTreeWidgetItem *remoteRoot = new QTreeWidgetItem(m_branchesTree, {QStringLiteral("Remote Branches")});
    remoteRoot->setExpanded(true);

    QString currentBranch;
    QStringList branchNames;

    for (const auto &bi : branches) {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        QString display = bi.name;
        if (bi.isHead) {
            display = QStringLiteral("? %1").arg(bi.name);
            item->setData(0, Qt::FontRole, QFont(m_branchesTree->font().family(), -1, QFont::Bold));
        }
        item->setText(0, display);
        item->setData(0, Qt::UserRole, bi.name);

        if (bi.isRemote) {
            remoteRoot->addChild(item);
        } else {
            localRoot->addChild(item);
            branchNames.append(bi.name);
            if (bi.isHead) {
                currentBranch = bi.name;
            }
        }
    }
    
    emit branchListChanged(branchNames, currentBranch);
}

    // Show diff for item is now handled directly via onCommitFileClicked

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Commit Graph Selection & Diff (Faz 5)
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

void RepoWidget::onCommitSelected(const QItemSelection &selected, const QItemSelection &deselected)
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

void RepoWidget::onCommitFileClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return; // Ignored root items
    
    static const QStringList binExts = {"pdf", "png", "jpg", "jpeg", "gif", "exe", "dll", "so", "bin", "zip", "tar", "gz", "mp4", "mp3", "obj", "o", "a", "lib", "ttf", "woff", "hex", "elf", "map", "out"};
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

void RepoWidget::onFileItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
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

void RepoWidget::stageSelected()
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

void RepoWidget::unstageSelected()
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

void RepoWidget::stageAll()
{
    m_git->stageAll();
    refreshAll();
}

void RepoWidget::unstageAll()
{
    m_git->unstageAll();
    refreshAll();
}

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Commit
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

void RepoWidget::doCommit()
{
    QString msg = m_commitEdit->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Commit"),
            QStringLiteral("Please enter a commit message."));
        return;
    }

    bool amend = m_amendCheck->isChecked();

    if (m_git->commit(msg, amend)) {
        m_commitEdit->clear();
        m_amendCheck->setChecked(false);
        refreshAll();
        emit statusMessage(amend ? QStringLiteral("Amend commit successful!") : QStringLiteral("Commit successful!"));
        
        if (m_pushAfterCommitCheck->isChecked()) {
            doPush();
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Stash
// ═══════════════════════════════════════════════════════════════════

void RepoWidget::doStashSave()
{
    bool ok;
    QString msg = QInputDialog::getText(this, "Stash Changes", 
        "Enter stash message (optional):", QLineEdit::Normal, "", &ok);
        
    if (!ok) return;

    if (m_git->stashSave(msg)) {
        refreshAll();
        QMessageBox::information(this, "Stash", "Changes have been stashed successfully.");
    } else {
        QMessageBox::critical(this, "Error", m_git->lastError());
    }
}

void RepoWidget::doStashPop()
{
    if (QMessageBox::question(this, "Pop Stash", "Pop the latest stash?") == QMessageBox::Yes) {
        if (m_git->stashPop()) {
            refreshAll();
            QMessageBox::information(this, "Pop Stash", "Stash popped successfully.");
        } else {
            QMessageBox::critical(this, "Error", m_git->lastError());
        }
    }
}

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Push / Pull
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

void RepoWidget::doPush()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Pushing to remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->push()) {
        emit statusMessage(QStringLiteral("Sync: Pushed at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully pushed to remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Push failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
    }
}

void RepoWidget::doPull()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Pulling from remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->pull()) {
        emit statusMessage(QStringLiteral("Sync: Pulled at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully pulled from remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Pull failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
    }
}

void RepoWidget::doFetch()
{
    if (!m_git->isOpen()) return;
    QProgressDialog progress("Fetching from remote...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    if (m_git->fetch()) {
        emit statusMessage(QStringLiteral("Sync: Fetched at %1").arg(QTime::currentTime().toString("HH:mm:ss")));
        QMessageBox::information(this, "Success", "Successfully fetched from remote.");
        refreshAll();
    } else {
        QMessageBox::critical(this, "Error", QStringLiteral("Fetch failed:\n%1\n\nPlease check your Credentials.").arg(m_git->lastError()));
    }
}



// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Branch operations
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

void RepoWidget::onBranchItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || item->parent() == nullptr) return; // Ignore clicks on Root items

    QString branchName = item->data(0, Qt::UserRole).toString();
    if (branchName.isEmpty()) return;

    QString currentBranch = m_git->getCurrentBranch();
    if (branchName == currentBranch) return;

    if (m_git->checkoutBranch(branchName)) {
        emit statusMessage(
            QStringLiteral("Switched to branch '%1'").arg(branchName));
        refreshAll();
    } else {
        QMessageBox::warning(this, QStringLiteral("Checkout Failed"),
                             QStringLiteral("Failed to checkout branch. Do you have uncommitted changes?"));
    }
}

void RepoWidget::createBranch()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, QStringLiteral("Create Branch"),
        QStringLiteral("Branch name:"),
        QLineEdit::Normal, QString(), &ok);

    if (ok && !name.isEmpty()) {
        if (m_git->createBranch(name)) {
            emit statusMessage(
                QStringLiteral("Branch '%1' created").arg(name));
            refreshAll();
        }
    }
}

void RepoWidget::startGitFlowBranch(const QString &prefix)
{
    BranchDialog dlg(prefix, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString branchName = dlg.branchName();
        if (branchName.isEmpty()) return;

        if (m_git->createBranch(branchName)) {
            emit statusMessage(QStringLiteral("Branch '%1' created").arg(branchName));
            if (dlg.shouldCheckout()) {
                if (m_git->checkoutBranch(branchName)) {
                    emit statusMessage(QStringLiteral("Switched to branch '%1'").arg(branchName));
                } else {
                    QMessageBox::warning(this, "Checkout Failed", "Branch created but checkout failed. Commit your changes first.");
                }
            }
            refreshAll();
        }
    }
}

void RepoWidget::finishGitFlowBranch()
{
    QString currentBranch = m_git->getCurrentBranch();
    if (currentBranch.isEmpty() || currentBranch == "main" || currentBranch == "master") {
        QMessageBox::information(this, "Finish Branch", "You must be on a feature, bugfix, release, or hotfix branch to finish it.");
        return;
    }

    // Find main or master
    QVector<BranchInfo> branches = m_git->getBranches();
    QString targetBranch = "";
    for (const auto &b : branches) {
        if (b.name == "main") {
            targetBranch = "main";
            break;
        } else if (b.name == "master") {
            targetBranch = "master";
        }
    }

    if (targetBranch.isEmpty()) {
        QMessageBox::warning(this, "Finish Branch", "Could not find 'main' or 'master' branch to merge into.");
        return;
    }

    int ret = QMessageBox::question(this, "Finish Branch", 
        QString("Finish branch '%1'?\n\nThis will merge it into '%2' and then delete '%1'.").arg(currentBranch, targetBranch),
        QMessageBox::Yes | QMessageBox::No);
        
    if (ret == QMessageBox::Yes) {
        if (!m_git->checkoutBranch(targetBranch)) {
            QMessageBox::warning(this, "Checkout Failed", QString("Could not checkout '%1'. Do you have uncommitted changes?").arg(targetBranch));
            return;
        }

        if (m_git->mergeBranch(currentBranch)) {
            if (m_git->deleteBranch(currentBranch)) {
                emit statusMessage(QStringLiteral("Successfully finished and deleted '%1'").arg(currentBranch));
            } else {
                emit statusMessage(QStringLiteral("Merged '%1', but failed to delete it.").arg(currentBranch));
            }
        } else {
            QMessageBox::warning(this, "Merge Failed", "Merge failed or resulted in conflicts. The branch was not deleted.");
        }
        refreshAll();
    }
}

void RepoWidget::deleteBranch()
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
                emit statusMessage(
                    QStringLiteral("Branch '%1' deleted").arg(name));
                refreshAll();
            }
        }
    }
}

void RepoWidget::mergeBranch()
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
            emit statusMessage(
                QStringLiteral("Merge successful!"));
            refreshAll();
        }
    }
}

// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ
//  Misc
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââââ



void RepoWidget::onRepoChanged(const QString & /*path*/)
{
    // Debounce: refresh after a short delay to avoid rapid re-reads
    QTimer::singleShot(500, this, &RepoWidget::refreshAll);
}


void RepoWidget::showBranchContextMenu(const QPoint &pos)
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

void RepoWidget::showLocalFilesContextMenu(const QPoint &pos)
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

void RepoWidget::showCommitContextMenu(const QPoint &pos)
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

void RepoWidget::showDirTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = m_dirTree->indexAt(pos);
    if (!index.isValid()) return;

    QString absPath = m_dirModel->filePath(index);
    if (absPath.isEmpty()) return;

    QString repoPath = m_git->repoPath();
    QString relPath = QDir(repoPath).relativeFilePath(absPath);

    QMenu menu(this);
    
    QFileInfo fi(absPath);
    bool isDir = fi.isDir();
    QString fileName = fi.fileName();
    QString ext = fi.suffix();

    QAction *actIgnoreExact = menu.addAction(QStringLiteral("Ignore exact file (%1)").arg(fileName));
    QAction *actIgnoreExt = nullptr;
    if (!isDir && !ext.isEmpty()) {
        actIgnoreExt = menu.addAction(QStringLiteral("Ignore all .%1 files").arg(ext));
    }
    QAction *actIgnoreFolder = nullptr;
    if (isDir) {
        actIgnoreFolder = menu.addAction(QStringLiteral("Ignore folder (%1/)").arg(fileName));
    }

    menu.addSeparator();
    QAction *actOpenDir = menu.addAction(QStringLiteral("Open in Explorer"));

    QAction *res = menu.exec(m_dirTree->viewport()->mapToGlobal(pos));
    
    QString ignoreStr;
    if (res == actIgnoreExact) ignoreStr = relPath;
    else if (res && res == actIgnoreExt) ignoreStr = "*." + ext;
    else if (res && res == actIgnoreFolder) ignoreStr = relPath + "/";

    if (!ignoreStr.isEmpty()) {
        if (m_git->addToGitignore(ignoreStr)) refreshAll();
        else QMessageBox::critical(this, "Error", m_git->lastError());
    } else if (res == actOpenDir) {
        if (isDir) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(absPath));
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
        }
    }
}















