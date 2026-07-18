#include "commitgraphmodel.h"
#include <QBrush>

static const QList<QColor> GRAPH_COLORS = {
    QColor("#F14C4C"), // Red
    QColor("#3794FF"), // Blue
    QColor("#73C991"), // Green
    QColor("#E2C08D"), // Yellow
    QColor("#C586C0"), // Purple
    QColor("#D16969"), // Orange
    QColor("#4EC9B0")  // Cyan
};

CommitGraphModel::CommitGraphModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void CommitGraphModel::setCommits(const QVector<CommitInfo> &commits)
{
    beginResetModel();
    m_data.clear();
    m_data.reserve(commits.size());

    for (const auto &ci : commits) {
        GraphCommit gc;
        gc.commit = ci;
        m_data.append(gc);
    }

    computeGraph();
    endResetModel();
}

QColor CommitGraphModel::colorForLane(int lane) const
{
    return GRAPH_COLORS[lane % GRAPH_COLORS.size()];
}

void CommitGraphModel::computeGraph()
{
    // Active branches (lanes). The string is the SHA1 of the commit expected in this lane.
    QVector<QString> activeLanes;

    for (int i = 0; i < m_data.size(); ++i) {
        GraphCommit &gc = m_data[i];
        QString id = gc.commit.id;

        // Find which lane this commit belongs to
        int lane = activeLanes.indexOf(id);

        if (lane == -1) {
            // New branch head. Find first empty lane or append.
            lane = activeLanes.indexOf(QString());
            if (lane == -1) {
                lane = activeLanes.size();
                activeLanes.append(id);
            } else {
                activeLanes[lane] = id;
            }
        }

        gc.graph.lane = lane;
        gc.graph.color = colorForLane(lane);

        // Calculate outgoing edges (to parents)
        // First parent stays in the same lane (usually).
        // Additional parents (merges) get new lanes or find existing.
        
        // Remove current commit from active lanes
        activeLanes[lane] = QString();

        if (!gc.commit.parentIds.isEmpty()) {
            for (int pIdx = 0; pIdx < gc.commit.parentIds.size(); ++pIdx) {
                QString parentId = gc.commit.parentIds[pIdx];
                int pLane = activeLanes.indexOf(parentId);

                if (pLane == -1) {
                    // Parent not seen yet. 
                    if (pIdx == 0) {
                        // First parent takes our lane
                        pLane = lane;
                        activeLanes[pLane] = parentId;
                    } else {
                        // Other parents take empty lanes or append
                        pLane = activeLanes.indexOf(QString());
                        if (pLane == -1) {
                            pLane = activeLanes.size();
                            activeLanes.append(parentId);
                        } else {
                            activeLanes[pLane] = parentId;
                        }
                    }
                }

                GraphEdge edge;
                edge.fromLane = lane;
                edge.toLane = pLane;
                edge.color = colorForLane(pLane); // Edge takes the color of the parent's lane
                gc.graph.edgesOut.append(edge);
            }
        }

        // Draw passthrough lines for all other active lanes
        for (int l = 0; l < activeLanes.size(); ++l) {
            if (!activeLanes[l].isEmpty() && l != lane) {
                GraphEdge edge;
                edge.fromLane = l;
                edge.toLane = l;
                edge.color = colorForLane(l);
                gc.graph.edgesOut.append(edge);
            }
        }
    }

    // Now compute edgesIn by looking at previous row's edgesOut
    for (int i = 1; i < m_data.size(); ++i) {
        GraphCommit &gc = m_data[i];
        const GraphCommit &prev = m_data[i - 1];

        for (const GraphEdge &edge : prev.graph.edgesOut) {
            GraphEdge inEdge;
            inEdge.fromLane = edge.fromLane;
            inEdge.toLane = edge.toLane;
            inEdge.color = edge.color;
            gc.graph.edgesIn.append(inEdge);
        }
    }
}

int CommitGraphModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_data.size();
}

int CommitGraphModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant CommitGraphModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const GraphCommit &gc = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColGraph:   return QVariant(); // Drawn by delegate
            case ColHash:    return gc.commit.shortId;
            case ColMessage: {
                return gc.commit.summary;
            }
            case ColAuthor:  return gc.commit.authorName;
            case ColDate:    return gc.commit.date.toString("yyyy-MM-dd hh:mm");
        }
    }
    else if (role == Qt::UserRole) {
        if (index.column() == ColHash) return gc.commit.id;
        if (index.column() == ColMessage) return gc.commit.message;
        if (index.column() == ColAuthor) return gc.commit.authorName + " <" + gc.commit.authorEmail + ">";
        if (index.column() == ColDate) return gc.commit.date;
    }
    else if (role == GraphNodeRole && index.column() == ColGraph) {
        return QVariant::fromValue(gc.graph);
    }
    else if (role == Qt::UserRole + 1 && index.column() == ColMessage) {
        return gc.commit.refs;
    }
    else if (role == Qt::ForegroundRole) {
        return QColor("#D4D4D4");
    }

    return QVariant();
}

QVariant CommitGraphModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ColGraph:   return QStringLiteral("Graph");
            case ColHash:    return QStringLiteral("Hash");
            case ColMessage: return QStringLiteral("Message");
            case ColAuthor:  return QStringLiteral("Author");
            case ColDate:    return QStringLiteral("Date");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
