#ifndef DIFFVIEWWIDGET_H
#define DIFFVIEWWIDGET_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

// ─── DiffHighlighter ───────────────────────────────────────────────

class DiffHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DiffHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_addedFormat;
    QTextCharFormat m_removedFormat;
    QTextCharFormat m_hunkFormat;
    QTextCharFormat m_headerFormat;
    QTextCharFormat m_metaFormat;
};

// ─── LineNumberArea (helper) ───────────────────────────────────────

class DiffViewWidget;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(DiffViewWidget *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    DiffViewWidget *m_editor;
};

// ─── DiffViewWidget ────────────────────────────────────────────────

class DiffViewWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DiffViewWidget(QWidget *parent = nullptr);

    void setDiffText(const QString &diff);
    void clearDiff();

    // Line number area support
    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void setupEditor();

    DiffHighlighter *m_highlighter;
    LineNumberArea  *m_lineNumberArea;
};

#endif // DIFFVIEWWIDGET_H
