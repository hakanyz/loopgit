#pragma once

#include <QDialog>
#include <QString>
#include <QVector>
#include <QSplitter>

class GitManager;
class QTreeWidget;
class QTreeWidgetItem;
class DiffViewWidget;

class CompareDialog : public QDialog {
    Q_OBJECT
public:
    explicit CompareDialog(GitManager *git, const QString &oid1, const QString &oid2, QWidget *parent = nullptr);

private slots:
    void onFileSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    void setupUi();
    void loadDiff();

    GitManager *m_git;
    QString m_oid1;
    QString m_oid2;
    
    QTreeWidget *m_fileList;
    DiffViewWidget *m_diffView;
};
