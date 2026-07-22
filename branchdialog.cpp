#include "branchdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>

BranchDialog::BranchDialog(const QString &defaultPrefix, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Start New Branch");
    setMinimumWidth(350);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    m_prefixCombo = new QComboBox(this);
    m_prefixCombo->addItems({"feature/", "bugfix/", "release/", "hotfix/", "custom"});
    if (m_prefixCombo->findText(defaultPrefix) != -1) {
        m_prefixCombo->setCurrentText(defaultPrefix);
    }
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("branch-name");
    
    nameLayout->addWidget(m_prefixCombo);
    nameLayout->addWidget(m_nameEdit, 1);

    m_checkoutCheck = new QCheckBox("Checkout after creation", this);
    m_checkoutCheck->setChecked(true);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(new QLabel("Branch Name:"));
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(m_checkoutCheck);
    mainLayout->addStretch();
    mainLayout->addWidget(btnBox);
}

QString BranchDialog::branchName() const
{
    QString prefix = m_prefixCombo->currentText();
    if (prefix == "custom") prefix = "";
    return prefix + m_nameEdit->text().trimmed();
}

bool BranchDialog::shouldCheckout() const
{
    return m_checkoutCheck->isChecked();
}
 
