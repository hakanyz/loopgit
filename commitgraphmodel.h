#ifndef COMMITGRAPHMODEL_H
#define COMMITGRAPHMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>
#include <QColor>
#include "gitmanager.h"

// Types of graph segments connecting to/from a node
struct GraphEdge {
    int fromLane;
    int toLane;
    QColor color;
};

struct GraphNode {
    int lane;
    QColor color;
    QVector<GraphEdge> edgesOut; // edges going down (to parents)
    QVector<GraphEdge> edgesIn;  // edges coming from above (from children)
};

struct GraphCommit {
    CommitInfo commit;
    GraphNode graph;
};

class CommitGraphModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        ColGraph = 0,
        ColHash,
        ColMessage,
        ColAuthor,
        ColDate,
        ColCount
    };

    explicit CommitGraphModel(QObject *parent = nullptr);

    void setCommits(const QVector<CommitInfo> &commits);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom role to fetch graph node data for the delegate
    enum CustomRoles {
        GraphNodeRole = Qt::UserRole + 1,
        IsHeadRole
    };

private:
    void computeGraph();
    QColor colorForLane(int lane) const;

    QVector<GraphCommit> m_data;
};

// Required so we can put GraphNode in QVariant
Q_DECLARE_METATYPE(GraphNode)

#endif // COMMITGRAPHMODEL_H
