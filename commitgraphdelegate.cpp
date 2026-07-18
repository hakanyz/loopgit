#include "commitgraphdelegate.h"
#include "commitgraphmodel.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

CommitGraphDelegate::CommitGraphDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize CommitGraphDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(qMax(s.height(), 24)); // Minimum height for decent graph nodes
    
    // We need enough width for all active lanes. The model knows the lane count implicitly through the max lane used, 
    // but we can just give it a fixed decent width. The header resizing will handle the rest.
    s.setWidth(100); 
    return s;
}

void CommitGraphDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == CommitGraphModel::ColMessage) {
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#062f4a")); // Exact match to stylesheet
        }

        QStringList refs = index.data(Qt::UserRole + 1).toStringList();
        QString msg = index.data(Qt::DisplayRole).toString();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect rect = option.rect;

        int x = rect.left() + 4;
        QFontMetrics fm(painter->font());
        
        for (const QString &ref : refs) {
            QRect badgeRect(x, rect.top() + (rect.height() - 16) / 2, fm.horizontalAdvance(ref) + 12, 16);
            
            QColor bgColor = QColor("#0E639C"); // Default blue
            if (ref == "HEAD" || ref.startsWith("HEAD ->")) bgColor = QColor("#238636"); // Green
            else if (ref.startsWith("origin/")) bgColor = QColor("#DA3633"); // Red for remote
            
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(badgeRect, 4, 4);

            painter->setPen(Qt::white);
            painter->drawText(badgeRect, Qt::AlignCenter, ref);

            x += badgeRect.width() + 6;
        }

        QRect textRect = rect;
        textRect.setLeft(x);
        painter->setPen(option.state & QStyle::State_Selected ? option.palette.highlightedText().color() : option.palette.text().color());
        
        // Use elided text if too long
        QString elidedMsg = fm.elidedText(msg, Qt::ElideRight, textRect.width() - 4);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedMsg);

        painter->restore();
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() != CommitGraphModel::ColGraph)
        return;

    QVariant var = index.data(CommitGraphModel::GraphNodeRole);
    if (!var.isValid())
        return;

    GraphNode node = var.value<GraphNode>();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const int laneWidth = 16;
    const int dotRadius = 4;
    const int centerX = option.rect.left() + (node.lane * laneWidth) + (laneWidth / 2) + 4;
    const int centerY = option.rect.center().y();
    const int topY    = option.rect.top();
    const int bottomY = option.rect.bottom();

    // Helper to get X coordinate for a lane
    auto laneX = [&](int l) {
        return option.rect.left() + (l * laneWidth) + (laneWidth / 2) + 4;
    };

    // Draw incoming edges (from children above)
    QList<GraphEdge> sortedInEdges = node.edgesIn;
    std::sort(sortedInEdges.begin(), sortedInEdges.end(), [](const GraphEdge &a, const GraphEdge &b) {
        bool aIsStraight = (a.fromLane == a.toLane);
        bool bIsStraight = (b.fromLane == b.toLane);
        return aIsStraight && !bIsStraight;
    });

    for (const GraphEdge &edge : sortedInEdges) {
        painter->setPen(QPen(edge.color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        
        QPainterPath path;
        path.moveTo(laneX(edge.fromLane), topY);
        
        if (edge.fromLane == edge.toLane) {
            // Straight line
            path.lineTo(laneX(edge.toLane), centerY);
        } else {
            // Angular/Straight diagonal curve
            int startX = laneX(edge.fromLane);
            int endX   = laneX(edge.toLane);
            path.lineTo(startX, topY + 4);
            path.lineTo(endX, centerY - 4);
            path.lineTo(endX, centerY);
        }
        painter->drawPath(path);
    }

    // Draw outgoing edges (to parents below)
    QList<GraphEdge> sortedOutEdges = node.edgesOut;
    std::sort(sortedOutEdges.begin(), sortedOutEdges.end(), [](const GraphEdge &a, const GraphEdge &b) {
        bool aIsStraight = (a.fromLane == a.toLane);
        bool bIsStraight = (b.fromLane == b.toLane);
        return aIsStraight && !bIsStraight; // true if a is straight and b is not
    });

    for (const GraphEdge &edge : sortedOutEdges) {
        painter->setPen(QPen(edge.color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        
        QPainterPath path;
        path.moveTo(laneX(edge.fromLane), centerY);
        
        if (edge.fromLane == edge.toLane) {
            // Straight line
            path.lineTo(laneX(edge.toLane), bottomY);
        } else {
            // Angular/Straight diagonal curve
            int startX = laneX(edge.fromLane);
            int endX   = laneX(edge.toLane);
            path.lineTo(startX, centerY + 4);
            path.lineTo(endX, bottomY - 4);
            path.lineTo(endX, bottomY);
        }
        painter->drawPath(path);
    }

    // Draw the commit node (dot) - Hollow
    painter->setPen(QPen(node.color, 2));
    painter->setBrush(QColor("#1E1E1E")); // Dark background to make it hollow
    painter->drawEllipse(QPoint(centerX, centerY), dotRadius + 1, dotRadius + 1);

    painter->restore();
}
