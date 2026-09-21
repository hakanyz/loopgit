#include "commitgraphmodel.h"
#include <QBrush>
#include <QDateTime>
#include <QColor>
#include <QSet>

static const QList<QColor> GRAPH_COLORS = {
    QColor("#388BFD"), // Blue (GitHub primary)
    QColor("#34D058"), // Vibrant Green
    QColor("#F85149"), // Coral Red
    QColor("#D29922"), // Warm Amber
    QColor("#BC8CFF"), // Electric Purple
    QColor("#39C5BB"), // Cyan / Teal
    QColor("#F778BA")  // Pink
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

QColor CommitGraphModel::colorForLane(int lane, bool isStash) const
{
    if (isStash) {
        return QColor("#8C959F"); // Muted Slate Gray for stashes
    }
    static const QList<QColor> LANE_COLORS = {
        QColor("#388BFD"), // Lane 0: GitHub Blue (Primary Trunk - always solid Blue)
        QColor("#F0883E"), // Lane 1: GitHub Coral / Orange (Feature branch - always Orange)
        QColor("#8957E5"), // Lane 2: Purple (2nd branch)
        QColor("#2EA043"), // Lane 3: Green (3rd branch)
        QColor("#D29922"), // Lane 4: Warm Amber
        QColor("#39C5BB"), // Lane 5: Cyan / Teal
        QColor("#F778BA")  // Lane 6: Pink
    };
    return LANE_COLORS[lane % LANE_COLORS.size()];
}

void CommitGraphModel::computeGraph()
{
    struct ActiveTrack {
        QString commitId;
        bool isStash = false;
    };

    const QColor STASH_COLOR("#8C959F"); // Muted Slate Gray for stashes

    QVector<ActiveTrack> activeTracks;

    for (int i = 0; i < m_data.size(); ++i) {
        GraphCommit &gc = m_data[i];
        const QString &id = gc.commit.id;

        // Detect if this commit is a stash or an auxiliary stash commit
        bool isStashCommit = false;
        for (const QString &r : gc.commit.refs) {
            if (r.startsWith("stash@{")) {
                isStashCommit = true;
                break;
            }
        }
        if (gc.commit.summary.startsWith("index on ") || gc.commit.summary.startsWith("untracked files on ")) {
            isStashCommit = true;
        }

        // Find which lane this commit belongs to
        int lane = -1;
        for (int l = 0; l < activeTracks.size(); ++l) {
            if (!activeTracks[l].commitId.isEmpty() && activeTracks[l].commitId == id) {
                lane = l;
                break;
            }
        }

        if (lane == -1) {
            // New branch head or stash tip
            if (isStashCommit) {
                // If it's a stash at the very top, keep lane 0 reserved for HEAD if activeTracks is empty
                if (activeTracks.isEmpty()) {
                    activeTracks.append(ActiveTrack{}); // lane 0 for HEAD
                    activeTracks.append(ActiveTrack{}); // lane 1 for stash
                    lane = 1;
                } else {
                    for (int l = 1; l < activeTracks.size(); ++l) {
                        if (activeTracks[l].commitId.isEmpty()) {
                            lane = l;
                            break;
                        }
                    }
                    if (lane == -1) {
                        lane = activeTracks.size();
                        activeTracks.append(ActiveTrack{});
                    }
                }
            } else {
                for (int l = 0; l < activeTracks.size(); ++l) {
                    if (activeTracks[l].commitId.isEmpty()) {
                        lane = l;
                        break;
                    }
                }
                if (lane == -1) {
                    lane = activeTracks.size();
                    activeTracks.append(ActiveTrack{});
                }
            }
            activeTracks[lane] = { id, isStashCommit };
        } else if (isStashCommit) {
            activeTracks[lane].isStash = true;
        }

        gc.graph.lane = lane;
        gc.graph.isStash = activeTracks[lane].isStash;
        gc.graph.color = colorForLane(lane, gc.graph.isStash);
        gc.graph.isMerge = (gc.commit.parentIds.size() > 1);

        // Record which lanes were incoming to this cell
        QSet<int> incomingLanes;
        for (int l = 0; l < activeTracks.size(); ++l) {
            if (!activeTracks[l].commitId.isEmpty()) {
                incomingLanes.insert(l);
            }
        }

        bool isCurrentStash = activeTracks[lane].isStash;
        activeTracks[lane].commitId.clear();

        // Calculate outgoing edges to parents
        if (!gc.commit.parentIds.isEmpty()) {
            if (isCurrentStash) {
                // A stash is a transient local snapshot (leaf node).
                // It should connect directly to the main trunk (lane 0) without keeping
                // an active track open across distant historical commits.
                int targetLane = 0;
                if (activeTracks.isEmpty()) {
                    activeTracks.append(ActiveTrack{ gc.commit.parentIds[0], false });
                    targetLane = 0;
                } else {
                    // If parent is already in an active track, use it; otherwise connect to trunk lane 0
                    for (int l = 0; l < activeTracks.size(); ++l) {
                        if (activeTracks[l].commitId == gc.commit.parentIds[0]) {
                            targetLane = l;
                            break;
                        }
                    }
                }

                GraphEdge edge;
                edge.fromLane = lane;
                edge.toLane = targetLane;
                edge.color = STASH_COLOR;
                gc.graph.edgesOut.append(edge);
            } else {
                for (int pIdx = 0; pIdx < gc.commit.parentIds.size(); ++pIdx) {
                    const QString &parentId = gc.commit.parentIds[pIdx];
                    int pLane = -1;
                    for (int l = 0; l < activeTracks.size(); ++l) {
                        if (activeTracks[l].commitId == parentId) {
                            pLane = l;
                            break;
                        }
                    }

                    if (pLane == -1) {
                        // Parent not seen yet
                        if (pIdx == 0) {
                            pLane = lane;
                            activeTracks[pLane] = { parentId, false };
                        } else {
                            // Merge parent or branch fork: allocate new lane
                            for (int l = 0; l < activeTracks.size(); ++l) {
                                if (activeTracks[l].commitId.isEmpty()) {
                                    pLane = l;
                                    break;
                                }
                            }
                            if (pLane == -1) {
                                pLane = activeTracks.size();
                                activeTracks.append(ActiveTrack{});
                            }
                            activeTracks[pLane] = { parentId, false };
                        }
                    }

                    GraphEdge edge;
                    edge.fromLane = lane;
                    edge.toLane = pLane;
                    if (edge.fromLane == edge.toLane) {
                        edge.color = colorForLane(edge.fromLane);
                    } else {
                        edge.color = colorForLane(qMax(edge.fromLane, edge.toLane));
                    }
                    gc.graph.edgesOut.append(edge);
                }
            }
        }

        // Draw passthrough lines for all other active lanes
        for (int l = 0; l < activeTracks.size(); ++l) {
            if (!activeTracks[l].commitId.isEmpty() && l != lane && incomingLanes.contains(l)) {
                GraphEdge edge;
                edge.fromLane = l;
                edge.toLane = l;
                edge.color = activeTracks[l].isStash ? STASH_COLOR : colorForLane(l);
                gc.graph.edgesOut.append(edge);
            }
        }

        // --- Lane Compaction ---
        // Collapse empty gaps so branches smoothly shift left into available lanes
        int lastNonEmpty = -1;
        for (int l = 0; l < activeTracks.size(); ++l) {
            if (!activeTracks[l].commitId.isEmpty()) {
                lastNonEmpty = l;
            }
        }

        bool needsCompaction = false;
        for (int l = 0; l < lastNonEmpty; ++l) {
            if (activeTracks[l].commitId.isEmpty()) {
                needsCompaction = true;
                break;
            }
        }

        if (needsCompaction) {
            QVector<int> laneMap(activeTracks.size(), -1);
            QVector<ActiveTrack> compacted;
            for (int l = 0; l < activeTracks.size(); ++l) {
                if (!activeTracks[l].commitId.isEmpty()) {
                    int newL = compacted.size();
                    laneMap[l] = newL;
                    compacted.append(activeTracks[l]);
                }
            }

            // Remap edgesOut targets and update colors deterministically
            for (auto &edge : gc.graph.edgesOut) {
                if (edge.toLane >= 0 && edge.toLane < laneMap.size() && laneMap[edge.toLane] != -1) {
                    edge.toLane = laneMap[edge.toLane];
                }
                bool isStashEdge = (gc.graph.isStash || (edge.toLane < compacted.size() && compacted[edge.toLane].isStash));
                if (isStashEdge) {
                    edge.color = STASH_COLOR;
                } else if (edge.fromLane == edge.toLane) {
                    edge.color = colorForLane(edge.fromLane);
                } else {
                    edge.color = colorForLane(qMax(edge.fromLane, edge.toLane));
                }
            }

            activeTracks = compacted;
        } else {
            // Trim trailing empty slots
            while (!activeTracks.isEmpty() && activeTracks.last().commitId.isEmpty()) {
                activeTracks.removeLast();
            }
        }
    }

    // Compute edgesIn from previous row's edgesOut
    for (int i = 1; i < m_data.size(); ++i) {
        GraphCommit &gc = m_data[i];
        const GraphCommit &prev = m_data[i - 1];

        for (const GraphEdge &edge : prev.graph.edgesOut) {
            GraphEdge inEdge;
            inEdge.fromLane = edge.fromLane;
            inEdge.toLane   = edge.toLane;
            inEdge.color    = edge.color;
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
            case ColBranches:return QVariant(); // Drawn by delegate
            case ColMessage: return gc.commit.summary;
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
    else if (role == Qt::UserRole + 1 && (index.column() == ColBranches || index.column() == ColMessage)) {
        return gc.commit.refs;
    }
    else if (role == Qt::ToolTipRole && index.column() == ColBranches) {
        if (gc.commit.refs.isEmpty()) return QVariant();
        
        QStringList cleanedRefs;
        for (const QString &r : gc.commit.refs) {
            if (r == "HEAD" || r.startsWith("HEAD ->")) continue; // Skip redundant HEAD
            cleanedRefs.append(r);
        }
        if (cleanedRefs.isEmpty()) return QVariant();
        return cleanedRefs.join("\n");
    }
    else if (role == Qt::ForegroundRole) {
        return QColor("#D4D4D4");
    }
    else if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColMessage) {
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignCenter);
    }

    return QVariant();
}

QVariant CommitGraphModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case ColGraph:   return QStringLiteral("Graph");
                case ColHash:    return QStringLiteral("Hash");
                case ColBranches:return QStringLiteral("Branches");
                case ColMessage: return QStringLiteral("Message");
                case ColAuthor:  return QStringLiteral("Author");
                case ColDate:    return QStringLiteral("Date");
            }
        } else if (role == Qt::TextAlignmentRole) {
            if (section == ColMessage) {
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
            }
            return static_cast<int>(Qt::AlignCenter);
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
 
