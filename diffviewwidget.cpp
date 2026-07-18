#include "diffviewwidget.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QFontDatabase>

// ═══════════════════════════════════════════════════════════════════
//  DiffHighlighter
// ═══════════════════════════════════════════════════════════════════

DiffHighlighter::DiffHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Added lines — green
    m_addedFormat.setForeground(QColor("#4EC9B0"));
    m_addedFormat.setBackground(QColor(78, 201, 176, 30));

    // Removed lines — red
    m_removedFormat.setForeground(QColor("#F14C4C"));
    m_removedFormat.setBackground(QColor(241, 76, 76, 30));

    // Hunk headers (@@ ... @@) — cyan/blue
    m_hunkFormat.setForeground(QColor("#569CD6"));
    m_hunkFormat.setFontWeight(QFont::Bold);

    // File headers (diff --git, ---/+++ lines) — yellow/gold
    m_headerFormat.setForeground(QColor("#DCDCAA"));
    m_headerFormat.setFontWeight(QFont::Bold);

    // Meta lines (index, similarity, etc.) — gray
    m_metaFormat.setForeground(QColor("#808080"));
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty()) return;

    QChar first = text.at(0);

    if (text.startsWith(QStringLiteral("diff --git")) ||
        text.startsWith(QStringLiteral("--- "))       ||
        text.startsWith(QStringLiteral("+++ ")))
    {
        setFormat(0, text.length(), m_headerFormat);
    }
    else if (text.startsWith(QStringLiteral("@@"))) {
        setFormat(0, text.length(), m_hunkFormat);
    }
    else if (first == '+') {
        setFormat(0, text.length(), m_addedFormat);
    }
    else if (first == '-') {
        setFormat(0, text.length(), m_removedFormat);
    }
    else if (text.startsWith(QStringLiteral("index "))     ||
             text.startsWith(QStringLiteral("similarity")) ||
             text.startsWith(QStringLiteral("rename"))     ||
             text.startsWith(QStringLiteral("new file"))   ||
             text.startsWith(QStringLiteral("deleted file")))
    {
        setFormat(0, text.length(), m_metaFormat);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  LineNumberArea
// ═══════════════════════════════════════════════════════════════════

LineNumberArea::LineNumberArea(DiffViewWidget *editor)
    : QWidget(editor), m_editor(editor)
{}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

// ═══════════════════════════════════════════════════════════════════
//  DiffViewWidget
// ═══════════════════════════════════════════════════════════════════

DiffViewWidget::DiffViewWidget(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_highlighter(nullptr)
    , m_lineNumberArea(new LineNumberArea(this))
{
    setupEditor();
}

void DiffViewWidget::setupEditor()
{
    // Monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    setFont(font);

    // Read-only
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Dark-ish styling
    setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background-color: #1E1E1E;"
        "  color: #D4D4D4;"
        "  border: none;"
        "  selection-background-color: #264F78;"
        "}"
    ));

    // Highlighter
    m_highlighter = new DiffHighlighter(document());

    // Line number area connections
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &DiffViewWidget::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &DiffViewWidget::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

void DiffViewWidget::setDiffText(const QString &diff)
{
    setPlainText(diff);
    moveCursor(QTextCursor::Start);
}

void DiffViewWidget::clearDiff()
{
    clear();
}

// ─── Line number area ──────────────────────────────────────────────

int DiffViewWidget::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void DiffViewWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DiffViewWidget::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(),
                                 m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void DiffViewWidget::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DiffViewWidget::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#252526"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber  = block.blockNumber();
    int top    = qRound(blockBoundingGeometry(block)
                            .translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(QColor("#858585"));
    QFont numFont = font();
    numFont.setPointSize(font().pointSize() - 1);
    painter.setFont(numFont);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top,
                             m_lineNumberArea->width() - 4,
                             fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
