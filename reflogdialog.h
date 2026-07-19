#pragma once

#include <QDialog>
#include <QString>

class GitManager;
class QTableWidget;
class QPushButton;

class ReflogDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReflogDialog(GitManager *git, QWidget *parent = nullptr);

private slots:
    void loadData();

private:
    void setupUi();

    GitManager *m_git;
    QTableWidget *m_table;
    QPushButton *m_closeBtn;
    QPushButton *m_refreshBtn;
};
