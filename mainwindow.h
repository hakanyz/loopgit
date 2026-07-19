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
class QStackedWidget;
class QTreeView;
class QFileSystemModel;

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
    void onFileItemDoubleClicked(QTreeWidgetItem *item, int column);

    // ── Stage / Unstage ─────────────────────────────────
    void stageSelected();
    void unstageSelected();
    void stageAll();
    void unstageAll();

    // ── Commit ──────────────────────────────────────────
    void doCommit();

    // ── Perspectives ────────────────────────────────────
    void switchPerspective(int index);

    // ── Push / Pull ─────────────────────────────────────
    void doPush();
    void doPull();
    void doFetch();

    // ── Branch ──────────────────────────────────────────
    void onBranchItemDoubleClicked(QTreeWidgetItem *item, int column);
    void createBranch();
    void deleteBranch();
    void mergeBranch();

    void cloneRepository();
    void openCredentials();
    void showAboutDialog();

    // ── Commit Graph and Commit Diff ────────────────────
    void onCommitSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void onCommitFileClicked(QTreeWidgetItem *item, int column);
    void showCommitContextMenu(const QPoint &pos);
    void showBranchContextMenu(const QPoint &pos);
    void showLocalFilesContextMenu(const QPoint &pos);

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
    void updateBranchesTree();
    void showDiffForItem(QTreeWidgetItem *item);
    void setRepoActionsEnabled(bool enabled);

    // ── Members ─────────────────────────────────────────
    GitManager     *m_git;

    // Perspectives
    QStackedWidget *m_stackedWidget;
    QAction        *m_actLocalFiles;
    QAction        *m_actHistory;

    // Local Files Perspective
    QTreeView      *m_dirTree;
    QFileSystemModel *m_dirModel;
    QTreeWidget    *m_localChangesTree;
    QTreeWidgetItem *m_stagedRoot;
    QTreeWidgetItem *m_unstagedRoot;
    QWidget        *m_commitPanelWidget;
    DiffViewWidget *m_localDiffView;

    // History Perspective
    QTreeWidget    *m_branchesTree;
    QTreeWidget    *m_historyFilesTree;
    DiffViewWidget *m_historyDiffView;
    QTableView     *m_logTable;
    CommitGraphModel *m_logModel;
    CommitGraphDelegate *m_logDelegate;
    QLabel         *m_commitDetailsLabel;

    QTextEdit      *m_commitEdit;
    QPushButton    *m_commitBtn;
    QLabel         *m_statusLabel;
    QToolBar       *m_toolBar;
    QSplitter      *m_mainSplitter;
    QSplitter      *m_leftSplitter;
    QSplitter      *m_centerTopSplitter;
    QSplitter      *m_rightSplitter;

    QString        m_selectedCommitId;

    // Actions
    QAction *m_actOpen;
    QAction *m_actClone;
    QAction *m_actClose;
    QAction *m_actCredentials;
    QAction *m_actRefresh;
    QAction *m_actStageAll;
    QAction *m_actUnstageAll;
    QAction *m_actPush;
    QAction *m_actPull;
    QAction *m_actFetch;
    QAction *m_actNewBranch;
    QAction *m_actDeleteBranch;
    QAction *m_actMergeBranch;

    // File system watcher for auto-refresh
    QFileSystemWatcher *m_watcher;

    // Track whether we're programmatically updating the branch combo
    bool m_updatingBranch = false;
};

#endif // MAINWINDOW_H
