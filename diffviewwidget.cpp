#include "diffviewwidget.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QFontDatabase>

// ═══════════════════════════════════════════════════════════════════
//  LineNumberArea
// ═══════════════════════════════════════════════════════════════════

LineNumberArea::LineNumberArea(DiffEditor *editor)
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
//  DiffEditor
// ═══════════════════════════════════════════════════════════════════

DiffEditor::DiffEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
{
    setupEditor();
}

void DiffEditor::setupEditor()
{
    // Monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    setFont(font);

    // Read-only
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setMouseTracking(true); // Needed for hover tracking over buttons

    // Dark-ish styling
    setStyleSheet(QStringLiteral(
        "QPlainTextEdit {"
        "  background-color: #1E1E1E;"
        "  color: #D4D4D4;"
        "  border: none;"
        "  selection-background-color: #264F78;"
        "}"
    ));

    // Line number area connections
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &DiffEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &DiffEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

// ─── Line number area ──────────────────────────────────────────────

int DiffEditor::lineNumberAreaWidth() const
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

void DiffEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DiffEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(),
                                 m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void DiffEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DiffEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
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

void DiffEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (block.text().startsWith("@@ ")) {
                QRect btnRect(viewport()->width() - 90, top + 2, 80, fontMetrics().height() + 4);
                
                QColor bgColor = (m_hoveredBlock == block.blockNumber()) ? QColor("#3085C3") : QColor("#007ACC");
                painter.setPen(Qt::NoPen);
                painter.setBrush(bgColor);
                painter.drawRoundedRect(btnRect, 4, 4);

                painter.setPen(Qt::white);
                painter.drawText(btnRect, Qt::AlignCenter, "Stage Hunk");
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}

void DiffEditor::mouseMoveEvent(QMouseEvent *event)
{
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    bool foundHover = false;

    while (block.isValid() && top <= viewport()->rect().bottom()) {
        if (block.isVisible() && bottom >= viewport()->rect().top()) {
            if (block.text().startsWith("@@ ")) {
                QRect btnRect(viewport()->width() - 90, top + 2, 80, fontMetrics().height() + 4);
                if (btnRect.contains(event->pos())) {
                    if (m_hoveredBlock != block.blockNumber()) {
                        m_hoveredBlock = block.blockNumber();
                        viewport()->setCursor(Qt::PointingHandCursor);
                        viewport()->update();
                    }
                    foundHover = true;
                    break;
                }
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }

    if (!foundHover && m_hoveredBlock != -1) {
        m_hoveredBlock = -1;
        viewport()->unsetCursor();
        viewport()->update();
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void DiffEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QTextBlock block = firstVisibleBlock();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

        while (block.isValid() && top <= viewport()->rect().bottom()) {
            if (block.isVisible() && bottom >= viewport()->rect().top()) {
                if (block.text().startsWith("@@ ")) {
                    QRect btnRect(viewport()->width() - 90, top + 2, 80, fontMetrics().height() + 4);
                    if (btnRect.contains(event->pos())) {
                        emit stageHunkRequested(block.blockNumber());
                        return; // Handled
                    }
                }
            }
            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
        }
    }
    QPlainTextEdit::mousePressEvent(event);
}

void DiffEditor::leaveEvent(QEvent *event)
{
    if (m_hoveredBlock != -1) {
        m_hoveredBlock = -1;
        viewport()->unsetCursor();
        viewport()->update();
    }
    QPlainTextEdit::leaveEvent(event);
}

// ═══════════════════════════════════════════════════════════════════
//  DiffViewWidget (Side-by-Side Container)
// ═══════════════════════════════════════════════════════════════════

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>

DiffViewWidget::DiffViewWidget(QWidget *parent)
    : QWidget(parent)
{
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left Pane
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    m_leftEditor = new DiffEditor(this);
    
    leftLayout->addWidget(m_leftEditor);
    
    // Right Pane
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    m_rightEditor = new DiffEditor(this);
    
    rightLayout->addWidget(m_rightEditor);
    
    m_splitter->addWidget(leftWidget);
    m_splitter->addWidget(rightWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_splitter);
    
    // Synchronize scroll bars
    connect(m_leftEditor->verticalScrollBar(), &QScrollBar::valueChanged,
            m_rightEditor->verticalScrollBar(), &QScrollBar::setValue);
    connect(m_rightEditor->verticalScrollBar(), &QScrollBar::valueChanged,
            m_leftEditor->verticalScrollBar(), &QScrollBar::setValue);

    connect(m_leftEditor, &DiffEditor::stageHunkRequested, this, &DiffViewWidget::onLeftEditorStageHunk);
}

void DiffViewWidget::setDiffText(const QString &diff)
{
    m_currentDiff = diff;
    m_leftEditor->clear();
    m_rightEditor->clear();

    QTextCursor leftCursor(m_leftEditor->document());
    QTextCursor rightCursor(m_rightEditor->document());

    QTextBlockFormat normalFormat;
    QTextBlockFormat addFormat;
    addFormat.setBackground(QColor(78, 201, 176, 30)); // Greenish

    QTextBlockFormat delFormat;
    delFormat.setBackground(QColor(241, 76, 76, 30)); // Reddish

    QTextBlockFormat emptyFormat;
    emptyFormat.setBackground(QColor("#252526")); // Slightly different dark empty

    QTextCharFormat hunkFormat;
    hunkFormat.setForeground(QColor("#569CD6"));
    hunkFormat.setFontWeight(QFont::Bold);

    QTextCharFormat normalCharFormat;
    normalCharFormat.setForeground(QColor("#D4D4D4"));

    bool firstLine = true;
    auto addRow = [&](const QString &lText, const QTextBlockFormat &lBf, const QTextCharFormat &lCf,
                      const QString &rText, const QTextBlockFormat &rBf, const QTextCharFormat &rCf) {
        if (!firstLine) {
            leftCursor.insertBlock(lBf, lCf);
            rightCursor.insertBlock(rBf, rCf);
        } else {
            leftCursor.setBlockFormat(lBf);
            leftCursor.setCharFormat(lCf);
            rightCursor.setBlockFormat(rBf);
            rightCursor.setCharFormat(rCf);
            firstLine = false;
        }
        leftCursor.insertText(lText.isEmpty() ? " " : lText);
        rightCursor.insertText(rText.isEmpty() ? " " : rText);
    };

    QTextBlockFormat conflictFormat;
    conflictFormat.setBackground(QColor(220, 20, 60, 150)); // Crimson
    QTextCharFormat conflictCharFormat;
    conflictCharFormat.setForeground(Qt::white);
    conflictCharFormat.setFontWeight(QFont::Bold);

    auto isConflict = [](const QString &s) {
        return s.contains("<<<<<<<") || s.contains("=======") || s.contains(">>>>>>>");
    };

    QStringList lines = diff.split('\n');
    int i = 0;
    while (i < lines.size()) {
        QString line = lines[i];

        if (line.startsWith("diff ") || line.startsWith("index ") || 
            line.startsWith("---") || line.startsWith("+++")) 
        {
            i++;
            continue;
        }

        if (line.startsWith("@@")) {
            addRow(line, normalFormat, hunkFormat, line, normalFormat, hunkFormat);
            i++;
            continue;
        }

        if (line.startsWith("-") && !line.startsWith("---")) {
            QStringList delLines, addLines;
            while (i < lines.size()) {
                QString cLine = lines[i];
                if (cLine.startsWith("-") && !cLine.startsWith("---")) {
                    delLines.append(cLine.mid(1));
                    i++;
                } else if (cLine.startsWith("+") && !cLine.startsWith("+++")) {
                    addLines.append(cLine.mid(1));
                    i++;
                } else if (cLine.startsWith("\\")) {
                    i++;
                } else {
                    break;
                }
            }
            int maxCount = qMax(delLines.size(), addLines.size());
            for (int k = 0; k < maxCount; ++k) {
                QString lText = (k < delLines.size()) ? ("-" + delLines[k]) : "";
                QTextBlockFormat lBf = emptyFormat;
                QTextCharFormat lCf = normalCharFormat;
                if (k < delLines.size()) {
                    lBf = isConflict(delLines[k]) ? conflictFormat : delFormat;
                    lCf = isConflict(delLines[k]) ? conflictCharFormat : normalCharFormat;
                }
                
                QString rText = (k < addLines.size()) ? ("+" + addLines[k]) : "";
                QTextBlockFormat rBf = emptyFormat;
                QTextCharFormat rCf = normalCharFormat;
                if (k < addLines.size()) {
                    rBf = isConflict(addLines[k]) ? conflictFormat : addFormat;
                    rCf = isConflict(addLines[k]) ? conflictCharFormat : normalCharFormat;
                }
                
                addRow(lText, lBf, lCf, rText, rBf, rCf);
            }
        } 
        else if (line.startsWith("+") && !line.startsWith("+++")) {
            QStringList addLines;
            while (i < lines.size() && lines[i].startsWith("+") && !lines[i].startsWith("+++")) {
                addLines.append(lines[i].mid(1));
                i++;
            }
            for (int k = 0; k < addLines.size(); ++k) {
                QTextBlockFormat rBf = isConflict(addLines[k]) ? conflictFormat : addFormat;
                QTextCharFormat rCf = isConflict(addLines[k]) ? conflictCharFormat : normalCharFormat;
                addRow("", emptyFormat, normalCharFormat, "+" + addLines[k], rBf, rCf);
            }
        } 
        else if (line.startsWith("\\")) {
            i++;
        }
        else {
            QString text = line;
            if (text.startsWith(" ")) text = text; 
            QTextBlockFormat bf = isConflict(text) ? conflictFormat : normalFormat;
            QTextCharFormat cf = isConflict(text) ? conflictCharFormat : normalCharFormat;
            addRow(line, bf, cf, line, bf, cf);
            i++;
        }
    }

    m_leftEditor->moveCursor(QTextCursor::Start);
    m_rightEditor->moveCursor(QTextCursor::Start);
}

void DiffViewWidget::clearDiff()
{
    m_leftEditor->clear();
    m_rightEditor->clear();
}

void DiffViewWidget::onLeftEditorStageHunk(int blockNumber)
{
    QTextBlock block = m_leftEditor->document()->findBlockByNumber(blockNumber);
    if (!block.isValid() || !block.text().startsWith("@@ ")) return;
    
    QString hunkHeader = block.text().trimmed();
    QStringList lines = m_currentDiff.split('\n');
    QString patchHeader;
    QString patchHunk;
    
    bool inHeader = true;
    bool foundHunk = false;
    
    for (const QString &line : lines) {
        if (inHeader) {
            if (line.startsWith("@@ ")) inHeader = false;
            else {
                patchHeader += line + "\n";
            }
        }
        
        if (!inHeader) {
            if (line.trimmed() == hunkHeader) {
                foundHunk = true;
                patchHunk += line + "\n";
            } else if (foundHunk) {
                if (line.startsWith("@@ ")) {
                    break; // End of this hunk
                }
                patchHunk += line + "\n";
            }
        }
    }
    
    if (foundHunk && !patchHunk.trimmed().isEmpty()) {
        emit stageHunkRequested(patchHeader + patchHunk);
    }
}

