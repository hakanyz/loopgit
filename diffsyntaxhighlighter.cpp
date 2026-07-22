#include "diffsyntaxhighlighter.h"

DiffSyntaxHighlighter::DiffSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // We only set foreground colors, leaving background colors intact.
    // Keywords (C++, Python, JS, etc.)
    keywordFormat.setForeground(QColor("#569cd6")); // Blue keywords
    QStringList keywordPatterns;
    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
                    << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
                    << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
                    << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
                    << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
                    << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
                    << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
                    << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
                    << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
                    << "\\bvoid\\b" << "\\bvolatile\\b" << "\\bbool\\b"
                    << "\\bif\\b" << "\\belse\\b" << "\\bwhile\\b" << "\\bfor\\b"
                    << "\\breturn\\b" << "\\btrue\\b" << "\\bfalse\\b"
                    << "\\bvar\\b" << "\\blet\\b" << "\\bfunction\\b"
                    << "\\bdef\\b" << "\\bimport\\b" << "\\bfrom\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Qt Classes / generic Classes (starting with uppercase)
    classFormat.setForeground(QColor("#4ec9b0")); // Teal classes
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Single line comments
    singleLineCommentFormat.setForeground(QColor("#6A9955")); // Green comments (VSCode dark)
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("#[^\n]*"); // Python style
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Strings
    quotationFormat.setForeground(QColor("#ce9178")); // Orange strings
    rule.pattern = QRegularExpression("\".*?\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    rule.pattern = QRegularExpression("'.*?'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Functions
    functionFormat.setForeground(QColor("#dcdcaa")); // Yellow functions
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Multi-line comments
    multiLineCommentFormat.setForeground(QColor("#6A9955"));
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void DiffSyntaxHighlighter::highlightBlock(const QString &text)
{
    // Ignore lines that are patch metadata headers
    if (text.startsWith("@@ ") || text.startsWith("diff ") || text.startsWith("index ")) {
        // We can explicitly color these blue
        QTextCharFormat diffHeaderFormat;
        diffHeaderFormat.setForeground(QColor("#569cd6"));
        setFormat(0, text.length(), diffHeaderFormat);
        return;
    }

    // Apply all syntax highlighting rules
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Multi-line comment processing
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
 
