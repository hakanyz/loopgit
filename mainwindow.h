#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

class QTabWidget;
class QToolBar;
class QAction;
class QComboBox;
class RepoWidget;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openRepository();
    void openRepositoryPath(const QString &path);
    void closeCurrentRepository();
    void cloneRepository();
    void onTabChanged(int index);
    void updateBranchCombo();
    void onBranchComboActivated(int index);
    void showAboutDialog();
    void openCredentials();
    void showStatusMessage(const QString &msg);
    void showError(const QString &msg);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupWelcomeScreen();
    void applyDarkTheme();

    RepoWidget* currentRepoWidget() const;

    QTabWidget *m_tabWidget;
    QWidget    *m_welcomeWidget;
    
    QToolBar   *m_toolBar;
    QComboBox  *m_branchCombo;
    bool       m_updatingBranch = false;

    // Actions
    QAction *m_actOpen;
    QAction *m_actClone;
    QAction *m_actClose;
    QAction *m_actCredentials;
    
    QAction *m_actLocalFiles;
    QAction *m_actHistory;
    QAction *m_actRefresh;
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
};

#endif // MAINWINDOW_H
