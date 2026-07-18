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

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void setupEditor();

    LineNumberArea  *m_lineNumberArea;
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

private:
    DiffEditor *m_leftEditor;
    DiffEditor *m_rightEditor;
    QSplitter  *m_splitter;
};

#endif // DIFFVIEWWIDGET_H
