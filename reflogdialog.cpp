#include "reflogdialog.h"
#include "gitmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

ReflogDialog::ReflogDialog(GitManager *git, QWidget *parent)
    : QDialog(parent), m_git(git)
{
    setWindowTitle("Reflog Viewer");
    resize(700, 400);
    setupUi();
    loadData();
}

void ReflogDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_table = new QTableWidget;
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Old Hash", "New Hash", "Message"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet("QTableWidget { background-color: #1E1E1E; color: #CCCCCC; }");
    mainLayout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_refreshBtn = new QPushButton("Refresh");
    connect(m_refreshBtn, &QPushButton::clicked, this, &ReflogDialog::loadData);
    btnLayout->addWidget(m_refreshBtn);

    btnLayout->addStretch();

    m_closeBtn = new QPushButton("Close");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);
}

void ReflogDialog::loadData()
{
    if (!m_git) return;

    m_table->setRowCount(0);
    QVector<GitManager::ReflogEntry> reflog = m_git->getReflog();

    m_table->setRowCount(reflog.size());
    for (int i = 0; i < reflog.size(); ++i) {
        const auto &entry = reflog[i];
        
        QTableWidgetItem *oldItem = new QTableWidgetItem(entry.oldId.left(7));
        QTableWidgetItem *newItem = new QTableWidgetItem(entry.newId.left(7));
        QTableWidgetItem *msgItem = new QTableWidgetItem(entry.message);
        
        m_table->setItem(i, 0, oldItem);
        m_table->setItem(i, 1, newItem);
        m_table->setItem(i, 2, msgItem);
    }
    
    m_table->resizeColumnsToContents();
}
