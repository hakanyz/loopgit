#include "appsettingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>

AppSettingsDialog::AppSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Application Settings");
    resize(400, 200);
    setupUi();
    loadSettings();
}

void AppSettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *formLayout = new QFormLayout;

    m_closeBehaviorCombo = new QComboBox;
    m_closeBehaviorCombo->addItem("Ask me every time", "ask");
    m_closeBehaviorCombo->addItem("Minimize to System Tray", "tray");
    m_closeBehaviorCombo->addItem("Exit Application", "quit");

    m_checkUpdatesCheck = new QCheckBox("Check for updates on startup");

    formLayout->addRow("Close Behavior:", m_closeBehaviorCombo);
    formLayout->addRow("", m_checkUpdatesCheck);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton("Save");
    saveBtn->setStyleSheet("background-color: #0e639c; color: white; padding: 6px; font-weight: bold; border-radius: 4px;");
    connect(saveBtn, &QPushButton::clicked, this, &AppSettingsDialog::saveSettings);

    QPushButton *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void AppSettingsDialog::loadSettings()
{
    QSettings settings;
    
    QString closeAction = settings.value("app/closeAction", "").toString();
    if (closeAction == "tray") {
        m_closeBehaviorCombo->setCurrentIndex(1);
    } else if (closeAction == "quit") {
        m_closeBehaviorCombo->setCurrentIndex(2);
    } else {
        m_closeBehaviorCombo->setCurrentIndex(0);
    }

    bool checkUpdates = settings.value("app/checkUpdates", true).toBool();
    m_checkUpdatesCheck->setChecked(checkUpdates);
}

void AppSettingsDialog::saveSettings()
{
    QSettings settings;
    
    QString closeAction = m_closeBehaviorCombo->currentData().toString();
    if (closeAction == "ask") {
        settings.setValue("app/closeAction", ""); // Empty means ask
    } else {
        settings.setValue("app/closeAction", closeAction);
    }

    settings.setValue("app/checkUpdates", m_checkUpdatesCheck->isChecked());

    accept();
}
 
