#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QLineEdit;
class QCheckBox;

class BranchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BranchDialog(const QString &defaultPrefix = "feature/", QWidget *parent = nullptr);

    QString branchName() const;
    bool shouldCheckout() const;

private:
    QComboBox *m_prefixCombo;
    QLineEdit *m_nameEdit;
    QCheckBox *m_checkoutCheck;
};

#endif // BRANCHDIALOG_H
 
