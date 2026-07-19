#pragma once

#include <QDialog>
#include <QString>

class GitManager;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(GitManager *git, QWidget *parent = nullptr);

private slots:
    void saveUserConfig();
    void addRemote();
    void editRemote();
    void removeRemote();
    void onRemoteSelected();

private:
    void setupUi();
    void loadData();

    GitManager *m_git;

    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QPushButton *m_saveUserBtn;

    QListWidget *m_remotesList;
    QPushButton *m_addRemoteBtn;
    QPushButton *m_editRemoteBtn;
    QPushButton *m_removeRemoteBtn;
};
