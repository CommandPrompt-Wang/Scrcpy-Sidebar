#include "toolboxmain.h"
#include "ui_toolboxmain.h"

// 在 main.cpp
extern void loadTranslation(const QString& langCode);

ToolboxMain::ToolboxMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ToolboxMain)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint |
               Qt::WindowTitleHint |
               Qt::WindowCloseButtonHint |
               Qt::WindowStaysOnTopHint |
               Qt::Tool
               );

    conf = new GlobalConfig;
    conf->loadConfig();
    tray = new QSystemTrayIcon(this->style()->standardIcon(QStyle::SP_MessageBoxInformation) ,this);
    tray->show();
    audioSink = nullptr;
    audioSocket = nullptr;

    loadTranslation(conf->language());
    qDebug() <<"detected"<<conf->language()<<"@start";

    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    proc->set(conf->adbPath(),QStringList("devices"),this);
    proc->run(true);

    buttonKeyMap[ui->btVolUp] = "VOLUME_UP";
    buttonKeyMap[ui->btVolDown] = "VOLUME_DOWN";
    buttonKeyMap[ui->btPower] = "POWER";

    longPressTimer = new QTimer(this);
    longPressTimer->setSingleShot(true);
    connect(longPressTimer, &QTimer::timeout, this, &ToolboxMain::handleLongPress);

    for(auto it = buttonKeyMap.begin(); it != buttonKeyMap.end(); ++it) {
        QPushButton *btn = it.key();
        const QString &key = it.value();

        // 按下时开始计时
        connect(btn, &QPushButton::pressed, [this, btn, key]() {
            currentPressedButton = btn;
            longPressTimer->start(500); // 800ms长按阈值
        });

        // 释放时判断短按/取消长按
        connect(btn, &QPushButton::released, [this, key]() {
            if (longPressTimer->isActive()) {
                longPressTimer->stop();  // 在阈值内释放，触发短按
                sendAdbKey(key, false);         // 原有短按逻辑
            }
            currentPressedButton = nullptr;
        });
    }

    connect(ui->btClose, &QToolButton::clicked, this, []() {
        qDebug() << "Quit requested";
        QCoreApplication::quit();
    });

    this->bKillADBonExit = false;

    settingsWnd = new SettingsWindow(conf);
    connect(settingsWnd, &SettingsWindow::closed, this, [this]() {
        ui->btSettings->setChecked(false);  // 窗口隐藏时同步按钮状态
    });
    connect(settingsWnd, &SettingsWindow::configRequested, this, [this](){
        settingsWnd->setConfig(conf->getText());
    });
    connect(settingsWnd, &SettingsWindow::configSaved, this, [this](QString configText){
        conf->loadConfigFromText(configText);
        conf->saveConfig();
        advBrd->updateButtonLayout();
    });
    connect(settingsWnd, &SettingsWindow::settingProcessFinished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus, const QString &output){
        handleError(exitCode, exitStatus, output);
    });
    connect(settingsWnd, &SettingsWindow::sendTrayMessage, this,
            [this](QString message) {
                tray->showMessage(tr("提示"),message);
    });
    // connect(settingsWnd, &SettingsWindow::configRequested, this, [this](){
    //     emit sendConfig(conf->getText());
    // });

    advBrd = new AdvancedKeyboard(conf);
    connect(advBrd, &AdvancedKeyboard::closed, this, [this](){
        ui->ckNvgateBtOn->setChecked(false);  // 窗口隐藏时同步按钮状态
    });
    connect(advBrd, &AdvancedKeyboard::settingProcessFinished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus, const QString &output){
                handleError(exitCode, exitStatus, output);
            });

    // 初始化默认值
    m_originalStayOnValue = "0";
    m_originalScreenOffTimeout = "30000";

    // 异步获取设备当前设置
    initDeviceSettings();
}

void ToolboxMain::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);  // 自动更新所有文本！
    }
}

void ToolboxMain::initDeviceSettings()
{
    // 获取屏幕常亮设置
    fetchSetting("global", "stay_on_while_plugged_in",
                 [this](const QString& value) {
                     m_originalStayOnValue = value.isEmpty() ? "0" : value;
                 });

    // 获取屏幕超时设置
    fetchSetting("system", "screen_off_timeout",
                 [this](const QString& value) {
                     m_originalScreenOffTimeout = value.isEmpty() ? "0" : value;
                 });
}

void ToolboxMain::handleLongPress(){
    if (!currentPressedButton || !buttonKeyMap.contains(currentPressedButton))
        return;

    QString key = buttonKeyMap[currentPressedButton];

    qDebug()<< "longpress: "<<key;

    sendAdbKey(key, true);
}

void ToolboxMain::sendAdbKey(const QString &keyevent, bool isLongPress = false)
{
    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    QString adb = conf->adbPath();

    // proc->set(adb, QStringList({conf->deviceSerial(), "shell", "input", "keyevent", keyevent}), this);
    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();  // 只在有设备号时添加
    }
    args << "shell" << "input" << "keyevent";
    if (isLongPress) {
        args << "--longpress";
    }
    args << keyevent;

    proc->set(adb, args, this);

    //建立新的连接
    // disconnect(proc, &ExternalProcess::finished, nullptr, nullptr);
    // connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    proc->run();
}

// void ToolboxMain::showMessage(const QString &title, const QString &msg, QSystemTrayIcon::MessageIcon icon, int msecs)
// {
//     tray->showMessage(title, msg, icon, msecs);
// }

void ToolboxMain::handleError(int exitCode, QProcess::ExitStatus exitStatus, const QString &output){
    if(exitCode != 0){
        qDebug()<<output;

        // 定义错误模式和对应消息的映射
        static const QMap<QString, QString> errorMessages = {
            {"no devices/emulators found", tr("未找到可用的设备")},
            {"error: more than one device and emulator", tr("你连接了多个设备，请在设置⚙里指定设备序列号")},
            {"error: device offline", tr("设备离线")},
            {"Error: No UID for com.rom1v.sndcpy", tr("你的设备还没有安装sndcpy.apk。请在设置里手动安装。")}
        };

        bool hasKnownError = false;
        for (auto it = errorMessages.constBegin(); it != errorMessages.constEnd(); ++it) {
            if (output.contains(it.key())) {
                tray->showMessage(tr("错误"), it.value(), QSystemTrayIcon::Critical, 3000);
                hasKnownError = true;
            }
        }

        // 如果没有匹配到已知错误，但输出中包含"errortr("或")Error"，则显示原始错误信息
        if (!hasKnownError && (output.contains("error") || output.contains("Error"))) {
            tray->showMessage(tr("错误"), output, QSystemTrayIcon::Critical, 3000);
        }
    }
}

void ToolboxMain::on_ckUseSndcpy_checkStateChanged(const Qt::CheckState &arg1)
{
    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    QString adb = conf->adbPath();
    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }

    if(arg1 == Qt::Checked)
    {
        // 防止锁死，在setupAudioPlayback解封
        ui->ckUseSndcpy->setDisabled(true);

        bool success = true;
        // 1. 授权
        QStringList argt = args;
        argt << "shell" << "appops" << "set" << "com.rom1v.sndcpy" << "PROJECT_MEDIA" << "allow";
        proc->set(adb, argt, this);
        qDebug() << argt;
        success &= proc->run(true);

        // 2. 开端口
        argt = args;
        argt << "forward" << "tcp:" + QString::number(conf->sndcpyPort())
             << "localabstract:sndcpy";
        qDebug() << argt;
        proc->set(adb, argt, this);
        success &= proc->run(true);

        // 3. 启动应用
        argt = args;
        argt << "shell" << "am" << "start" << "com.rom1v.sndcpy/.MainActivity";
        qDebug() << argt;
        proc->set(adb, argt, this);
        success &= proc->run(true);

        if(!success) {
            tray->showMessage(tr("错误"), tr("未能启动sndcpy，请检查先前的错误消息！"));
            ui->ckUseSndcpy->setDisabled(false);
            ui->ckUseSndcpy->setChecked(false);
            return;
        }

        // 4. 设置音频播放
        QTimer::singleShot(2000, this, &ToolboxMain::setupAudioPlayback);
        // ui->ckUseSndcpy->setDisabled(false);
    }
    else
    {
        // 停止音频播放
        if(audioSink) {
            audioSink->stop();
            delete audioSink;
            audioSink = nullptr;
        }
        if(audioSocket) {
            audioSocket->disconnectFromHost();
            delete audioSocket;
            audioSocket = nullptr;
        }

        // 停止应用
        args << "shell" << "am" << "force-stop" << "com.rom1v.sndcpy";
        proc->set(adb, args, this);
        proc->run();
    }
}

void ToolboxMain::setupAudioPlayback()
{
    // 创建TCP socket连接
    audioSocket = new QTcpSocket(this);

    // 使用成员变量记录重试次数
    audioConnectRetries = 0;
    const int maxRetries = 3;

    // 使用局部QTimer实现延迟重试
    QTimer *retryTimer = new QTimer(this);
    retryTimer->setSingleShot(true);

    auto tryConnect = [this, retryTimer]() {
        if (this->audioConnectRetries >= maxRetries) {
            tray->showMessage(tr("错误"), tr("无法连接到音频流，已达到最大重试次数"));
            retryTimer->deleteLater();

            // 解封
            ui->ckUseSndcpy->setDisabled(false);
            return;
        }

        qDebug() << tr("尝试连接音频流，重试次数:") << this->audioConnectRetries;
        audioSocket->connectToHost("127.0.0.1", conf->sndcpyPort());
        this->audioConnectRetries++;
    };

    connect(retryTimer, &QTimer::timeout, this, tryConnect);

    connect(audioSocket, &QTcpSocket::connected, this, [this, retryTimer]() {
        qDebug() << tr("成功连接到音频流");
        audioConnectRetries = 0;
        retryTimer->deleteLater();

        // 设置音频输出格式 (使用48000Hz匹配sndcpy默认设置)
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);

        QAudioDevice device = QMediaDevices::defaultAudioOutput();
        if(!device.isFormatSupported(format)) {
            qDebug() << tr("默认格式不支持，尝试使用首选格式");
            format = device.preferredFormat();
        }

        qDebug() << tr("使用的音频格式:")
                 << tr("采样率:") << format.sampleRate()
                 << tr("声道数:") << format.channelCount()
                 << tr("采样格式:") << format.sampleFormat();

        audioSink = new QAudioSink(device, format, this);
        audioOutput = audioSink->start();

        connect(audioSocket, &QTcpSocket::readyRead, this, [this]() {
            QByteArray data = audioSocket->readAll();
            if (audioOutput && audioOutput->isOpen()) {
                audioOutput->write(data);
                // qDebug() << tr("写入音频数据大小:") << data.size() << tr("字节");
            }
        });
        // 解封
        ui->ckUseSndcpy->setDisabled(false);
    });

    connect(audioSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qDebug() << tr("音频流连接错误:") << error << audioSocket->errorString();
        tray->showMessage(tr("错误"), tr("音频流连接错误:") + audioSocket->errorString());
    });

    // 首次立即尝试连接
    tryConnect();
}

void ToolboxMain::on_btKillADBonExit_checkStateChanged(const Qt::CheckState &arg1)
{
    if(arg1 == Qt::Checked) {
        this->bKillADBonExit = true;
    } else {
        this->bKillADBonExit = false;
    }
}

void ToolboxMain::closeEvent(QCloseEvent *event) {

    // disconnect(proc, &ExternalProcess::finished, nullptr, nullptr);

    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    // connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);


    // 创建全模态进度对话框
    QProgressDialog progress(tr("正在关闭程序..."), QString(), 0, 5, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(nullptr); // 隐藏取消按钮
    progress.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    progress.setMinimumDuration(0); // 立即显示
    progress.setValue(0);

    //0. 关闭端口
    if(audioSink) {
        audioSink->stop();
        delete audioSink;
        audioSink = nullptr;
    }
    if(audioSocket) {
        audioSocket->disconnectFromHost();
        delete audioSocket;
        audioSocket = nullptr;
    }
    progress.setValue(1);

    // 1. 保存配置
    progress.setLabelText(tr("正在保存配置文件..."));
    qDebug() << "saving config...";
    if (!conf->saveConfig()) {
        qWarning() << "Failed to save config!";
    }
    progress.setValue(2);
    QCoreApplication::processEvents();

    QString adb = conf->adbPath();

    // 2. 停止sndcpy应用
    if(ui->ckUseSndcpy->isChecked())
    {
        progress.setLabelText(tr("正在停止sndcpy应用..."));
        QStringList stopArgs;
        stopArgs << "shell" << "am" << "force-stop" << "com.rom1v.sndcpy";
        proc->set(adb, stopArgs, this);
        qDebug() << "stopping sndcpy...";
        proc->run(true);  // 阻塞执行
    }
    progress.setValue(3);
    QCoreApplication::processEvents();

    // 3. 杀死ADB服务
    if(bKillADBonExit) {
        progress.setLabelText(tr("正在停止ADB服务..."));
        qDebug() << "killing adb...";
        proc->set(adb, QStringList({"kill-server"}), this);
        proc->run(true);
    }
    progress.setValue(4);

    //4. 恢复配置
    QStringList args, argt;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }
    argt = args;
    argt << "shell" << "settings" << "put" << "global" << "stay_on_while_plugged_in" << m_originalStayOnValue;
    proc->set(adb, argt, this);
    proc->run(true);

    argt = args;
    argt << "shell" << "settings" << "put" << "system" << "screen_off_timeout" << m_originalScreenOffTimeout;
    proc->set(adb, argt, this);
    proc->run(true);

    progress.setValue(4);

    // 完成提示
    progress.setLabelText(tr("完成"));
    QThread::msleep(200);
    progress.close();

    event->accept();
    qDebug() << "finish.";

    settingsWnd->close();
    qApp->exit();
}

ToolboxMain::~ToolboxMain()
{
    delete ui;
}

void ToolboxMain::on_btSettings_toggled(bool checked)
{
    if (checked) {
        settingsWnd->show();

        // 确保窗口已经显示，这样才能获取正确的几何信息
        settingsWnd->adjustSize();

        // 获取主窗口和设置窗口的几何信息
        QRect mainRect = this->geometry();
        QRect settingsRect = settingsWnd->geometry();

        // 获取窗口框架的标题栏高度
        int titleBarHeight = this->frameGeometry().top() - this->geometry().top();

        // 计算可用空间
        QScreen *screen = this->screen();
        QRect screenRect = screen->availableGeometry();

        // 计算实际需要的Y位置（考虑标题栏高度）
        int yPos = mainRect.top() + titleBarHeight;

        // 尝试右侧
        int rightSpace = screenRect.right() - mainRect.right();
        if (rightSpace >= settingsRect.width()) {
            settingsWnd->move(mainRect.right(), yPos);
        }
        // 尝试左侧
        else if (mainRect.left() >= settingsRect.width()) {
            settingsWnd->move(mainRect.left() - settingsRect.width(), yPos);
        }
        // 默认居中（但保持顶端对齐）
        else {
            int x = mainRect.left() + (mainRect.width() - settingsRect.width()) / 2;
            settingsWnd->move(x, yPos);
        }
    } else {
        settingsWnd->close();
    }
}

void ToolboxMain::saveConfigFromText(){
    conf->loadConfigFromText(settingsWnd->config());
}


void ToolboxMain::sendTrayMessage(QString title, QString content,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                     int msecs = 10000)
{
    tray->showMessage(title, content, icon, msecs);
}

void ToolboxMain::on_ckNvgateBtOn_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        advBrd->show();

        // 确保窗口已经显示，这样才能获取正确的几何信息
        // advBrd->adjustSize();

        // 获取主窗口和设置窗口的几何信息
        QRect mainRect = this->geometry();
        QRect settingsRect = advBrd->geometry();

        // 获取窗口框架的标题栏高度
        int titleBarHeight = this->frameGeometry().top() - this->geometry().top();

        // 计算可用空间
        QScreen *screen = this->screen();
        QRect screenRect = screen->availableGeometry();

        // 计算实际需要的X位置（保持右侧对齐）
        int xPos = mainRect.right() - settingsRect.width();

        // 尝试下方
        int topSpace = mainRect.top() - screenRect.top();
        if (screenRect.bottom() - mainRect.bottom() >= settingsRect.height()) {
            advBrd->move(xPos, mainRect.bottom());
        }
        // 尝试上方
        else if (topSpace >= settingsRect.height()) {
            advBrd->move(xPos, mainRect.top() - settingsRect.height() + 2*titleBarHeight);
        }
        // 默认居中（但保持右侧对齐）
        else {
            int y = mainRect.top() + (mainRect.height() - settingsRect.height()) / 2;
            advBrd->move(xPos, y);
        }
    } else {
        advBrd->close();
    }
}

void ToolboxMain::fetchSetting(const QString& settingNamespace,
                               const QString& settingName,
                               std::function<void(const QString&)> callback)
{
    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    QString adb = conf->adbPath();
    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }

    args << "shell" << "settings" << "get" << settingNamespace << settingName;

    auto *connection = new QMetaObject::Connection;
    *connection = connect(proc, &ExternalProcess::finished,
                          [connection, callback](int exitCode, QProcess::ExitStatus exitStatus, const QString& output) {
                              disconnect(*connection);
                              delete connection;

                              if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                                  callback(output.trimmed());
                              } else {
                                  qWarning() << "Failed to fetch setting:" << output;
                              }
                          });

    proc->set(adb, args, this);
    proc->run();
}

void ToolboxMain::on_ckNeverLock_checkStateChanged(const Qt::CheckState &arg1)
{
    handleStayOnSetting(arg1);
}

void ToolboxMain::handleStayOnSetting(Qt::CheckState state)
{
    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    QString adb = conf->adbPath();
    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }

    if (state == Qt::Checked) {
        // 启用常亮
        args << "shell" << "settings" << "put" << "global" << "stay_on_while_plugged_in" << "7";
    } else {
        // 恢复原设置
        args << "shell" << "settings" << "put" << "global" << "stay_on_while_plugged_in" << m_originalStayOnValue;
    }

    proc->set(adb, args, this);
    if (!proc->run(true)) {
        tray->showMessage(tr("错误"), state == Qt::Checked ? tr("启用屏幕常亮失败") : tr("恢复屏幕设置失败"));
    }
}

void ToolboxMain::on_ckScreenOffTimeout_checkStateChanged(const Qt::CheckState &arg1)
{
    handleScreenOffTimeout(arg1);
}

void ToolboxMain::handleScreenOffTimeout(Qt::CheckState state)
{
    ExternalProcess* proc;
    proc = new ExternalProcess(this);
    connect(proc, &ExternalProcess::finished, this, &ToolboxMain::handleError);

    QString adb = conf->adbPath();
    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }

    if (state == Qt::Checked) {
        // 设置新的超时时间（从spinBox获取）
        int newTimeout = ui->spScreenOffTimeout->value();
        args << "shell" << "settings" << "put" << "system" << "screen_off_timeout" << QString::number(newTimeout);
    } else {
        // 恢复原设置
        args << "shell" << "settings" << "put" << "system" << "screen_off_timeout" << m_originalScreenOffTimeout;
    }

    proc->set(adb, args, this);
    if (!proc->run(true)) {
        tray->showMessage(tr("错误"), state == Qt::Checked ? tr("设置屏幕超时失败") : tr("恢复屏幕超时设置失败"));
    }
}

void ToolboxMain::on_spScreenOffTimeout_editingFinished()
{
    if (ui->ckScreenOffTimeout->isChecked()) {
        on_ckScreenOffTimeout_checkStateChanged(Qt::Checked);
    }
}

void ToolboxMain::on_btAbout_clicked()
{
    // 模态化+置顶
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle("关于 Scrcpy-Sidebar");
    aboutDialog->setWindowModality(Qt::WindowModal);
    aboutDialog->setWindowFlags(aboutDialog->windowFlags() | Qt::WindowStaysOnTopHint);

    QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
    aboutDialog->resize(screenSize.width() * 0.4, screenSize.height() * 0.8);

    // 带滚动条的文本编辑框（只读）
    QTextEdit *textEdit = new QTextEdit(aboutDialog);
    textEdit->setReadOnly(true);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许选择文本
    textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    // 构建关于文本内容
    QString aboutText =
     tr("<h2 style='text-align:center;'>Scrcpy-Sidebar</h2>"
        "<hr>"
        "<p><b>开发者:</b> Command_Prompt</p>"
        "<p><b>构建工具:</b> Qt 6.9.1 (LGPLv3)</p>"
        "<hr>"
        "<h3>主要功能:</h3>"
        "<ul>"
        "<li>Scrcpy扩展面板</li>"
        "<li>音量/电源键快捷操作</li>"
        "<li>针对 api 29 的sndcpy音频转发支持(Android 10)</li>"
        "<li>可扩展的按键面板</li>"
        "</ul>"
        "<hr>"
        "<p style='color:red;'><b>注意:</b> 请尽量使用GUI修改设置，直接编辑配置文件可能不会立即生效或导致错误。</p>"
        "<hr>"
        "<h3>当前配置文件内容:</h3>"
        "<pre style='white-space: pre-wrap;'>%1</pre>"
        "<hr>"
        "<h3>配置文件说明:</h3>"
        "<ul>"
        "<li><b>adbPath</b>: ADB工具路径</li>"
        "<li><b>deviceSerial</b>: 设备序列号(多设备时使用)</li>"
        "<li><b>sndcpyApkPath</b>: sndcpy.apk路径</li>"
        "<li><b>sndcpyPort</b>: 音频转发TCP端口(默认28200)</li>"
        "<li><b>wndInfoOfAdvancedKeyboard</b>: 扩展按键面板设置</li>"
        "</ul>"
        "<p>路径填写推荐使用正斜杠/，可以但不推荐\\\\；单斜杠\\无效。</p>"
        "<h4>扩展按键面板配置说明:</h4>"
        "<pre>"
        "\"wndInfoOfAdvancedKeyboard\": {\n"
        "    \"buttons\": {\n"
        "        \"主页\": \"HOME\"  // 按钮显示名称+对应的键码\n"
        "    },\n"
        "    \"height\": 46,      // 扩展键盘高度(像素)\n"
        "    \"width\": 178       // 扩展键盘宽度(像素)\n"
        "}"
        "</pre>"
        "<p>配置说明：</p>"
        "<ul>"
        "<li><b>buttons</b>: 定义按键，键名为显示文本，键值为对应的Android键码</li>"
        "<li><b>height/width</b>: 定义扩展键盘窗口的尺寸(单位:像素)</li>"
        "<li>常用键码: HOME(主页), BACK(返回), VOLUME_UP(音量+), VOLUME_DOWN(音量-)</li>"
        "</ul>").arg(conf->getText().toHtmlEscaped());

    textEdit->setHtml(aboutText);
    textEdit->moveCursor(QTextCursor::Start); // 滚动到顶部

    // 添加关闭按钮
    QPushButton *closeButton = new QPushButton(tr("关闭"), aboutDialog);
    connect(closeButton, &QPushButton::clicked, aboutDialog, &QDialog::accept);

    // 布局设置
    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);
    layout->addWidget(textEdit);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    // 显示对话框
    aboutDialog->exec();
    aboutDialog->deleteLater();
}
