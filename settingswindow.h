#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QEvent>
#include <QStringList>
#include <externalprocess.h>
#include <globalconfig.h>

namespace Ui {
class SettingsWindow;
}

class GlobalConfig;

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(GlobalConfig* config, QWidget *parent = nullptr);
    ~SettingsWindow();

    QString config() const;

public slots:
    void setConfig(const QString &config);

signals:
    void closed();
    void configRequested();
    void configSaved(QString);
    void settingProcessFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &output);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::SettingsWindow *ui;
    QPoint m_lastPos;
    ExternalProcess *proc;

    GlobalConfig *conf;

private slots:
    void on_btSaveOnly_clicked();
    void on_btSaveClose_clicked();
    void on_btClose_clicked();
    void on_btReloadConfig_clicked();
    void on_btInstallSndcpy_clicked();

    void changeEvent(QEvent *event) override;
};

#endif // SETTINGSWINDOW_H
