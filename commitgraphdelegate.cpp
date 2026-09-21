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
    if (index.column() == CommitGraphModel::ColBranches || index.column() == CommitGraphModel::ColMessage) {
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor("#062f4a")); // Exact match to stylesheet
        }
        
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (index.column() == CommitGraphModel::ColMessage) {
            QString msg = index.data(Qt::DisplayRole).toString();
            QStringList refs = index.data(Qt::UserRole + 1).toStringList();
            
            QRect rect = option.rect;
            int currentX = rect.left() + 4;
            int y = rect.top() + (rect.height() - 20) / 2; // 20 is badge height
            
            painter->setFont(QFont(painter->font().family(), 8, QFont::Bold)); // Slightly larger font for branch names
            
            for (const QString &ref : refs) {
                if (ref == "HEAD" || ref.startsWith("HEAD ->")) continue;
                
                QColor bgColor;
                if (ref.startsWith("origin/")) {
                    bgColor = QColor("#DA3633"); // Remote (Red)
                } else if (ref.startsWith("tag: ")) {
                    bgColor = QColor("#E2C08D"); // Tag (Yellow)
                } else if (ref.startsWith("stash@{")) {
                    bgColor = QColor("#6E7681"); // Stash (Slate Gray)
                } else {
                    bgColor = QColor("#0E639C"); // Local (Blue)
                }
                
                QString text = ref;
                if (text.startsWith("tag: ")) text = text.mid(5); // Remove "tag: " prefix for display
                
                QFontMetrics fm(painter->font());
                int textWidth = fm.horizontalAdvance(text);
                int badgeWidth = textWidth + 12; // 6px padding on each side
                
                QRect badgeRect(currentX, y, badgeWidth, 20);
                
                // Draw badge if it fits (at least partially)
                if (currentX + badgeWidth < rect.right() - 20) {
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(bgColor);
                    painter->drawRoundedRect(badgeRect, 4, 4);
                    
                    painter->setPen(ref.startsWith("tag: ") ? QColor("#1E1E1E") : Qt::white);
                    painter->drawText(badgeRect, Qt::AlignCenter, text);
                    
                    currentX += badgeWidth + 4;
                }
            }
            
            QRect textRect = option.rect;
            textRect.setLeft(currentX + 4);
            
            painter->setFont(option.font); // Restore normal font for message
            painter->setPen(option.palette.color(QPalette::Text));
            
            // Elide text if it's too long
            QFontMetrics fmMsg(painter->font());
            QString elidedText = fmMsg.elidedText(msg, Qt::ElideRight, textRect.width() - 8);
            
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
        }

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
        qreal toX = laneX(edge.toLane);
        
        if (edge.fromLane == edge.toLane) {
            // Straight line
            path.moveTo(toX, topY);
            path.lineTo(toX, centerY);
        } else {
            // Second half of C1 continuous cubic spline (entering this row)
            qreal fromX = laneX(edge.fromLane);
            qreal midX  = (fromX + toX) * 0.5;
            qreal dy    = centerY - topY;
            path.moveTo(midX, topY);
            path.cubicTo(0.25 * fromX + 0.75 * toX, topY + dy * 0.25,
                         toX,                       centerY - dy * 0.5,
                         toX,                       centerY);
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
        qreal fromX = laneX(edge.fromLane);
        
        if (edge.fromLane == edge.toLane) {
            // Straight line
            path.moveTo(fromX, centerY);
            path.lineTo(fromX, bottomY);
        } else {
            // First half of C1 continuous cubic spline (leaving this row)
            qreal toX  = laneX(edge.toLane);
            qreal midX = (fromX + toX) * 0.5;
            qreal dy   = bottomY - centerY;
            path.moveTo(fromX, centerY);
            path.cubicTo(fromX,                     centerY + dy * 0.5,
                         0.75 * fromX + 0.25 * toX, centerY + dy * 0.75,
                         midX,                      bottomY);
        }
        painter->drawPath(path);
    }

    bool isMerge = node.isMerge;
    
    QModelIndex msgIndex = index.siblingAtColumn(CommitGraphModel::ColMessage);
    QStringList refs = msgIndex.data(Qt::UserRole + 1).toStringList();
    bool isHead = false;
    for (const QString &r : refs) {
        if (r == "HEAD" || r.startsWith("HEAD ->")) {
            isHead = true;
            break;
        }
    }

    if (isHead) {
        // Draw a subtle glow for HEAD
        painter->setPen(Qt::NoPen);
        QColor glowColor = node.color;
        glowColor.setAlpha(80);
        painter->setBrush(glowColor);
        painter->drawEllipse(QPoint(centerX, centerY), dotRadius + 4, dotRadius + 4);
        
        painter->setPen(QPen(node.color, 3));
        painter->setBrush(QColor("#1E1E1E")); // Dark background
        painter->drawEllipse(QPoint(centerX, centerY), dotRadius + 1, dotRadius + 1);
    } else if (node.isStash) {
        // Stash: diamond marker in muted gray (GitExtensions style)
        painter->setPen(QPen(node.color, 2));
        painter->setBrush(node.color);
        QPolygon diamond;
        diamond << QPoint(centerX, centerY - 4)
                << QPoint(centerX + 4, centerY)
                << QPoint(centerX, centerY + 4)
                << QPoint(centerX - 4, centerY);
        painter->drawPolygon(diamond);
    } else if (isMerge) {
        // Solid fill for merge commits
        painter->setPen(QPen(node.color, 2));
        painter->setBrush(node.color); 
        painter->drawEllipse(QPoint(centerX, centerY), dotRadius + 1, dotRadius + 1);
    } else {
        // Standard hollow dot
        painter->setPen(QPen(node.color, 2));
        painter->setBrush(QColor("#1E1E1E"));
        painter->drawEllipse(QPoint(centerX, centerY), dotRadius + 1, dotRadius + 1);
    }

    painter->restore();
}
 
