#include "externalprocess.h"

ExternalProcess::ExternalProcess(const QString &name, const QStringList &para,
                                 QObject *parent = nullptr)
    : QObject(parent), process(new QProcess(this))
{
    set(name, para, parent);
}

void ExternalProcess::set(const QString &name, const QStringList &para, QObject *parent = nullptr )
{
    this->setParent(parent);

    disconnect(process, nullptr, this, nullptr);
    process->setProgram(name);
    process->setArguments(para);

#ifdef Q_OS_WIN
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
    });
#endif

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ExternalProcess::handleFinished);
}

void ExternalProcess::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // 获取所有标准输出
    QString output = QString::fromLocal8Bit(process->readAllStandardOutput());

    // 如果有错误输出也合并
    QString errorOutput = QString::fromLocal8Bit(process->readAllStandardError());
    if (!errorOutput.isEmpty()) {
        output += errorOutput;
    }

    emit finished(exitCode, exitStatus, output);
}
