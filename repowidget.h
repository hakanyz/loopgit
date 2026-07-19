#ifndef RepoWidget_H
#define RepoWidget_H

#include <QWidget>
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
class QCheckBox;

class GitManager;
class DiffViewWidget;
class CommitGraphModel;
class CommitGraphDelegate;

class RepoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RepoWidget(const QString &repoPath, QWidget *parent = nullptr);
    ~RepoWidget() override;

    QString repoPath() const { return m_repoPath; }

public slots:
    void refreshAll();
    void switchPerspective(int index);
    void doPush();
    void doPull();
    void doFetch();
    void startGitFlowBranch(const QString &prefix);
    void finishGitFlowBranch();

private slots:
    // ── File list ───────────────────────────────────────
    void onFileItemDoubleClicked(QTreeWidgetItem *item, int column);

    // ── Stage / Unstage ─────────────────────────────────
    void stageSelected();
    void unstageSelected();
    void stageAll();
    void unstageAll();

    // ── Commit ──────────────────────────────────────────
    void doCommit();

    // ── Stash ───────────────────────────────────────────
    void doStashSave();
    void doStashPop();

    // ── Branch ──────────────────────────────────────────
    void onBranchItemDoubleClicked(QTreeWidgetItem *item, int column);
    void createBranch();
    void deleteBranch();
    void mergeBranch();

    // ── Commit Graph and Commit Diff ────────────────────
    void onCommitSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void onCommitFileClicked(QTreeWidgetItem *item, int column);
    void showCommitContextMenu(const QPoint &pos);
    void showBranchContextMenu(const QPoint &pos);
    void showLocalFilesContextMenu(const QPoint &pos);
    void showDirTreeContextMenu(const QPoint &pos);

    // ── Misc ────────────────────────────────────────────
    void onRepoChanged(const QString &path);

signals:
    void errorOccurred(const QString &message);
    void branchListChanged(const QStringList &branches, const QString &currentBranch);
    void statusMessage(const QString &msg);

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
    QCheckBox      *m_amendCheck;
    QCheckBox      *m_pushAfterCommitCheck;
    QSplitter      *m_mainSplitter;
    QSplitter      *m_leftSplitter;
    QSplitter      *m_centerTopSplitter;
    QSplitter      *m_rightSplitter;

    QString        m_selectedCommitId;

    // File system watcher for auto-refresh
    QFileSystemWatcher *m_watcher;
    QString m_repoPath;
};

#endif // RepoWidget_H


