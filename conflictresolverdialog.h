#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

class QVBoxLayout;
class QScrollArea;

struct ConflictBlock {
    bool isConflict = false;
    QString normalText;
    QString oursText;
    QString theirsText;
    int resolvedChoice = 0; // 0=unresolved, 1=ours, 2=theirs, 3=both
    
    // For UI updating
    class QWidget* widget = nullptr;
};

class ConflictResolverDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConflictResolverDialog(const QString &repoPath, const QString &filePath, QWidget *parent = nullptr);

signals:
    void resolved();

private:
    void parseFile();
    void setupUi();
    void saveAndResolve();
    void updateSaveButtonState();

    QString m_repoPath;
    QString m_filePath;
    QString m_fullPath;
    QVector<ConflictBlock> m_blocks;

    QVBoxLayout *m_blocksLayout;
    class QPushButton *m_saveBtn;
};
 
