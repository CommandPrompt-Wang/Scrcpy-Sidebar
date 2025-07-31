#ifndef TOOLBOXMAIN_H
#define TOOLBOXMAIN_H

#pragma once

#include <QWidget>
#include <QEvent>
#include <globalconfig.h>
#include <externalprocess.h>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QStyle>
#include <QTimer>
#include <QCloseEvent>
#include <QTcpSocket>
#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QProgressDialog>
#include <QThread>
#include <settingswindow.h>
#include <QScreen>
#include <advancedkeyboard.h>
#include <QTextEdit>


QT_BEGIN_NAMESPACE
namespace Ui {
class ToolboxMain;
}
QT_END_NAMESPACE

class ToolboxMain : public QWidget
{
    Q_OBJECT

public:
    ToolboxMain(QWidget *parent = nullptr);
    // void showMessage(const QString &title, const QString &msg,
    //                  QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int msecs = 10000);
    ~ToolboxMain();

protected:
     void closeEvent(QCloseEvent *event) override;

private slots:
    void sendTrayMessage(QString title, QString content,
                         QSystemTrayIcon::MessageIcon icon,
                         int msecs);

    void handleLongPress();

    void saveConfigFromText();

    void on_ckUseSndcpy_checkStateChanged(const Qt::CheckState &arg1);

    void on_btKillADBonExit_checkStateChanged(const Qt::CheckState &arg1);

    void handleError(int exitCode, QProcess::ExitStatus exitStatus, const QString &output);

    void on_btSettings_toggled(bool checked);

    // void on_btSettings_triggered(QAction *arg1);
    void on_ckNvgateBtOn_checkStateChanged(const Qt::CheckState &arg1);

    void on_ckNeverLock_checkStateChanged(const Qt::CheckState &arg1);

    void on_ckScreenOffTimeout_checkStateChanged(const Qt::CheckState &arg1);

    void on_spScreenOffTimeout_editingFinished();

    void on_btAbout_clicked();

signals:
    // void getConfig(QString configText);

private:
    Ui::ToolboxMain *ui;
    GlobalConfig* conf;
    // ExternalProcess* proc;
    QHash<QPushButton*, QString> buttonKeyMap;
    QSystemTrayIcon *tray;
    QTimer *longPressTimer;                  // 长按检测计时器
    QPushButton *currentPressedButton;       // 当前按下的按钮
    QTcpSocket *audioSocket;
    QAudioSink *audioSink;
    QIODevice *audioOutput;
    int audioConnectRetries;

    bool bKillADBonExit;

    void sendAdbKey(const QString &keyevent, bool isLongPress);
    void setupAudioPlayback();
    void initDeviceSettings();
    void fetchSetting(const QString& settingNamespace,
                      const QString& settingName,
                      std::function<void(const QString&)> callback);
    void handleStayOnSetting(Qt::CheckState state);
    void handleScreenOffTimeout(Qt::CheckState state);

    void changeEvent(QEvent *event) override;

    SettingsWindow *settingsWnd;
    AdvancedKeyboard *advBrd;

    QString m_originalStayOnValue = "0"; // 存储原始值
    QString m_originalScreenOffTimeout;
};
#endif // TOOLBOXMAIN_H
