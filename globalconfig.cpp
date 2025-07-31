#include "globalconfig.h"

//at main.cpp
extern void loadTranslation(const QString& langCode);

GlobalConfig::GlobalConfig(QObject* parent) : QObject(parent) {
    // 确定配置文件路径
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << appDataDir << "\n";
    QDir dir(appDataDir);
    if (!dir.exists())
        dir.mkpath(".");

    m_configPath = dir.filePath("config.json");

    // 加载或初始化配置
    if (!loadConfig()) {
        initDefaultConfig();
        saveConfig();
    }
}

bool GlobalConfig::loadConfigFromText(QString configText)
{
    configText.replace(QStringLiteral("："), QStringLiteral(": "));
    configText.replace(QStringLiteral("，"), QStringLiteral(", "));


    // 解析JSON文本
    QJsonParseError parseError;
    QJsonDocument tempDoc = QJsonDocument::fromJson(configText.toUtf8(), &parseError);
    if (tempDoc.isNull() || !tempDoc.isObject()) {
        qDebug() << tr("JSON解析错误:") << parseError.errorString();
        return false;
    }

    // 定义允许的字段及其默认值
    const QStringList allowedFields = {"adbPath", "deviceSerial", "sndcpyApkPath", "sndcpyPort", "note", "wndInfoOfAdvancedKeyboard", "language"};
    const QStringList allowedKeyboardFields = {"buttons", "height", "width"};

    QJsonObject defaultLayout{{"width", 800}, {"height", 80}};
    QJsonObject defaultObj{
        {"adbPath", "adb.exe"},
        {"sndcpyApkPath", "sndcpy.apk"},
        {"sndcpyPort", 28200},
        {"deviceSerial", ""},
        {"note", tr("此配置不会实时刷新。请尽量从GUI更改配置，不合适的修改可能导致错误。/This configuration will not be refreshed in real-time. Please try to make configuration changes through the GUI, as inappropriate modifications may lead to errors.")},
        {"wndInfoOfAdvancedKeyboard", defaultLayout},
        {"language", "zh_CN"}
    };

    // 过滤并处理配置
    QJsonObject originalObj = tempDoc.object();
    QJsonObject filteredObj;

    for (const auto& key : allowedFields) {
        if (!originalObj.contains(key)) {
            filteredObj[key] = defaultObj[key];
            continue;
        }

        if (key != "wndInfoOfAdvancedKeyboard") {
            filteredObj[key] = originalObj[key];
            continue;
        }

        // 特殊处理键盘布局配置
        QJsonObject keyboardObj = originalObj[key].toObject();
        QJsonObject filteredKeyboardObj = defaultLayout;

        for (const auto& keyboardKey : allowedKeyboardFields) {
            if (!keyboardObj.contains(keyboardKey)) continue;

            if (keyboardKey == "buttons") {
                // 过滤buttons对象，只保留简单键值对
                QJsonObject buttonsObj = keyboardObj[keyboardKey].toObject();
                QJsonObject filteredButtonsObj;

                for (const auto& buttonKey : buttonsObj.keys()) {
                    auto value = buttonsObj[buttonKey];
                    if (value.isString() || value.isDouble() || value.isBool()) {
                        filteredButtonsObj[buttonKey] = value;
                    }
                }
                filteredKeyboardObj[keyboardKey] = filteredButtonsObj;
            } else {
                filteredKeyboardObj[keyboardKey] = keyboardObj[keyboardKey];
            }
        }
        filteredObj[key] = filteredKeyboardObj;
    }

    m_jsonDoc.setObject(filteredObj);
    normalizePath();
    return true;
}

bool GlobalConfig::loadConfig()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    file.close();

    if (!loadConfigFromText(data)) {
        return false;
    }

    // 保留原有的保存逻辑
    QSaveFile saveFile(m_configPath);
    if (!saveFile.open(QIODevice::WriteOnly))
        return false;

    saveFile.write(m_jsonDoc.toJson());
    return saveFile.commit();
}

bool GlobalConfig::saveConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    normalizePath();
    file.write(m_jsonDoc.toJson());
    qDebug()<<m_jsonDoc<<'\n';
    loadTranslation(m_jsonDoc["language"].toString());
    return true;
}

void GlobalConfig::normalizePath() {
    QJsonObject obj = m_jsonDoc.object();

    auto normalizeHelper = [](const QString& path) -> QString {
        if (path.isEmpty()) return path;  // 保持空值不变

        // 转换为本地分隔符并清理路径
        QString normalized = QDir::toNativeSeparators(QDir::cleanPath(path));


        return normalized;
    };

    if (obj.contains("adbPath") && obj["adbPath"].isString())
        obj["adbPath"] = normalizeHelper(obj["adbPath"].toString());

    if (obj.contains("sndcpyApkPath") && obj["sndcpyApkPath"].isString())
        obj["sndcpyApkPath"] = normalizeHelper(obj["sndcpyApkPath"].toString());

    m_jsonDoc.setObject(obj);
}

QString GlobalConfig::adbPath() const {
    return m_jsonDoc.object()["adbPath"].toString();
}

void GlobalConfig::setAdbPath(const QString& path) {
    QJsonObject obj = m_jsonDoc.object();
    obj["adbPath"] = path;
    m_jsonDoc.setObject(obj);
}

QString GlobalConfig::sndcpyApkPath() const {
    return m_jsonDoc.object()["sndcpyApkPath"].toString();
}

void GlobalConfig::setSndcpyApkPath(const QString& path) {
    QJsonObject obj = m_jsonDoc.object();
    obj["sndcpyApkPath"] = path;
    m_jsonDoc.setObject(obj);
}

int GlobalConfig::sndcpyPort() const {
    return m_jsonDoc.object()["sndcpyPort"].toInt();
}

void GlobalConfig::setSndcpyPort(int port) {
    QJsonObject obj = m_jsonDoc.object();
    obj["sndcpyPort"] = port;
    m_jsonDoc.setObject(obj);
}

QString GlobalConfig::deviceSerial() const {
    return m_jsonDoc.object()["deviceSerial"].toString();
}

void GlobalConfig::setDeviceSerial(const QString& ser) {
    QJsonObject obj = m_jsonDoc.object();
    obj["deviceSerial"] = ser;
    m_jsonDoc.setObject(obj);
}

void GlobalConfig::initDefaultConfig() {
    QJsonObject obj, layout;
    layout["width"] = 800;
    layout["height"] = 80;

    obj["adbPath"] = "adb.exe";
    obj["sndcpyApkPath"] = "sndcpy.apk";
    obj["sndcpyPort"] = 28200;
    obj["deviceSerial"] = "";
    obj["note"] = tr("此配置不会实时刷新。请尽量从GUI更改配置，不合适的修改可能导致错误。/This configuration will not be refreshed in real-time. Please try to make configuration changes through the GUI, as inappropriate modifications may lead to errors.");
    obj["wndInfoOfAdvancedKeyboard"] = layout;
    m_jsonDoc.setObject(obj);
}

QJsonObject GlobalConfig::advancedKeyboardLayout() {
    return m_jsonDoc["wndInfoOfAdvancedKeyboard"].toObject();
}

void GlobalConfig::setAdvancedKeyboardLayout(const QJsonObject& layout){
    QJsonObject obj = m_jsonDoc.object();
    obj["wndInfoOfAdvancedKeyboard"] = layout;
    m_jsonDoc.setObject(obj);
}

QString GlobalConfig::language() const{
    return m_jsonDoc["language"].toString();
}
