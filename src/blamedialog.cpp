#include "blamedialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPalette>
#include <QFontDatabase>

BlameDialog::BlameDialog(const QString &filePath, const QVector<BlameLine> &blameData, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Blame: %1").arg(filePath));
    resize(1000, 600);

    m_table = new QTableWidget(blameData.size(), 5, this);
    m_table->setHorizontalHeaderLabels({"Commit", "Author", "Date", "Line", "Code"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { border: none; selection-background-color: #062f4a; selection-color: white; outline: none; }"
        "QTableWidget::item { padding-left: 5px; padding-right: 5px; }"
    );

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    for (int i = 0; i < blameData.size(); ++i) {
        const BlameLine &bl = blameData[i];

        QTableWidgetItem *itemCommit = new QTableWidgetItem(bl.commitId.left(7));
        itemCommit->setToolTip(bl.commitId);
        itemCommit->setForeground(QColor("#D2A8FF")); // Light purple

        QTableWidgetItem *itemAuthor = new QTableWidgetItem(bl.author);
        itemAuthor->setForeground(QColor("#79C0FF")); // Light blue

        QTableWidgetItem *itemDate = new QTableWidgetItem(bl.date.toString("yyyy-MM-dd HH:mm"));
        itemDate->setForeground(QColor("#8B949E")); // Gray

        QTableWidgetItem *itemLineNo = new QTableWidgetItem(QString::number(bl.lineNo));
        itemLineNo->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemLineNo->setForeground(QColor("#8B949E"));

        QTableWidgetItem *itemCode = new QTableWidgetItem(bl.code);
        itemCode->setFont(fixedFont);

        m_table->setItem(i, 0, itemCommit);
        m_table->setItem(i, 1, itemAuthor);
        m_table->setItem(i, 2, itemDate);
        m_table->setItem(i, 3, itemLineNo);
        m_table->setItem(i, 4, itemCode);
    }

    m_table->resizeColumnToContents(0);
    m_table->resizeColumnToContents(1);
    m_table->resizeColumnToContents(2);
    m_table->resizeColumnToContents(3);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
}
 
