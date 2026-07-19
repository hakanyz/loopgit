#include "conflictresolverdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include "gitmanager.h" // We need to include this or resolve via RepoWidget later. We can just emit resolved() and let RepoWidget call m_git->stageFile().

ConflictResolverDialog::ConflictResolverDialog(const QString &repoPath, const QString &filePath, QWidget *parent)
    : QDialog(parent), m_repoPath(repoPath), m_filePath(filePath)
{
    m_fullPath = QDir(m_repoPath).filePath(m_filePath);
    setWindowTitle(QStringLiteral("Resolve Conflicts - %1").arg(m_filePath));
    resize(1000, 700);

    parseFile();
    setupUi();
}

void ConflictResolverDialog::parseFile()
{
    QFile file(m_fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    ConflictBlock currentBlock;
    bool inConflict = false;
    int conflictPhase = 0; // 0=normal, 1=ours, 2=theirs

    while (!in.atEnd()) {
        QString line = in.readLine() + "\n";
        
        if (line.startsWith("<<<<<<<")) {
            // Push previous normal block
            if (!currentBlock.normalText.isEmpty()) {
                currentBlock.isConflict = false;
                m_blocks.append(currentBlock);
                currentBlock = ConflictBlock();
            }
            inConflict = true;
            conflictPhase = 1;
            currentBlock.isConflict = true;
        } else if (line.startsWith("=======") && inConflict) {
            conflictPhase = 2;
        } else if (line.startsWith(">>>>>>>") && inConflict) {
            m_blocks.append(currentBlock);
            currentBlock = ConflictBlock();
            inConflict = false;
            conflictPhase = 0;
        } else {
            if (conflictPhase == 0) {
                currentBlock.normalText += line;
            } else if (conflictPhase == 1) {
                // If it's a base block (|||||||), we just ignore it for now or append to ours.
                if (line.startsWith("|||||||")) {
                    // skip base for simplicity
                } else {
                    currentBlock.oursText += line;
                }
            } else if (conflictPhase == 2) {
                currentBlock.theirsText += line;
            }
        }
    }
    
    if (!currentBlock.normalText.isEmpty() || inConflict) {
        m_blocks.append(currentBlock);
    }
}

void ConflictResolverDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *headerLabel = new QLabel(QStringLiteral("<b>Resolving Conflicts:</b> %1").arg(m_filePath));
    headerLabel->setStyleSheet("padding: 10px; background-color: #062f4a; color: white; font-size: 14px;");
    mainLayout->addWidget(headerLabel);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; }");

    QWidget *contentWidget = new QWidget;
    m_blocksLayout = new QVBoxLayout(contentWidget);
    m_blocksLayout->setSpacing(0);
    m_blocksLayout->setContentsMargins(10, 10, 10, 10);

    for (int i = 0; i < m_blocks.size(); ++i) {
        ConflictBlock &block = m_blocks[i];
        
        if (!block.isConflict) {
            if (!block.normalText.trimmed().isEmpty()) {
                QTextEdit *te = new QTextEdit;
                te->setReadOnly(true);
                te->setPlainText(block.normalText);
                te->setStyleSheet("QTextEdit { background-color: transparent; border: none; font-family: Consolas, monospace; color: #CCCCCC; }");
                
                // Adjust height
                int lines = block.normalText.count('\n');
                te->setFixedHeight((lines + 1) * 20); // rough estimate
                
                m_blocksLayout->addWidget(te);
            }
        } else {
            QWidget *conflictWidget = new QWidget;
            conflictWidget->setStyleSheet("QWidget { background-color: #252526; border: 1px solid #3C3C3C; border-radius: 4px; margin: 10px 0px; }");
            QVBoxLayout *cwLayout = new QVBoxLayout(conflictWidget);
            
            // Ours
            QLabel *oursLbl = new QLabel("Current Change (HEAD)");
            oursLbl->setStyleSheet("color: #4EC9B0; font-weight: bold; border: none;");
            cwLayout->addWidget(oursLbl);
            
            QTextEdit *oursTe = new QTextEdit;
            oursTe->setReadOnly(true);
            oursTe->setPlainText(block.oursText);
            oursTe->setStyleSheet("QTextEdit { background-color: rgba(78, 201, 176, 0.1); border: none; font-family: Consolas, monospace; color: #4EC9B0; }");
            oursTe->setFixedHeight((qMax(1, block.oursText.count('\n')) + 1) * 20);
            cwLayout->addWidget(oursTe);

            // Theirs
            QLabel *theirsLbl = new QLabel("Incoming Change");
            theirsLbl->setStyleSheet("color: #569CD6; font-weight: bold; border: none;");
            cwLayout->addWidget(theirsLbl);
            
            QTextEdit *theirsTe = new QTextEdit;
            theirsTe->setReadOnly(true);
            theirsTe->setPlainText(block.theirsText);
            theirsTe->setStyleSheet("QTextEdit { background-color: rgba(86, 156, 214, 0.1); border: none; font-family: Consolas, monospace; color: #569CD6; }");
            theirsTe->setFixedHeight((qMax(1, block.theirsText.count('\n')) + 1) * 20);
            cwLayout->addWidget(theirsTe);

            // Actions
            QHBoxLayout *actionsLayout = new QHBoxLayout;
            QPushButton *btnOurs = new QPushButton("Accept Current");
            QPushButton *btnTheirs = new QPushButton("Accept Incoming");
            QPushButton *btnBoth = new QPushButton("Accept Both");
            
            btnOurs->setStyleSheet("background-color: #0e639c; color: white; padding: 6px; font-weight: bold; border-radius: 4px;");
            btnTheirs->setStyleSheet("background-color: #0e639c; color: white; padding: 6px; font-weight: bold; border-radius: 4px;");
            btnBoth->setStyleSheet("background-color: #0e639c; color: white; padding: 6px; font-weight: bold; border-radius: 4px;");

            actionsLayout->addWidget(btnOurs);
            actionsLayout->addWidget(btnTheirs);
            actionsLayout->addWidget(btnBoth);
            actionsLayout->addStretch();
            
            cwLayout->addLayout(actionsLayout);
            
            // Result view (initially hidden)
            QTextEdit *resTe = new QTextEdit;
            resTe->setReadOnly(true);
            resTe->hide();
            resTe->setStyleSheet("QTextEdit { background-color: rgba(255, 255, 255, 0.05); border: 1px dashed #666; font-family: Consolas, monospace; color: white; }");
            cwLayout->addWidget(resTe);
            
            auto resolveFn = [this, i, oursTe, theirsTe, resTe, actionsLayout, conflictWidget](int choice) {
                m_blocks[i].resolvedChoice = choice;
                
                // Hide ours/theirs text edits
                oursTe->hide();
                theirsTe->hide();
                
                // Hide actions layout widgets
                for(int j=0; j<actionsLayout->count(); ++j) {
                    QWidget *w = actionsLayout->itemAt(j)->widget();
                    if(w) w->hide();
                }
                
                // Show result
                resTe->show();
                if (choice == 1) resTe->setPlainText(m_blocks[i].oursText);
                else if (choice == 2) resTe->setPlainText(m_blocks[i].theirsText);
                else if (choice == 3) resTe->setPlainText(m_blocks[i].oursText + m_blocks[i].theirsText);
                
                int lines = resTe->toPlainText().count('\n');
                resTe->setFixedHeight((lines + 1) * 20);
                
                conflictWidget->setStyleSheet("QWidget { background-color: #1e1e1e; border: 1px solid #007acc; border-radius: 4px; margin: 10px 0px; }");
                
                updateSaveButtonState();
            };
            
            connect(btnOurs, &QPushButton::clicked, this, [resolveFn]() { resolveFn(1); });
            connect(btnTheirs, &QPushButton::clicked, this, [resolveFn]() { resolveFn(2); });
            connect(btnBoth, &QPushButton::clicked, this, [resolveFn]() { resolveFn(3); });

            m_blocksLayout->addWidget(conflictWidget);
        }
    }

    m_blocksLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    QWidget *bottomWidget = new QWidget;
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
    m_saveBtn = new QPushButton("Complete Merge");
    m_saveBtn->setEnabled(false);
    m_saveBtn->setMinimumHeight(40);
    m_saveBtn->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background-color: #0e639c; color: white; } QPushButton:disabled { background-color: #333333; color: #888888; }");
    
    connect(m_saveBtn, &QPushButton::clicked, this, &ConflictResolverDialog::saveAndResolve);
    
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_saveBtn);
    mainLayout->addWidget(bottomWidget);
}

void ConflictResolverDialog::updateSaveButtonState()
{
    bool allResolved = true;
    for (const auto &b : m_blocks) {
        if (b.isConflict && b.resolvedChoice == 0) {
            allResolved = false;
            break;
        }
    }
    m_saveBtn->setEnabled(allResolved);
}

void ConflictResolverDialog::saveAndResolve()
{
    QFile file(m_fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Error", "Could not save the resolved file.");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    for (const auto &b : m_blocks) {
        if (!b.isConflict) {
            out << b.normalText;
        } else {
            if (b.resolvedChoice == 1) out << b.oursText;
            else if (b.resolvedChoice == 2) out << b.theirsText;
            else if (b.resolvedChoice == 3) out << b.oursText << b.theirsText;
        }
    }
    
    file.close();
    emit resolved();
    accept();
}
