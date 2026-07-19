#include "comparedialog.h"
#include "gitmanager.h"
#include "diffviewwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>

CompareDialog::CompareDialog(GitManager *git, const QString &oid1, const QString &oid2, QWidget *parent)
    : QDialog(parent), m_git(git), m_oid1(oid1), m_oid2(oid2)
{
    setWindowTitle(QString("Compare: %1 ... %2").arg(oid1.left(7), oid2.left(7)));
    resize(1000, 600);
    setupUi();
    loadDiff();
}

void CompareDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    m_fileList = new QTreeWidget;
    m_fileList->setHeaderLabels({"Status", "File", "Path"});
    m_fileList->setColumnWidth(0, 60);
    m_fileList->setColumnWidth(1, 150);
    m_fileList->setStyleSheet("QTreeWidget { background-color: #1E1E1E; color: #CCCCCC; }");
    connect(m_fileList, &QTreeWidget::currentItemChanged, this, &CompareDialog::onFileSelected);

    m_diffView = new DiffViewWidget;

    splitter->addWidget(m_fileList);
    splitter->addWidget(m_diffView);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    QPushButton *btnClose = new QPushButton("Close");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnClose);

    mainLayout->addLayout(btnLayout);
}

void CompareDialog::loadDiff()
{
    if (!m_git) return;

    m_fileList->clear();
    QVector<FileStatusEntry> entries = m_git->getTwoCommitsChangedFiles(m_oid1, m_oid2);

    for (const auto &entry : entries) {
        auto *item = new QTreeWidgetItem(m_fileList);
        QString statusText = FileStatusEntry::statusString(entry.worktreeStatus);
        
        QFileInfo fi(entry.path);
        
        item->setText(0, statusText);
        item->setText(1, fi.fileName());
        item->setText(2, fi.path() == "." ? "" : fi.path());
        
        item->setData(0, Qt::UserRole, entry.path);
        item->setTextAlignment(0, Qt::AlignCenter);
        
        QColor bgColor;
        QColor fgColor = Qt::white;
        
        switch (entry.worktreeStatus) {
            case FileStatusEntry::Added:    bgColor = QColor("#238636"); break; // Green
            case FileStatusEntry::Deleted:  bgColor = QColor("#DA3633"); break; // Red
            case FileStatusEntry::Modified: bgColor = QColor("#007ACC"); break; // Blue
            case FileStatusEntry::Renamed:  bgColor = QColor("#8957E5"); break; // Purple
            default:                        bgColor = QColor("#7A7A7A"); break; // Gray
        }
        
        item->setBackground(0, bgColor);
        item->setForeground(0, fgColor);
        item->setForeground(1, bgColor.lighter(130));
    }
}

void CompareDialog::onFileSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    m_diffView->clearDiff();

    if (!current) return;

    QString filePath = current->data(0, Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    QString diffText = m_git->getTwoCommitsDiff(m_oid1, m_oid2, filePath);
    m_diffView->setDiffText(diffText);
}
