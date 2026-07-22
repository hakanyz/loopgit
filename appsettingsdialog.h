#ifndef APPSETTINGSDIALOG_H
#define APPSETTINGSDIALOG_H

#include <QDialog>

class QComboBox;
class QCheckBox;

class AppSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AppSettingsDialog(QWidget *parent = nullptr);

private slots:
    void saveSettings();

private:
    void setupUi();
    void loadSettings();

    QComboBox *m_closeBehaviorCombo;
    QCheckBox *m_checkUpdatesCheck;
};

#endif // APPSETTINGSDIALOG_H
 
