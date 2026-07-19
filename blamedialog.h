#ifndef BLAMEDIALOG_H
#define BLAMEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "gitmanager.h"

class BlameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BlameDialog(const QString &filePath, const QVector<BlameLine> &blameData, QWidget *parent = nullptr);

private:
    QTableWidget *m_table;
};

#endif // BLAMEDIALOG_H
