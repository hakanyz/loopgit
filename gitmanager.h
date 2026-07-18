#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

// Forward declaration — git2.h is only included in gitmanager.cpp
struct git_repository;

// ─── Data structures ───────────────────────────────────────────────

struct FileStatusEntry {
    QString path;
    QString oldPath;  // only set for renames

    enum Status {
        Unmodified = 0,
        Added,
        Modified,
        Deleted,
        Renamed,
        Typechange,
        Untracked,
        Ignored,
        Conflicted
    };

    Status indexStatus    = Unmodified;   // staged status
    Status worktreeStatus = Unmodified;   // unstaged status

    bool hasIndexChanges()    const { return indexStatus    != Unmodified; }
    bool hasWorktreeChanges() const { return worktreeStatus != Unmodified; }

    static QChar statusChar(Status s) {
        switch (s) {
            case Added:      return 'A';
            case Modified:   return 'M';
            case Deleted:    return 'D';
            case Renamed:    return 'R';
            case Typechange: return 'T';
            case Untracked:  return '?';
            case Conflicted: return 'C';
            case Ignored:    return '!';
            default:         return ' ';
        }
    }

    static QString statusString(Status s) {
        switch (s) {
            case Added:      return QStringLiteral("Added");
            case Modified:   return QStringLiteral("Modified");
            case Deleted:    return QStringLiteral("Deleted");
            case Renamed:    return QStringLiteral("Renamed");
            case Typechange: return QStringLiteral("Typechange");
            case Untracked:  return QStringLiteral("Untracked");
            case Conflicted: return QStringLiteral("Conflicted");
            case Ignored:    return QStringLiteral("Ignored");
            default:         return QStringLiteral("Unmodified");
        }
    }
};

struct CommitInfo {
    QString  id;           // full SHA-1 hex
    QString  shortId;      // first 7 chars
    QString  message;      // full commit message
    QString  summary;      // first line only
    QString  authorName;
    QString  authorEmail;
    QDateTime date;
    QStringList parentIds; // full SHA-1 hex of parents
    QStringList refs;      // branch names pointing to this commit
};

struct BranchInfo {
    QString name;
    bool isRemote = false;
    bool isHead   = false;
    QString targetHash;
};

// ─── GitManager ────────────────────────────────────────────────────

class GitManager : public QObject
{
    Q_OBJECT

public:
    explicit GitManager(QObject *parent = nullptr);
    ~GitManager() override;

    /* ── Global init / shutdown ──────────────────────── */
    static bool  initialize();
    static void  shutdown();

    /* ── Repository (Faz 0) ──────────────────────────── */
    bool    openRepository(const QString &path);
    void    closeRepository();
    bool    isOpen()    const;
    QString repoPath()  const;

    /* ── Status & Log (Faz 1) ────────────────────────── */
    QVector<FileStatusEntry> getFileStatus();
    QString                  getCurrentBranch();
    QVector<CommitInfo>      getLog(int maxCount = 100);

    /* ── Stage / Unstage / Commit (Faz 2) ────────────── */
    bool stageFile(const QString &path);
    bool unstageFile(const QString &path);
    bool stageAll();
    bool unstageAll();
    bool commit(const QString &message);

    /* ── Push / Pull (Faz 2) ─────────────────────────── */
    bool push(const QString &remoteName = QStringLiteral("origin"));
    bool pull(const QString &remoteName = QStringLiteral("origin"));

    /* ── Diff (Faz 3) ────────────────────────────────── */
    QString getWorkdirDiff(const QString &filePath = QString());
    QString getStagedDiff(const QString &filePath = QString());
    
    /* ── Commit Details (Faz 5) ──────────────────────── */
    QVector<FileStatusEntry> getCommitChangedFiles(const QString &commitId);
    QString getCommitDiff(const QString &commitId, const QString &filePath = QString());

    /* ── Branch management (Faz 4) ───────────────────── */
    QVector<BranchInfo> getBranches();
    bool createBranch(const QString &name);
    bool createBranchAt(const QString &name, const QString &commitId);
    bool checkoutBranch(const QString &name);
    bool mergeBranch(const QString &name);
    bool deleteBranch(const QString &name);

    /* ── Error handling ──────────────────────────────── */
    QString lastError() const;

signals:
    void repositoryOpened(const QString &path);
    void repositoryClosed();
    void errorOccurred(const QString &message);

private:
    git_repository *m_repo     = nullptr;
    QString         m_repoPath;
    QString         m_lastError;

    void setError(const QString &context);
    bool ensureOpen();

    // libgit2 credential callback (static, used from push/pull)
    static int credentialCb(void *out, const char *url,
                            const char *usernameFromUrl,
                            unsigned int allowedTypes, void *payload);
};

#endif // GITMANAGER_H
