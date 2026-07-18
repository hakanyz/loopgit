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
            // Bezier curve
            int startX = laneX(edge.fromLane);
            int endX   = laneX(edge.toLane);
            path.cubicTo(startX, topY + (centerY - topY)/2,
                         endX,   topY + (centerY - topY)/2,
                         endX,   centerY);
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
            // Bezier curve
            int startX = laneX(edge.fromLane);
            int endX   = laneX(edge.toLane);
            path.cubicTo(startX, centerY + (bottomY - centerY)/2,
                         endX,   centerY + (bottomY - centerY)/2,
                         endX,   bottomY);
        }
        painter->drawPath(path);
    }

    // Draw the commit node (dot)
    painter->setPen(QPen(node.color, 2));
    painter->setBrush(QColor("#252526")); // Background color of the table to make it hollow or solid
    // Let's make it solid colored
    painter->setBrush(node.color);
    painter->drawEllipse(QPoint(centerX, centerY), dotRadius, dotRadius);

    painter->restore();
}
