#ifndef EXTERNALPROCESS_H
#define EXTERNALPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <windows.h>
#include <QDebug>

class ExternalProcess : public QObject
{
    Q_OBJECT

public:
    ExternalProcess(const QString &name, const QStringList &para,
                             QObject *parent);

    ExternalProcess(QObject *parent)
        : QObject(parent), process(new QProcess(this)) {}

    void set(const QString &name, const QStringList &para,
             QObject *parent);

    bool run(bool wait = false) {
        process->start();
        if (!process->waitForStarted(3000)) {  // 等待启动（超时3秒）
            QString errorMsg;
            switch (process->error()) {
            case QProcess::FailedToStart:
                errorMsg = QString("程序%1不存在或权限不足").arg(process->program());
                break;
            case QProcess::Crashed:
                errorMsg = QString("程序%1启动后崩溃").arg(process->program());
                break;
            default:
                errorMsg = QString("未知错误:%1").arg(process->error());
            }
            emit finished(-1, QProcess::CrashExit, errorMsg);  // 自定义错误码 -1
            return false;
        }
        if(wait)
            process->waitForFinished();
        return true;
    }

    bool isFinished() const {
        return process->state() == QProcess::NotRunning;
    }

    int retval() const {
        return process->exitCode();
    }

signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus, const QString &output);

private slots:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *process;
};

#endif // EXTERNALPROCESS_H
