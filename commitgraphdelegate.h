#ifndef COMMITGRAPHDELEGATE_H
#define COMMITGRAPHDELEGATE_H

#include <QStyledItemDelegate>

class CommitGraphDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommitGraphDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // COMMITGRAPHDELEGATE_H
