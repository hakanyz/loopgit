#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemSelection>

class QTreeWidget;
class QTreeWidgetItem;
class QTableView;
class QTextEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QSplitter;
class QFileSystemWatcher;
class QAction;
class QToolBar;

class GitManager;
class DiffViewWidget;
class CommitGraphModel;
class CommitGraphDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // ── Repository ──────────────────────────────────────
    void openRepository();
    void closeRepository();
    void refreshAll();

    // ── File list ───────────────────────────────────────
    void onFileItemClicked(QTreeWidgetItem *item, int column);
    void onFileItemDoubleClicked(QTreeWidgetItem *item, int column);

    // ── Stage / Unstage ─────────────────────────────────
    void stageSelected();
    void unstageSelected();
    void stageAll();
    void unstageAll();

    // ── Commit ──────────────────────────────────────────
    void doCommit();

    // ── Push / Pull ─────────────────────────────────────
    void doPush();
    void doPull();

    // ── Branch ──────────────────────────────────────────
    void onBranchChanged(int index);
    void createBranch();
    void deleteBranch();
    void mergeBranch();

    // ── Commit Graph and Commit Diff ────────────────────
    void onCommitSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void onCommitFileClicked(QTreeWidgetItem *item, int column);

    // ── Misc ────────────────────────────────────────────
    void onRepoChanged(const QString &path);
    void onGitError(const QString &message);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void connectSignals();
    void applyDarkTheme();

    void updateFileList();
    void updateCommitLog();
    void updateBranchCombo();
    void showDiffForItem(QTreeWidgetItem *item);
    void setRepoActionsEnabled(bool enabled);

    // ── Members ─────────────────────────────────────────
    GitManager     *m_git;

    // UI widgets
    QTreeWidget    *m_fileTree;
    QTreeWidget    *m_commitFilesTree;
    QTreeWidgetItem *m_stagedRoot;
    QTreeWidgetItem *m_unstagedRoot;
    DiffViewWidget *m_diffView;
    QTableView     *m_logTable;
    CommitGraphModel *m_logModel;
    CommitGraphDelegate *m_logDelegate;
    QTextEdit      *m_commitEdit;
    QPushButton    *m_commitBtn;
    QComboBox      *m_branchCombo;
    QLabel         *m_statusLabel;
    QToolBar       *m_toolBar;
    QSplitter      *m_mainSplitter;
    QSplitter      *m_bottomSplitter;
    QSplitter      *m_rightSplitter;

    QString        m_selectedCommitId;

    // Actions
    QAction *m_actOpen;
    QAction *m_actClose;
    QAction *m_actRefresh;
    QAction *m_actPush;
    QAction *m_actPull;
    QAction *m_actStageAll;
    QAction *m_actUnstageAll;
    QAction *m_actNewBranch;
    QAction *m_actDeleteBranch;
    QAction *m_actMergeBranch;

    // File system watcher for auto-refresh
    QFileSystemWatcher *m_watcher;

    // Track whether we're programmatically updating the branch combo
    bool m_updatingBranch = false;
};

#endif // MAINWINDOW_H
