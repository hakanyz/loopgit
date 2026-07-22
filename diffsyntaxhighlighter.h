#ifndef DIFFSYNTAXHIGHLIGHTER_H
#define DIFFSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class DiffSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DiffSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
};

#endif // DIFFSYNTAXHIGHLIGHTER_H
 
