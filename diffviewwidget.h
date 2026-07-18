#ifndef DIFFVIEWWIDGET_H
#define DIFFVIEWWIDGET_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

// ─── DiffHighlighter ───────────────────────────────────────────────

// Removed DiffHighlighter as we use block formatting directly now

// ─── DiffEditor ────────────────────────────────────────────────────

class DiffEditor;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(DiffEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    DiffEditor *m_editor;
};

class DiffEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DiffEditor(QWidget *parent = nullptr);

    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

signals:
    void stageHunkRequested(int blockNumber);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void setupEditor();

    LineNumberArea  *m_lineNumberArea;
    int m_hoveredBlock = -1;
};

// ─── DiffViewWidget (Side-by-Side Container) ───────────────────────

class QSplitter;

class DiffViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiffViewWidget(QWidget *parent = nullptr);

    void setDiffText(const QString &diff);
    void clearDiff();

signals:
    void stageHunkRequested(const QString &hunkPatch);

private slots:
    void onLeftEditorStageHunk(int blockNumber);

private:
    DiffEditor *m_leftEditor;
    DiffEditor *m_rightEditor;
    QSplitter  *m_splitter;
    QString    m_currentDiff;
};

#endif // DIFFVIEWWIDGET_H
