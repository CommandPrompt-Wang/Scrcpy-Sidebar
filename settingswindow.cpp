#include "settingswindow.h"
#include "ui_settingswindow.h"


// 按道理可以把SettingsWindow、AdvancedKeyboard抽象为一个
// 一起共享主窗口的数据，但是懒得重构了。头疼。
SettingsWindow::SettingsWindow(GlobalConfig *config ,QWidget *parent)
    : QWidget(parent),
    ui(new Ui::SettingsWindow),
    m_lastPos(QPoint(0, 0)),
    conf(config)
{
    ui->setupUi(this);

    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                   Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint|
                   Qt::Tool);

    proc = new ExternalProcess(this);

    connect(proc, &ExternalProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus, const QString &output){
        emit settingProcessFinished(exitCode, exitStatus, output);
    });
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

QString SettingsWindow::config() const
{
    return ui->edConfig->toPlainText();
}

void SettingsWindow::setConfig(const QString &config)
{
    ui->edConfig->setText(config);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    m_lastPos = pos();
    emit closed();
    QWidget::closeEvent(event);
}

void SettingsWindow::showEvent(QShowEvent *event)
{
    // if (!m_lastPos.isNull()) {
    //     move(m_lastPos);
    // } else if (parentWidget()) {
    //     move(parentWidget()->geometry().center() - rect().center());
    // }

    emit configRequested();
    QWidget::showEvent(event);
}

void SettingsWindow::on_btSaveOnly_clicked()
{
    emit configSaved(ui->edConfig->toPlainText());
    emit configRequested();
}

void SettingsWindow::on_btSaveClose_clicked()
{
    emit configSaved(ui->edConfig->toPlainText());
    emit configRequested();
    close();
}

void SettingsWindow::on_btClose_clicked()
{
    close();
}

void SettingsWindow::on_btReloadConfig_clicked()
{
    emit configRequested();
}

void SettingsWindow::on_btInstallSndcpy_clicked()
{
    QString sndcpy = conf->sndcpyApkPath() + "sndcpy.apk";

    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();  // 只在有设备号时添加
    }
    args << "install" << sndcpy;

    proc->set(conf->adbPath(), args, this);
    proc->run();
}

