#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QSystemTrayIcon>
#include <QCloseEvent>

class QTabWidget;
class QToolBar;
class QAction;
class QComboBox;
class QPushButton;
class RepoWidget;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void initRepository();
    void openRepository();
    void openRepositoryPath(const QString &path);
    void closeCurrentRepository();
    void cloneRepository();

    void addNewHomeTab();
    void onTabChanged(int index);
    void updateBranchCombo();
    void showAboutDialog();
    void showShortcutsDialog();
    void openCredentials();
    void showStatusMessage(const QString &msg);
    void showError(const QString &msg);

    // Auto-update slots
    void checkForUpdates(bool silent = false);
    void onUpdateCheckFinished(QNetworkReply *reply, bool silent);
    void onTrayMessageClicked();
    void startUpdateDownload();
    void onDownloadFinished(QNetworkReply *reply, const QString &downloadPath);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();

    void applyDarkTheme();

    RepoWidget* currentRepoWidget() const;

    QTabWidget *m_tabWidget;
    int m_lastActiveTabIndex = 0;
    
    QToolBar   *m_toolBar;
    bool       m_updatingBranch = false;

    // Actions
    QAction *m_actOpen;
    QAction *m_actClone;
    QAction *m_actClose;
    QAction *m_actCredentials;
    
    QAction *m_actLocalFiles;
    QAction *m_actHistory;
    QAction *m_actRefresh;
    QAction *m_actTerminal;
    QAction *m_actStageAll;
    QAction *m_actUnstageAll;
    QAction *m_actPull;
    QAction *m_actPush;
    QAction *m_actFetch;
    QLabel *m_syncStatusLabel;
    
    QAction *m_actFeature;
    QAction *m_actBugfix;
    QAction *m_actRelease;
    QAction *m_actHotfix;
    QAction *m_actFinish;

    QMap<QString, QString> m_credentials;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    bool m_reallyQuit = false;

    // Auto-update variables
    QNetworkAccessManager *m_netManager;
    QString m_latestUpdateVersion;
    QString m_latestUpdateUrl;

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
