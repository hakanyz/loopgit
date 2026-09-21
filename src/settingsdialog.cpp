#include "settingsdialog.h"
#include "gitmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>
#include <QMessageBox>
#include <QInputDialog>

SettingsDialog::SettingsDialog(GitManager *git, QWidget *parent)
    : QDialog(parent), m_git(git)
{
    setWindowTitle("Repository Settings");
    resize(500, 400);
    setupUi();
    loadData();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QTabWidget *tabs = new QTabWidget;
    mainLayout->addWidget(tabs);

    // -- Tab 1: User Config --
    QWidget *userTab = new QWidget;
    QVBoxLayout *userLayout = new QVBoxLayout(userTab);
    QFormLayout *formLayout = new QFormLayout;
    
    m_nameEdit = new QLineEdit;
    m_emailEdit = new QLineEdit;
    formLayout->addRow("User Name:", m_nameEdit);
    formLayout->addRow("User Email:", m_emailEdit);
    userLayout->addLayout(formLayout);

    m_saveUserBtn = new QPushButton("Save Config");
    m_saveUserBtn->setStyleSheet("background-color: #0e639c; color: white; padding: 6px; font-weight: bold; border-radius: 4px;");
    connect(m_saveUserBtn, &QPushButton::clicked, this, &SettingsDialog::saveUserConfig);
    
    QHBoxLayout *saveLayout = new QHBoxLayout;
    saveLayout->addStretch();
    saveLayout->addWidget(m_saveUserBtn);
    userLayout->addLayout(saveLayout);
    userLayout->addStretch();
    tabs->addTab(userTab, "User Config");

    // -- Tab 2: Remotes --
    QWidget *remotesTab = new QWidget;
    QVBoxLayout *remotesLayout = new QVBoxLayout(remotesTab);
    
    m_remotesList = new QListWidget;
    connect(m_remotesList, &QListWidget::itemSelectionChanged, this, &SettingsDialog::onRemoteSelected);
    remotesLayout->addWidget(m_remotesList);

    QHBoxLayout *remotesBtnLayout = new QHBoxLayout;
    m_addRemoteBtn = new QPushButton("Add");
    m_editRemoteBtn = new QPushButton("Edit URL");
    m_removeRemoteBtn = new QPushButton("Remove");
    m_editRemoteBtn->setEnabled(false);
    m_removeRemoteBtn->setEnabled(false);

    connect(m_addRemoteBtn, &QPushButton::clicked, this, &SettingsDialog::addRemote);
    connect(m_editRemoteBtn, &QPushButton::clicked, this, &SettingsDialog::editRemote);
    connect(m_removeRemoteBtn, &QPushButton::clicked, this, &SettingsDialog::removeRemote);

    remotesBtnLayout->addWidget(m_addRemoteBtn);
    remotesBtnLayout->addWidget(m_editRemoteBtn);
    remotesBtnLayout->addWidget(m_removeRemoteBtn);
    remotesBtnLayout->addStretch();
    
    remotesLayout->addLayout(remotesBtnLayout);
    tabs->addTab(remotesTab, "Remotes");

    // Dialog buttons
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
}

void SettingsDialog::loadData()
{
    if (!m_git) return;

    m_nameEdit->setText(m_git->getConfigValue("user.name"));
    m_emailEdit->setText(m_git->getConfigValue("user.email"));

    m_remotesList->clear();
    QStringList remotes = m_git->getRemotes();
    for (const QString &r : remotes) {
        QString url = m_git->getRemoteUrl(r);
        QListWidgetItem *item = new QListWidgetItem(QString("%1 (%2)").arg(r, url), m_remotesList);
        item->setData(Qt::UserRole, r);
        item->setData(Qt::UserRole + 1, url);
    }
}

void SettingsDialog::saveUserConfig()
{
    if (m_git->setConfigValue("user.name", m_nameEdit->text()) &&
        m_git->setConfigValue("user.email", m_emailEdit->text())) {
        QMessageBox::information(this, "Success", "User configuration saved successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to save configuration.");
    }
}

void SettingsDialog::onRemoteSelected()
{
    bool hasSelection = m_remotesList->currentItem() != nullptr;
    m_editRemoteBtn->setEnabled(hasSelection);
    m_removeRemoteBtn->setEnabled(hasSelection);
}

void SettingsDialog::addRemote()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Remote", "Remote Name (e.g., origin):", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString url = QInputDialog::getText(this, "Add Remote", "Remote URL:", QLineEdit::Normal, "", &ok);
    if (!ok || url.isEmpty()) return;

    if (m_git->addRemote(name, url)) {
        loadData();
    } else {
        QMessageBox::critical(this, "Error", m_git->lastError());
    }
}

void SettingsDialog::editRemote()
{
    QListWidgetItem *item = m_remotesList->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    QString oldUrl = item->data(Qt::UserRole + 1).toString();

    bool ok;
    QString newUrl = QInputDialog::getText(this, "Edit Remote", QString("New URL for '%1':").arg(name), QLineEdit::Normal, oldUrl, &ok);
    if (!ok || newUrl.isEmpty() || newUrl == oldUrl) return;

    if (m_git->setRemoteUrl(name, newUrl)) {
        loadData();
    } else {
        QMessageBox::critical(this, "Error", m_git->lastError());
    }
}

void SettingsDialog::removeRemote()
{
    QListWidgetItem *item = m_remotesList->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, "Remove Remote", QString("Are you sure you want to remove remote '%1'?").arg(name)) == QMessageBox::Yes) {
        if (m_git->removeRemote(name)) {
            loadData();
        } else {
            QMessageBox::critical(this, "Error", m_git->lastError());
        }
    }
}
 
