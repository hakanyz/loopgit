#include "gitmanager.h"
#include <git2.h>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>

// ═══════════════════════════════════════════════════════════════════
//  Helper: convert git_delta_t → FileStatusEntry::Status
// ═══════════════════════════════════════════════════════════════════

static FileStatusEntry::Status deltaToStatus(git_delta_t delta)
{
    switch (delta) {
    case GIT_DELTA_ADDED:      return FileStatusEntry::Added;
    case GIT_DELTA_DELETED:    return FileStatusEntry::Deleted;
    case GIT_DELTA_MODIFIED:   return FileStatusEntry::Modified;
    case GIT_DELTA_RENAMED:    return FileStatusEntry::Renamed;
    case GIT_DELTA_TYPECHANGE: return FileStatusEntry::Typechange;
    case GIT_DELTA_CONFLICTED: return FileStatusEntry::Conflicted;
    default:                   return FileStatusEntry::Unmodified;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Diff print callback (captures unified diff text into a QString)
// ═══════════════════════════════════════════════════════════════════

static int diffPrintCb(const git_diff_delta * /*delta*/,
                       const git_diff_hunk  * /*hunk*/,
                       const git_diff_line  *line,
                       void                 *payload)
{
    QString *output = static_cast<QString *>(payload);

    // "Professional Touch": Prevent freezing on massive diffs (e.g. huge text/hex files)
    const int MAX_DIFF_LENGTH = 100 * 1024; // 100 KB text limit per diff view
    if (output->length() > MAX_DIFF_LENGTH) {
        if (!output->endsWith("\n--- Diff output truncated (file too large) ---\n")) {
            output->append("\n--- Diff output truncated (file too large) ---\n");
        }
        return -1; // Return non-zero to abort further iteration and save performance
    }

    // Prefix character for the line origin
    if (line->origin == GIT_DIFF_LINE_ADDITION  ||
        line->origin == GIT_DIFF_LINE_DELETION  ||
        line->origin == GIT_DIFF_LINE_CONTEXT)
    {
        output->append(QChar(line->origin));
    }

    output->append(QString::fromUtf8(line->content,
                                     static_cast<int>(line->content_len)));
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
//  Construction / Destruction
// ═══════════════════════════════════════════════════════════════════

GitManager::GitManager(QObject *parent)
    : QObject(parent)
{}

GitManager::~GitManager()
{
    closeRepository();
}

// ═══════════════════════════════════════════════════════════════════
//  Global init / shutdown
// ═══════════════════════════════════════════════════════════════════

bool GitManager::initialize()
{
    return git_libgit2_init() >= 0;
}

void GitManager::shutdown()
{
    git_libgit2_shutdown();
}

// ═══════════════════════════════════════════════════════════════════
//  Error handling
// ═══════════════════════════════════════════════════════════════════

void GitManager::setError(const QString &context)
{
    const git_error *err = git_error_last();
    if (err && err->message) {
        m_lastError = QString("%1: %2").arg(context, QString::fromUtf8(err->message));
    } else {
        m_lastError = context;
    }
    emit errorOccurred(m_lastError);
}

bool GitManager::ensureOpen()
{
    if (!m_repo) {
        m_lastError = QStringLiteral("No repository is open");
        emit errorOccurred(m_lastError);
        return false;
    }
    return true;
}

QString GitManager::lastError() const
{
    return m_lastError;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 0 — Repository open / close
// ═══════════════════════════════════════════════════════════════════

bool GitManager::openRepository(const QString &path)
{
    closeRepository();

    QByteArray pathUtf8 = QDir::toNativeSeparators(path).toUtf8();
    int err = git_repository_open_ext(&m_repo, pathUtf8.constData(),
                                      0, nullptr);
    if (err < 0) {
        setError(QStringLiteral("Failed to open repository"));
        m_repo = nullptr;
        return false;
    }

    // Resolve the actual workdir (might differ from the selected folder)
    const char *workdir = git_repository_workdir(m_repo);
    m_repoPath = workdir ? QDir::cleanPath(QString::fromUtf8(workdir))
                         : path;

    emit repositoryOpened(m_repoPath);
    return true;
}

void GitManager::closeRepository()
{
    if (m_repo) {
        git_repository_free(m_repo);
        m_repo = nullptr;
        m_repoPath.clear();
        emit repositoryClosed();
    }
}

bool GitManager::isOpen() const { return m_repo != nullptr; }
QString GitManager::repoPath() const { return m_repoPath; }

// ═══════════════════════════════════════════════════════════════════
//  Faz 1 — Status
// ═══════════════════════════════════════════════════════════════════

QVector<FileStatusEntry> GitManager::getFileStatus()
{
    QVector<FileStatusEntry> result;
    if (!ensureOpen()) return result;

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED
               | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
               | GIT_STATUS_OPT_RENAMES_INDEX_TO_WORKDIR
               | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    git_status_list *statusList = nullptr;
    if (git_status_list_new(&statusList, m_repo, &opts) < 0) {
        setError(QStringLiteral("Failed to get status"));
        return result;
    }

    size_t count = git_status_list_entrycount(statusList);
    result.reserve(static_cast<int>(count));

    for (size_t i = 0; i < count; ++i) {
        const git_status_entry *entry = git_status_byindex(statusList, i);
        if (!entry) continue;

        // Skip clean files
        if (entry->status == GIT_STATUS_CURRENT) continue;

        FileStatusEntry fse;

        // ── Staged (index) changes ──────────────────────
        if (entry->head_to_index) {
            fse.indexStatus = deltaToStatus(entry->head_to_index->status);
            fse.path = QString::fromUtf8(entry->head_to_index->new_file.path);
            if (entry->head_to_index->status == GIT_DELTA_RENAMED)
                fse.oldPath = QString::fromUtf8(entry->head_to_index->old_file.path);
        }

        // ── Unstaged (work-tree) changes ────────────────
        if (entry->index_to_workdir) {
            fse.worktreeStatus = deltaToStatus(entry->index_to_workdir->status);
            // Prefer workdir path if index path is empty
            if (fse.path.isEmpty())
                fse.path = QString::fromUtf8(entry->index_to_workdir->new_file.path);
            if (entry->index_to_workdir->status == GIT_DELTA_RENAMED && fse.oldPath.isEmpty())
                fse.oldPath = QString::fromUtf8(entry->index_to_workdir->old_file.path);
        }

        // Map WT_NEW (untracked) properly
        if (entry->status & GIT_STATUS_WT_NEW)
            fse.worktreeStatus = FileStatusEntry::Untracked;

        // Conflict
        if (entry->status & GIT_STATUS_CONFLICTED) {
            fse.indexStatus    = FileStatusEntry::Conflicted;
            fse.worktreeStatus = FileStatusEntry::Conflicted;
        }

        if (!fse.path.isEmpty())
            result.append(fse);
    }

    git_status_list_free(statusList);
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 1 — Current branch
// ═══════════════════════════════════════════════════════════════════

QString GitManager::getCurrentBranch()
{
    if (!ensureOpen()) return QString();

    git_reference *head = nullptr;
    int err = git_repository_head(&head, m_repo);

    if (err == GIT_EUNBORNBRANCH)
        return QStringLiteral("(no commits yet)");
    if (err < 0) {
        setError(QStringLiteral("Failed to get HEAD"));
        return QString();
    }

    if (git_reference_is_branch(head)) {
        const char *name = nullptr;
        git_branch_name(&name, head);
        QString branchName = QString::fromUtf8(name);
        git_reference_free(head);
        return branchName;
    }

    // Detached HEAD — show short OID
    const git_oid *oid = git_reference_target(head);
    char buf[12];
    git_oid_tostr(buf, sizeof(buf), oid);
    git_reference_free(head);
    return QStringLiteral("HEAD detached at %1").arg(QString::fromUtf8(buf));
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 1 — Commit log
// ═══════════════════════════════════════════════════════════════════

QVector<CommitInfo> GitManager::getLog(int maxCount)
{
    QVector<CommitInfo> result;
    if (!ensureOpen()) return result;

    git_revwalk *walker = nullptr;
    if (git_revwalk_new(&walker, m_repo) < 0) {
        setError(QStringLiteral("Failed to create revwalk"));
        return result;
    }

    git_revwalk_sorting(walker, GIT_SORT_TIME);

    if (git_revwalk_push_head(walker) < 0) {
        // Probably no commits yet
        git_revwalk_free(walker);
        return result;
    }

    git_oid oid;
    int count = 0;
    result.reserve(maxCount);

    QVector<BranchInfo> branches = getBranches();
    QVector<BranchInfo> tags = getTags();
    QHash<QString, QStringList> commitToRefs;
    
    for (const auto &b : branches) {
        if (!b.targetHash.isEmpty()) {
            commitToRefs[b.targetHash].append(b.name);
        }
    }
    for (const auto &t : tags) {
        if (!t.targetHash.isEmpty()) {
            commitToRefs[t.targetHash].append(t.name);
        }
    }

    while (git_revwalk_next(&oid, walker) == 0 && count < maxCount) {
        git_commit *commit = nullptr;
        if (git_commit_lookup(&commit, m_repo, &oid) != 0)
            continue;

        CommitInfo ci;
        char hashBuf[41];
        git_oid_tostr(hashBuf, sizeof(hashBuf), &oid);
        ci.id      = QString::fromUtf8(hashBuf);
        ci.shortId = ci.id.left(7);
        ci.message = QString::fromUtf8(git_commit_message(commit));
        ci.summary = QString::fromUtf8(git_commit_summary(commit));

        const git_signature *author = git_commit_author(commit);
        if (author) {
            ci.authorName  = QString::fromUtf8(author->name);
            ci.authorEmail = QString::fromUtf8(author->email);
            ci.date        = QDateTime::fromSecsSinceEpoch(
                                 author->when.time,
                                 QTimeZone(author->when.offset * 60));
        }

        unsigned int parentCount = git_commit_parentcount(commit);
        for (unsigned int i = 0; i < parentCount; ++i) {
            const git_oid *poid = git_commit_parent_id(commit, i);
            char pHashBuf[41];
            git_oid_tostr(pHashBuf, sizeof(pHashBuf), poid);
            ci.parentIds.append(QString::fromUtf8(pHashBuf));
        }

        ci.refs = commitToRefs.value(ci.id);

        result.append(ci);
        git_commit_free(commit);
        ++count;
    }

    git_revwalk_free(walker);

    // Returning only actual git commits now
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Stage / Unstage
// ═══════════════════════════════════════════════════════════════════

bool GitManager::stageFile(const QString &path)
{
    if (!ensureOpen()) return false;

    git_index *index = nullptr;
    if (git_repository_index(&index, m_repo) < 0) {
        setError(QStringLiteral("Failed to get index"));
        return false;
    }

    QByteArray pathBytes = path.toUtf8();

    // Check if the file was deleted in workdir
    QString fullPath = m_repoPath + "/" + path;
    if (!QFileInfo::exists(fullPath)) {
        // File was deleted — remove from index
        if (git_index_remove_bypath(index, pathBytes.constData()) < 0) {
            setError(QStringLiteral("Failed to stage deleted file"));
            git_index_free(index);
            return false;
        }
    } else {
        if (git_index_add_bypath(index, pathBytes.constData()) < 0) {
            setError(QStringLiteral("Failed to stage file"));
            git_index_free(index);
            return false;
        }
    }

    git_index_write(index);
    git_index_free(index);
    return true;
}

bool GitManager::stageHunk(const QString &patchText)
{
    if (!ensureOpen() || patchText.isEmpty()) return false;

    git_diff *diff = nullptr;
    int err = git_diff_from_buffer(&diff, patchText.toUtf8().constData(), patchText.toUtf8().length());
    if (err < 0) {
        setError("Failed to parse hunk patch");
        return false;
    }

    err = git_apply(m_repo, diff, GIT_APPLY_LOCATION_INDEX, nullptr);
    git_diff_free(diff);

    if (err < 0) {
        setError("Failed to apply hunk to index (might already be staged or conflict)");
        return false;
    }

    // git_apply with GIT_APPLY_LOCATION_INDEX updates the index on disk directly if it succeeds.
    // wait, actually we might need to manually git_index_write if git_apply doesn't do it?
    // git_apply usually writes it. Just to be safe:
    git_index *index = nullptr;
    if (git_repository_index(&index, m_repo) == 0) {
        git_index_write(index);
        git_index_free(index);
    }

    return true;
}

bool GitManager::unstageFile(const QString &path)
{
    if (!ensureOpen()) return false;

    git_reference *head = nullptr;
    int err = git_repository_head(&head, m_repo);

    if (err == GIT_EUNBORNBRANCH) {
        // No commits — remove from index entirely
        git_index *index = nullptr;
        if (git_repository_index(&index, m_repo) < 0) {
            setError(QStringLiteral("Failed to get index"));
            return false;
        }
        git_index_remove_bypath(index, path.toUtf8().constData());
        git_index_write(index);
        git_index_free(index);
        return true;
    }

    if (err < 0) {
        setError(QStringLiteral("Failed to get HEAD"));
        return false;
    }

    git_object *headObj = nullptr;
    git_reference_peel(&headObj, head, GIT_OBJECT_COMMIT);

    QByteArray pathBytes = path.toUtf8();
    char *paths[] = { pathBytes.data() };
    git_strarray pathspec = { paths, 1 };

    err = git_reset_default(m_repo, headObj, &pathspec);

    git_object_free(headObj);
    git_reference_free(head);

    if (err < 0) {
        setError(QStringLiteral("Failed to unstage file"));
        return false;
    }
    return true;
}

bool GitManager::stageAll()
{
    if (!ensureOpen()) return false;

    git_index *index = nullptr;
    if (git_repository_index(&index, m_repo) < 0) {
        setError(QStringLiteral("Failed to get index"));
        return false;
    }

    // Add all changes (including untracked)
    char *matchAll[] = { (char *)"*" };
    git_strarray pathspec = { matchAll, 1 };

    if (git_index_add_all(index, &pathspec,
                          GIT_INDEX_ADD_DEFAULT, nullptr, nullptr) < 0)
    {
        setError(QStringLiteral("Failed to stage all"));
        git_index_free(index);
        return false;
    }

    // Also update deleted files
    git_index_update_all(index, &pathspec, nullptr, nullptr);

    git_index_write(index);
    git_index_free(index);
    return true;
}

bool GitManager::unstageAll()
{
    if (!ensureOpen()) return false;

    git_reference *head = nullptr;
    int err = git_repository_head(&head, m_repo);

    if (err == GIT_EUNBORNBRANCH) {
        // No commits — clear the entire index
        git_index *index = nullptr;
        git_repository_index(&index, m_repo);
        git_index_clear(index);
        git_index_write(index);
        git_index_free(index);
        return true;
    }

    if (err < 0) {
        setError(QStringLiteral("Failed to get HEAD"));
        return false;
    }

    git_object *headObj = nullptr;
    git_reference_peel(&headObj, head, GIT_OBJECT_COMMIT);

    // Reset all paths
    err = git_reset_default(m_repo, headObj, nullptr);

    git_object_free(headObj);
    git_reference_free(head);

    if (err < 0) {
        setError(QStringLiteral("Failed to unstage all"));
        return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Commit
// ═══════════════════════════════════════════════════════════════════

bool GitManager::commit(const QString &message, bool amend)
{
    if (!ensureOpen()) return false;

    if (message.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Commit message cannot be empty");
        emit errorOccurred(m_lastError);
        return false;
    }

    // Write index → tree
    git_index *index = nullptr;
    if (git_repository_index(&index, m_repo) < 0) {
        setError(QStringLiteral("Failed to get index"));
        return false;
    }

    git_oid treeOid;
    if (git_index_write_tree(&treeOid, index) < 0) {
        setError(QStringLiteral("Failed to write tree"));
        git_index_free(index);
        return false;
    }
    git_index_write(index);
    git_index_free(index);

    git_tree *tree = nullptr;
    git_tree_lookup(&tree, m_repo, &treeOid);

    // Signature from git config
    git_signature *sig = nullptr;
    if (git_signature_default(&sig, m_repo) < 0) {
        setError(QStringLiteral("Configure user.name and user.email in git config"));
        git_tree_free(tree);
        return false;
    }

    // Parent commit
    git_oid commitOid;
    int err;
    git_reference *headRef = nullptr;
    int headErr = git_repository_head(&headRef, m_repo);

    QByteArray msgBytes = message.toUtf8();

    if (headErr == GIT_EUNBORNBRANCH || headErr == GIT_ENOTFOUND) {
        // Initial commit — no parent
        err = git_commit_create_v(&commitOid, m_repo, "HEAD",
                                  sig, sig, "UTF-8",
                                  msgBytes.constData(),
                                  tree, 0);
    } else {
        git_commit *parent = nullptr;
        git_reference_peel((git_object **)&parent, headRef, GIT_OBJECT_COMMIT);

        if (amend) {
            // Amend: replace HEAD commit, use HEAD's parent(s)
            int parentCount = git_commit_parentcount(parent);
            if (parentCount == 0) {
                // Amending initial commit
                err = git_commit_create_v(&commitOid, m_repo, "HEAD",
                                          sig, sig, "UTF-8",
                                          msgBytes.constData(),
                                          tree, 0);
            } else {
                git_commit *grandparent = nullptr;
                git_commit_parent(&grandparent, parent, 0);
                err = git_commit_create_v(&commitOid, m_repo, "HEAD",
                                          sig, sig, "UTF-8",
                                          msgBytes.constData(),
                                          tree, 1, grandparent);
                git_commit_free(grandparent);
            }
        } else {
            err = git_commit_create_v(&commitOid, m_repo, "HEAD",
                                      sig, sig, "UTF-8",
                                      msgBytes.constData(),
                                      tree, 1, parent);
        }

        git_commit_free(parent);
        git_reference_free(headRef);
    }

    git_signature_free(sig);
    git_tree_free(tree);

    if (err < 0) {
        setError(QStringLiteral("Failed to create commit"));
        return false;
    }
    return true;
}

bool GitManager::discardFileChanges(const QString &path)
{
    if (!ensureOpen() || path.isEmpty()) return false;

    // First try git_checkout_index to restore the file
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    
    QByteArray pathBytes = path.toUtf8();
    char *paths[1];
    paths[0] = pathBytes.data();
    opts.paths.strings = paths;
    opts.paths.count = 1;

    int err = git_checkout_index(m_repo, nullptr, &opts);
    if (err < 0) {
        // If it's untracked, git_checkout_index might fail or do nothing, 
        // we can just delete the file from disk.
        QFile file(QDir(repoPath()).absoluteFilePath(path));
        if (file.exists() && file.remove()) {
            return true;
        }
        setError(QStringLiteral("Failed to discard changes for '%1'").arg(path));
        return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Conflict Resolution
// ═══════════════════════════════════════════════════════════════════

bool GitManager::resolveUsingOurs(const QString &path)
{
    if (!ensureOpen() || path.isEmpty()) return false;
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_USE_OURS | GIT_CHECKOUT_FORCE;
    
    QByteArray pathBytes = path.toUtf8();
    char *paths[1];
    paths[0] = pathBytes.data();
    opts.paths.strings = paths;
    opts.paths.count = 1;

    if (git_checkout_index(m_repo, nullptr, &opts) < 0) {
        setError(QStringLiteral("Failed to checkout 'ours' version for '%1'").arg(path));
        return false;
    }
    return stageFile(path); // Mark as resolved by staging
}

bool GitManager::resolveUsingTheirs(const QString &path)
{
    if (!ensureOpen() || path.isEmpty()) return false;
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_USE_THEIRS | GIT_CHECKOUT_FORCE;
    
    QByteArray pathBytes = path.toUtf8();
    char *paths[1];
    paths[0] = pathBytes.data();
    opts.paths.strings = paths;
    opts.paths.count = 1;

    if (git_checkout_index(m_repo, nullptr, &opts) < 0) {
        setError(QStringLiteral("Failed to checkout 'theirs' version for '%1'").arg(path));
        return false;
    }
    return stageFile(path); // Mark as resolved by staging
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 3 — Interactive Rebase (Squash)
// ═══════════════════════════════════════════════════════════════════

bool GitManager::squashCommits(const QString &baseCommitId, const QString &newMessage)
{
    if (!ensureOpen() || baseCommitId.isEmpty()) return false;

    git_oid baseOid;
    if (git_oid_fromstr(&baseOid, baseCommitId.toUtf8().constData()) < 0) {
        setError(QStringLiteral("Invalid base commit ID for squash"));
        return false;
    }

    git_object *baseObj = nullptr;
    if (git_object_lookup(&baseObj, m_repo, &baseOid, GIT_OBJECT_COMMIT) < 0) {
        setError(QStringLiteral("Could not find base commit"));
        return false;
    }

    // Perform a soft reset to the base commit
    if (git_reset(m_repo, baseObj, GIT_RESET_SOFT, nullptr) < 0) {
        setError(QStringLiteral("Failed to perform soft reset"));
        git_object_free(baseObj);
        return false;
    }
    git_object_free(baseObj);

    // Commit the squashed changes with the new message
    return commit(newMessage, false);
}

bool GitManager::addToGitignore(const QString &path)
{
    if (!ensureOpen() || path.isEmpty()) return false;
    
    QString gitignorePath = QDir(repoPath()).absoluteFilePath(".gitignore");
    QFile file(gitignorePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "\n" << path << "\n";
        file.close();
        return true;
    }
    
    setError(QStringLiteral("Failed to write to .gitignore"));
    return false;
}


// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Credential callback
// ═══════════════════════════════════════════════════════════════════

int GitManager::credentialCb(void *out, const char *url,
                              const char *usernameFromUrl,
                              unsigned int allowedTypes, void *payload)
{
    git_credential **cred = reinterpret_cast<git_credential **>(out);
    GitManager *manager = static_cast<GitManager *>(payload);

    if (allowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        if (manager && !manager->m_token.isEmpty()) {
            QString user = manager->m_username;
            if (user.isEmpty() && usernameFromUrl) {
                user = QString::fromUtf8(usernameFromUrl);
            }
            return git_credential_userpass_plaintext_new(
                cred, user.toUtf8().constData(), manager->m_token.toUtf8().constData());
        }

        bool ok = false;
        QString defUser = usernameFromUrl
                              ? QString::fromUtf8(usernameFromUrl) : QString();
        QString username = QInputDialog::getText(
            nullptr, QStringLiteral("Authentication Required"),
            QStringLiteral("Username for %1:").arg(QString::fromUtf8(url)),
            QLineEdit::Normal, defUser, &ok);
        if (!ok) return GIT_EUSER;

        QString password = QInputDialog::getText(
            nullptr, QStringLiteral("Authentication Required"),
            QStringLiteral("Password / Token:"),
            QLineEdit::Password, QString(), &ok);
        if (!ok) return GIT_EUSER;

        return git_credential_userpass_plaintext_new(
            cred, username.toUtf8().constData(), password.toUtf8().constData());
    }

    return GIT_EUSER;   // unsupported type
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Push
// ═══════════════════════════════════════════════════════════════════

bool GitManager::push(const QString &remoteName)
{
    if (!ensureOpen()) return false;

    git_remote *remote = nullptr;
    if (git_remote_lookup(&remote, m_repo, remoteName.toUtf8().constData()) < 0) {
        setError(QStringLiteral("Remote '%1' not found").arg(remoteName));
        return false;
    }

    QString branch = getCurrentBranch();
    if (branch.isEmpty() || branch.startsWith("(") || branch.startsWith("HEAD")) {
        setError(QStringLiteral("Cannot push from detached HEAD or empty repo"));
        git_remote_free(remote);
        return false;
    }

    QByteArray refspec = QStringLiteral("refs/heads/%1:refs/heads/%1")
                             .arg(branch).toUtf8();
    char *specs[] = { refspec.data() };
    git_strarray refspecs = { specs, 1 };

    git_push_options opts = GIT_PUSH_OPTIONS_INIT;
    opts.callbacks.credentials = reinterpret_cast<git_credential_acquire_cb>(credentialCb);
    opts.callbacks.payload     = this;

    int err = git_remote_push(remote, &refspecs, &opts);
    git_remote_free(remote);

    if (err < 0) {
        setError(QStringLiteral("Push failed"));
        return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 2 — Pull  (fetch + fast-forward / merge)
// ═══════════════════════════════════════════════════════════════════

bool GitManager::pull(const QString &remoteName)
{
    if (!ensureOpen()) return false;

    // ── Step 1: Fetch ───────────────────────────────────────
    git_remote *remote = nullptr;
    if (git_remote_lookup(&remote, m_repo, remoteName.toUtf8().constData()) < 0) {
        setError(QStringLiteral("Remote '%1' not found").arg(remoteName));
        return false;
    }

    git_fetch_options fetchOpts = GIT_FETCH_OPTIONS_INIT;
    fetchOpts.callbacks.credentials =
        reinterpret_cast<git_credential_acquire_cb>(credentialCb);
    fetchOpts.callbacks.payload = this;

    int err = git_remote_fetch(remote, nullptr, &fetchOpts, "pull");
    git_remote_free(remote);

    if (err < 0) {
        setError(QStringLiteral("Fetch failed"));
        return false;
    }

    // ── Step 2: Find remote tracking branch ─────────────────
    QString branch = getCurrentBranch();
    QString trackingRefName = QStringLiteral("refs/remotes/%1/%2")
                                  .arg(remoteName, branch);

    git_oid remoteOid;
    if (git_reference_name_to_id(&remoteOid, m_repo,
                                  trackingRefName.toUtf8().constData()) < 0) {
        // Nothing to merge (no tracking branch)
        return true;
    }

    // ── Step 3: Merge analysis ──────────────────────────────
    git_annotated_commit *fetchHead = nullptr;
    if (git_annotated_commit_lookup(&fetchHead, m_repo, &remoteOid) < 0) {
        setError(QStringLiteral("Failed to lookup fetched commit"));
        return false;
    }

    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    const git_annotated_commit *heads[] = { fetchHead };

    git_merge_analysis(&analysis, &preference, m_repo, heads, 1);

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        git_annotated_commit_free(fetchHead);
        return true;  // already up-to-date
    }

    // ── Step 4a: Fast-forward ───────────────────────────────
    if (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) {
        git_reference *headRef = nullptr;
        git_repository_head(&headRef, m_repo);

        git_reference *newRef = nullptr;
        git_reference_set_target(&newRef, headRef, &remoteOid, "pull: fast-forward");

        git_checkout_options coOpts = GIT_CHECKOUT_OPTIONS_INIT;
        coOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

        git_object *target = nullptr;
        git_object_lookup(&target, m_repo, &remoteOid, GIT_OBJECT_COMMIT);
        git_checkout_tree(m_repo, target, &coOpts);

        git_object_free(target);
        if (newRef) git_reference_free(newRef);
        git_reference_free(headRef);
        git_annotated_commit_free(fetchHead);
        return true;
    }

    // ── Step 4b: Normal merge ───────────────────────────────
    if (analysis & GIT_MERGE_ANALYSIS_NORMAL) {
        git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
        git_checkout_options coOpts = GIT_CHECKOUT_OPTIONS_INIT;
        coOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

        err = git_merge(m_repo, heads, 1, &mergeOpts, &coOpts);
        if (err < 0) {
            setError(QStringLiteral("Merge failed"));
            git_annotated_commit_free(fetchHead);
            return false;
        }

        // Check for conflicts
        git_index *index = nullptr;
        git_repository_index(&index, m_repo);
        if (git_index_has_conflicts(index)) {
            setError(QStringLiteral("Merge has conflicts — resolve manually"));
            git_index_free(index);
            git_annotated_commit_free(fetchHead);
            return false;
        }

        // Create merge commit (two parents)
        git_oid treeOid;
        git_index_write_tree(&treeOid, index);
        git_index_write(index);
        git_index_free(index);

        git_tree *tree = nullptr;
        git_tree_lookup(&tree, m_repo, &treeOid);

        git_signature *sig = nullptr;
        git_signature_default(&sig, m_repo);

        git_commit *ourHead = nullptr;
        git_reference *hr = nullptr;
        git_repository_head(&hr, m_repo);
        git_reference_peel((git_object **)&ourHead, hr, GIT_OBJECT_COMMIT);

        git_commit *theirCommit = nullptr;
        git_commit_lookup(&theirCommit, m_repo, &remoteOid);

        git_oid commitOid;
        const git_commit *parents[] = { ourHead, theirCommit };
        git_commit_create(&commitOid, m_repo, "HEAD",
                          sig, sig, "UTF-8",
                          "Merge remote-tracking branch",
                          tree, 2, parents);

        git_repository_state_cleanup(m_repo);

        git_commit_free(theirCommit);
        git_commit_free(ourHead);
        git_reference_free(hr);
        git_signature_free(sig);
        git_tree_free(tree);
    }

    git_annotated_commit_free(fetchHead);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 3 — Diff (workdir = unstaged changes)
// ═══════════════════════════════════════════════════════════════════

QString GitManager::getWorkdirDiff(const QString &filePath)
{
    if (!ensureOpen()) return QString();

    git_diff *diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    opts.context_lines = 3;

    QByteArray pathBytes;
    char *paths[1];
    if (!filePath.isEmpty()) {
        pathBytes = filePath.toUtf8();
        paths[0] = pathBytes.data();
        opts.pathspec.strings = paths;
        opts.pathspec.count   = 1;
    }

    if (git_diff_index_to_workdir(&diff, m_repo, nullptr, &opts) < 0) {
        setError(QStringLiteral("Failed to create workdir diff"));
        return QString();
    }

    QString result;
    git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diffPrintCb, &result);
    git_diff_free(diff);
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 3 — Diff (staged = index vs HEAD)
// ═══════════════════════════════════════════════════════════════════

QString GitManager::getStagedDiff(const QString &filePath)
{
    if (!ensureOpen()) return QString();

    // Get HEAD tree (may be null for initial commit)
    git_tree *headTree = nullptr;
    git_object *obj = nullptr;
    if (git_revparse_single(&obj, m_repo, "HEAD^{tree}") == 0) {
        headTree = reinterpret_cast<git_tree *>(obj);
    }

    git_diff *diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    opts.context_lines = 3;

    QByteArray pathBytes;
    char *paths[1];
    if (!filePath.isEmpty()) {
        pathBytes = filePath.toUtf8();
        paths[0] = pathBytes.data();
        opts.pathspec.strings = paths;
        opts.pathspec.count   = 1;
    }

    if (git_diff_tree_to_index(&diff, m_repo, headTree, nullptr, &opts) < 0) {
        setError(QStringLiteral("Failed to create staged diff"));
        if (headTree) git_tree_free(headTree);
        return QString();
    }

    QString result;
    git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diffPrintCb, &result);

    git_diff_free(diff);
    if (headTree) git_tree_free(headTree);
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 5 — Commit Details (Tree-to-Tree Diff)
// ═══════════════════════════════════════════════════════════════════

QVector<FileStatusEntry> GitManager::getCommitChangedFiles(const QString &commitId)
{
    QVector<FileStatusEntry> result;
    if (!ensureOpen() || commitId.isEmpty()) return result;

    git_oid oid;
    if (git_oid_fromstr(&oid, commitId.toUtf8().constData()) < 0) return result;

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, m_repo, &oid) < 0) return result;

    git_tree *tree = nullptr;
    git_commit_tree(&tree, commit);

    git_tree *parentTree = nullptr;
    if (git_commit_parentcount(commit) > 0) {
        git_commit *parent = nullptr;
        if (git_commit_parent(&parent, commit, 0) == 0) {
            git_commit_tree(&parentTree, parent);
            git_commit_free(parent);
        }
    }

    git_diff *diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    if (git_diff_tree_to_tree(&diff, m_repo, parentTree, tree, &opts) == 0) {
        size_t numDeltas = git_diff_num_deltas(diff);
        for (size_t i = 0; i < numDeltas; ++i) {
            const git_diff_delta *delta = git_diff_get_delta(diff, i);
            FileStatusEntry fse;
            fse.path = QString::fromUtf8(delta->new_file.path);
            
            if (delta->status == GIT_DELTA_ADDED) {
                fse.worktreeStatus = FileStatusEntry::Added;
            } else if (delta->status == GIT_DELTA_DELETED) {
                fse.worktreeStatus = FileStatusEntry::Deleted;
            } else if (delta->status == GIT_DELTA_MODIFIED) {
                fse.worktreeStatus = FileStatusEntry::Modified;
            } else if (delta->status == GIT_DELTA_RENAMED) {
                fse.worktreeStatus = FileStatusEntry::Renamed;
                fse.oldPath = QString::fromUtf8(delta->old_file.path);
            } else {
                fse.worktreeStatus = FileStatusEntry::Modified; // Fallback
            }
            result.append(fse);
        }
        git_diff_free(diff);
    }

    if (tree) git_tree_free(tree);
    if (parentTree) git_tree_free(parentTree);
    git_commit_free(commit);

    return result;
}

QString GitManager::getCommitDiff(const QString &commitId, const QString &filePath)
{
    if (!ensureOpen() || commitId.isEmpty()) return QString();

    git_oid oid;
    if (git_oid_fromstr(&oid, commitId.toUtf8().constData()) < 0) return QString();

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, m_repo, &oid) < 0) return QString();

    git_tree *tree = nullptr;
    git_commit_tree(&tree, commit);

    git_tree *parentTree = nullptr;
    if (git_commit_parentcount(commit) > 0) {
        git_commit *parent = nullptr;
        if (git_commit_parent(&parent, commit, 0) == 0) {
            git_commit_tree(&parentTree, parent);
            git_commit_free(parent);
        }
    }

    git_diff *diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    opts.context_lines = 3;

    QByteArray pathBytes;
    char *paths[1];
    if (!filePath.isEmpty()) {
        pathBytes = filePath.toUtf8();
        paths[0] = pathBytes.data();
        opts.pathspec.strings = paths;
        opts.pathspec.count   = 1;
    }

    QString result;
    if (git_diff_tree_to_tree(&diff, m_repo, parentTree, tree, &opts) == 0) {
        git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diffPrintCb, &result);
        git_diff_free(diff);
    }

    if (tree) git_tree_free(tree);
    if (parentTree) git_tree_free(parentTree);
    git_commit_free(commit);

    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 4 — Branch listing
// ═══════════════════════════════════════════════════════════════════

QVector<BranchInfo> GitManager::getBranches()
{
    QVector<BranchInfo> result;
    if (!ensureOpen()) return result;

    QString currentBranch = getCurrentBranch();

    // Local branches
    git_branch_iterator *it = nullptr;
    if (git_branch_iterator_new(&it, m_repo, GIT_BRANCH_ALL) == 0) {
        git_reference *ref = nullptr;
        git_branch_t type;

        while (git_branch_next(&ref, &type, it) == 0) {
            const char *name = nullptr;
            git_branch_name(&name, ref);

            char hashBuf[41] = {0};
            git_reference *resolvedRef = nullptr;
            if (git_reference_resolve(&resolvedRef, ref) == 0) {
                const git_oid *oid = git_reference_target(resolvedRef);
                if (oid) {
                    git_oid_tostr(hashBuf, sizeof(hashBuf), oid);
                }
                git_reference_free(resolvedRef);
            }

            BranchInfo bi;
            bi.name       = QString::fromUtf8(name);
            bi.isRemote   = (type == GIT_BRANCH_REMOTE);
            bi.isHead     = (!bi.isRemote && bi.name == currentBranch);
            bi.targetHash = QString::fromUtf8(hashBuf);

            result.append(bi);
            git_reference_free(ref);
        }
        git_branch_iterator_free(it);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 5 — Tags
// ═══════════════════════════════════════════════════════════════════

QVector<BranchInfo> GitManager::getTags()
{
    QVector<BranchInfo> result;
    if (!ensureOpen()) return result;

    git_strarray tag_names;
    if (git_tag_list(&tag_names, m_repo) == 0) {
        for (size_t i = 0; i < tag_names.count; ++i) {
            const char *name = tag_names.strings[i];
            
            git_object *obj = nullptr;
            if (git_revparse_single(&obj, m_repo, name) == 0) {
                // If it's an annotated tag, we need to peel it to get the commit hash
                git_object *peeled = nullptr;
                git_object_peel(&peeled, obj, GIT_OBJECT_COMMIT);
                
                if (peeled) {
                    char hashBuf[41] = {0};
                    git_oid_tostr(hashBuf, sizeof(hashBuf), git_object_id(peeled));
                    
                    BranchInfo bi;
                    bi.name = QString::fromUtf8(name);
                    bi.isRemote = false;
                    bi.isHead = false;
                    bi.targetHash = QString::fromUtf8(hashBuf);
                    
                    result.append(bi);
                    git_object_free(peeled);
                }
                git_object_free(obj);
            }
        }
        git_strarray_dispose(&tag_names);
    }
    return result;
}

bool GitManager::createTag(const QString &tagName, const QString &commitId, const QString &message)
{
    if (!ensureOpen()) return false;

    git_oid targetOid;
    if (git_oid_fromstr(&targetOid, commitId.toUtf8().constData()) != 0) {
        setError(QStringLiteral("Invalid commit hash"));
        return false;
    }

    git_object *target = nullptr;
    if (git_object_lookup(&target, m_repo, &targetOid, GIT_OBJECT_COMMIT) != 0) {
        setError(QStringLiteral("Commit not found"));
        return false;
    }

    git_oid new_tag_oid;
    int err = 0;
    
    if (message.isEmpty()) {
        err = git_tag_create_lightweight(&new_tag_oid, m_repo, tagName.toUtf8().constData(), target, 0);
    } else {
        git_signature *sig = nullptr;
        git_signature_default(&sig, m_repo);
        err = git_tag_create(&new_tag_oid, m_repo, tagName.toUtf8().constData(), target, sig, message.toUtf8().constData(), 0);
        git_signature_free(sig);
    }

    git_object_free(target);

    if (err != 0) {
        const git_error *e = git_error_last();
        setError(QStringLiteral("Failed to create tag: %1").arg(e ? e->message : "unknown"));
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 4 — Create branch
// ═══════════════════════════════════════════════════════════════════

bool GitManager::createBranch(const QString &name)
{
    if (!ensureOpen()) return false;

    // Get HEAD commit
    git_object *headObj = nullptr;
    if (git_revparse_single(&headObj, m_repo, "HEAD") < 0) {
        setError(QStringLiteral("Failed to resolve HEAD"));
        return false;
    }

    git_commit *headCommit = nullptr;
    if (git_commit_lookup(&headCommit, m_repo,
                          git_object_id(headObj)) < 0)
    {
        setError(QStringLiteral("Failed to lookup HEAD commit"));
        git_object_free(headObj);
        return false;
    }
    git_object_free(headObj);

    git_reference *branch = nullptr;
    int err = git_branch_create(&branch, m_repo, name.toUtf8().constData(),
                                headCommit, 0 /* don't force */);

    git_commit_free(headCommit);

    if (err < 0) {
        setError(QStringLiteral("Failed to create branch '%1'").arg(name));
        return false;
    }

    git_reference_free(branch);
    return true;
}

bool GitManager::createBranchAt(const QString &name, const QString &commitId)
{
    if (!ensureOpen()) return false;

    git_oid oid;
    if (git_oid_fromstr(&oid, commitId.toUtf8().constData()) < 0) {
        setError(QStringLiteral("Invalid commit hash: %1").arg(commitId));
        return false;
    }

    git_commit *targetCommit = nullptr;
    if (git_commit_lookup(&targetCommit, m_repo, &oid) < 0) {
        setError(QStringLiteral("Failed to lookup commit %1").arg(commitId));
        return false;
    }

    git_reference *branch = nullptr;
    int err = git_branch_create(&branch, m_repo, name.toUtf8().constData(),
                                targetCommit, 0 /* don't force */);

    git_commit_free(targetCommit);

    if (err < 0) {
        setError(QStringLiteral("Failed to create branch '%1' at %2").arg(name, commitId));
        return false;
    }

    git_reference_free(branch);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 4 — Checkout branch
// ═══════════════════════════════════════════════════════════════════

bool GitManager::checkoutBranch(const QString &name)
{
    if (!ensureOpen()) return false;

    QString fullRef = QStringLiteral("refs/heads/%1").arg(name);

    // Resolve the target tree
    git_object *treeish = nullptr;
    if (git_revparse_single(&treeish, m_repo,
                             fullRef.toUtf8().constData()) < 0)
    {
        setError(QStringLiteral("Branch '%1' not found").arg(name));
        return false;
    }

    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if (git_checkout_tree(m_repo, treeish, &opts) < 0) {
        setError(QStringLiteral("Failed to checkout '%1'").arg(name));
        git_object_free(treeish);
        return false;
    }

    // Point HEAD to the branch
    if (git_repository_set_head(m_repo,
                                 fullRef.toUtf8().constData()) < 0)
    {
        setError(QStringLiteral("Failed to update HEAD"));
        git_object_free(treeish);
        return false;
    }

    git_object_free(treeish);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 4 — Merge branch
// ═══════════════════════════════════════════════════════════════════

bool GitManager::mergeBranch(const QString &name)
{
    if (!ensureOpen()) return false;

    // Resolve branch reference
    QString refName = QStringLiteral("refs/heads/%1").arg(name);
    git_reference *branchRef = nullptr;
    if (git_reference_lookup(&branchRef, m_repo,
                              refName.toUtf8().constData()) < 0)
    {
        setError(QStringLiteral("Branch '%1' not found").arg(name));
        return false;
    }

    git_annotated_commit *theirHead = nullptr;
    if (git_annotated_commit_from_ref(&theirHead, m_repo, branchRef) < 0) {
        setError(QStringLiteral("Failed to lookup annotated commit"));
        git_reference_free(branchRef);
        return false;
    }
    git_reference_free(branchRef);

    // Merge analysis
    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    const git_annotated_commit *heads[] = { theirHead };
    git_merge_analysis(&analysis, &preference, m_repo, heads, 1);

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        git_annotated_commit_free(theirHead);
        return true;
    }

    // Fast-forward
    if (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) {
        const git_oid *targetOid = git_annotated_commit_id(theirHead);

        git_reference *headRef = nullptr;
        git_repository_head(&headRef, m_repo);

        git_reference *newRef = nullptr;
        git_reference_set_target(&newRef, headRef, targetOid,
                                 "merge: fast-forward");

        git_checkout_options coOpts = GIT_CHECKOUT_OPTIONS_INIT;
        coOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

        git_object *target = nullptr;
        git_object_lookup(&target, m_repo, targetOid, GIT_OBJECT_COMMIT);
        git_checkout_tree(m_repo, target, &coOpts);

        git_object_free(target);
        if (newRef) git_reference_free(newRef);
        git_reference_free(headRef);
        git_annotated_commit_free(theirHead);
        return true;
    }

    // Normal merge
    git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options coOpts = GIT_CHECKOUT_OPTIONS_INIT;
    coOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if (git_merge(m_repo, heads, 1, &mergeOpts, &coOpts) < 0) {
        setError(QStringLiteral("Merge failed"));
        git_annotated_commit_free(theirHead);
        return false;
    }

    // Check conflicts
    git_index *index = nullptr;
    git_repository_index(&index, m_repo);
    if (git_index_has_conflicts(index)) {
        setError(QStringLiteral("Merge has conflicts — resolve manually"));
        git_index_free(index);
        git_annotated_commit_free(theirHead);
        return false;
    }

    // Create merge commit
    git_oid treeOid;
    git_index_write_tree(&treeOid, index);
    git_index_write(index);
    git_index_free(index);

    git_tree *tree = nullptr;
    git_tree_lookup(&tree, m_repo, &treeOid);

    git_signature *sig = nullptr;
    git_signature_default(&sig, m_repo);

    git_commit *ourHead = nullptr;
    git_reference *hr = nullptr;
    git_repository_head(&hr, m_repo);
    git_reference_peel((git_object **)&ourHead, hr, GIT_OBJECT_COMMIT);

    const git_oid *theirOid = git_annotated_commit_id(theirHead);
    git_commit *theirCommit = nullptr;
    git_commit_lookup(&theirCommit, m_repo, theirOid);

    git_oid commitOid;
    QString msg = QStringLiteral("Merge branch '%1'").arg(name);
    QByteArray msgBytes = msg.toUtf8();
    const git_commit *parents[] = { ourHead, theirCommit };
    git_commit_create(&commitOid, m_repo, "HEAD",
                      sig, sig, "UTF-8", msgBytes.constData(),
                      tree, 2, parents);

    git_repository_state_cleanup(m_repo);

    git_commit_free(theirCommit);
    git_commit_free(ourHead);
    git_reference_free(hr);
    git_signature_free(sig);
    git_tree_free(tree);
    git_annotated_commit_free(theirHead);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 4 — Delete branch
// ═══════════════════════════════════════════════════════════════════

bool GitManager::deleteBranch(const QString &name)
{
    if (!ensureOpen()) return false;

    git_reference *branchRef = nullptr;
    if (git_branch_lookup(&branchRef, m_repo,
                          name.toUtf8().constData(),
                          GIT_BRANCH_LOCAL) < 0)
    {
        setError(QStringLiteral("Branch '%1' not found").arg(name));
        return false;
    }

    // Don't delete current branch
    if (git_branch_is_head(branchRef)) {
        setError(QStringLiteral("Cannot delete the currently checked-out branch"));
        git_reference_free(branchRef);
        return false;
    }

    int err = git_branch_delete(branchRef);
    git_reference_free(branchRef);

    if (err < 0) {
        setError(QStringLiteral("Failed to delete branch '%1'").arg(name));
        return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 3 — Stash & Advanced
// ═══════════════════════════════════════════════════════════════════

bool GitManager::stashSave(const QString &message)
{
    if (!ensureOpen()) return false;

    git_signature *sig = nullptr;
    if (git_signature_default(&sig, m_repo) < 0) {
        git_signature_now(&sig, "Unknown", "unknown@example.com");
    }

    git_oid out;
    int err = git_stash_save(&out, m_repo, sig, message.toUtf8().constData(), GIT_STASH_DEFAULT);
    git_signature_free(sig);

    if (err == GIT_ENOTFOUND) {
        setError(QStringLiteral("No local changes to stash"));
        return false;
    }
    if (err < 0) {
        setError(QStringLiteral("Failed to stash changes"));
        return false;
    }
    return true;
}

bool GitManager::stashPop()
{
    if (!ensureOpen()) return false;

    git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
    int err = git_stash_pop(m_repo, 0, &opts);
    if (err == GIT_ENOTFOUND) {
        setError(QStringLiteral("No stashes available to pop"));
        return false;
    }
    if (err < 0) {
        setError(QStringLiteral("Failed to pop stash"));
        return false;
    }
    return true;
}

bool GitManager::cherryPick(const QString &commitId)
{
    if (!ensureOpen() || commitId.isEmpty()) return false;

    git_oid oid;
    if (git_oid_fromstr(&oid, commitId.toUtf8().constData()) < 0) {
        setError("Invalid commit ID for cherry-pick");
        return false;
    }

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, m_repo, &oid) < 0) {
        setError("Commit not found");
        return false;
    }

    git_cherrypick_options opts = GIT_CHERRYPICK_OPTIONS_INIT;
    int err = git_cherrypick(m_repo, commit, &opts);
    git_commit_free(commit);

    if (err < 0) {
        setError("Cherry-pick failed (check for conflicts)");
        return false;
    }
    return true;
}

bool GitManager::revertCommit(const QString &commitId)
{
    if (!ensureOpen() || commitId.isEmpty()) return false;

    git_oid oid;
    if (git_oid_fromstr(&oid, commitId.toUtf8().constData()) < 0) {
        setError("Invalid commit ID for revert");
        return false;
    }

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, m_repo, &oid) < 0) {
        setError("Commit not found");
        return false;
    }

    git_revert_options opts = GIT_REVERT_OPTIONS_INIT;
    int err = git_revert(m_repo, commit, &opts);
    git_commit_free(commit);

    if (err < 0) {
        setError("Revert failed (check for conflicts)");
        return false;
    }
    return true;
}

QVector<BlameLine> GitManager::getBlame(const QString &filePath)
{
    QVector<BlameLine> result;
    if (!ensureOpen() || filePath.isEmpty()) return result;

    git_blame_options opts = GIT_BLAME_OPTIONS_INIT;
    git_blame *blame = nullptr;

    if (git_blame_file(&blame, m_repo, filePath.toUtf8().constData(), &opts) < 0) {
        setError(QStringLiteral("Failed to blame file '%1'").arg(filePath));
        return result;
    }

    QFile file(QDir(repoPath()).absoluteFilePath(filePath));
    QStringList lines;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lines = QString::fromUtf8(file.readAll()).split('\n');
    }

    uint32_t hunk_count = git_blame_get_hunk_count(blame);
    for (uint32_t i = 0; i < hunk_count; ++i) {
        const git_blame_hunk *hunk = git_blame_get_hunk_byindex(blame, i);
        if (!hunk) continue;

        char oid_str[GIT_OID_HEXSZ + 1];
        git_oid_tostr(oid_str, sizeof(oid_str), &hunk->final_commit_id);
        QString commitId = QString::fromUtf8(oid_str);
        
        QString author = hunk->final_signature ? QString::fromUtf8(hunk->final_signature->name) : QStringLiteral("Unknown");
        QDateTime date;
        if (hunk->final_signature) {
            date = QDateTime::fromSecsSinceEpoch(hunk->final_signature->when.time);
        }

        for (uint16_t j = 0; j < hunk->lines_in_hunk; ++j) {
            BlameLine bl;
            bl.commitId = commitId;
            bl.author = author;
            bl.date = date;
            bl.lineNo = hunk->final_start_line_number + j;
            if (bl.lineNo >= 1 && bl.lineNo <= lines.size()) {
                bl.code = lines[bl.lineNo - 1];
            }
            result.append(bl);
        }
    }

    git_blame_free(blame);
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  Faz 6 — Credentials & Remote Operations
// ═══════════════════════════════════════════════════════════════════

void GitManager::setCredentials(const QString &username, const QString &token)
{
    m_username = username;
    m_token = token;
}

bool GitManager::cloneRepository(const QString &url, const QString &localPath)
{
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    clone_opts.fetch_opts.callbacks.credentials = reinterpret_cast<git_credential_acquire_cb>(credentialCb);
    clone_opts.fetch_opts.callbacks.payload = this;
    
    git_repository *cloned_repo = nullptr;
    int err = git_clone(&cloned_repo, url.toUtf8().constData(), localPath.toUtf8().constData(), &clone_opts);
    
    if (err < 0) {
        const git_error *e = git_error_last();
        setError(QStringLiteral("Failed to clone repository: %1").arg(e ? QString::fromUtf8(e->message) : QStringLiteral("Unknown error")));
        return false;
    }
    
    git_repository_free(cloned_repo);
    return openRepository(localPath);
}

bool GitManager::fetch(const QString &remoteName)
{
    if (!ensureOpen()) return false;
    
    git_remote *remote = nullptr;
    int err = git_remote_lookup(&remote, m_repo, remoteName.toUtf8().constData());
    if (err < 0) {
        setError(QStringLiteral("Remote '%1' not found").arg(remoteName));
        return false;
    }
    
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    fetch_opts.callbacks.credentials = reinterpret_cast<git_credential_acquire_cb>(credentialCb);
    fetch_opts.callbacks.payload = this;
    
    err = git_remote_fetch(remote, nullptr, &fetch_opts, nullptr);
    git_remote_free(remote);
    
    if (err < 0) {
        const git_error *e = git_error_last();
        setError(QStringLiteral("Fetch failed: %1").arg(e ? QString::fromUtf8(e->message) : QStringLiteral("Unknown error")));
        return false;
    }
    return true;
}

