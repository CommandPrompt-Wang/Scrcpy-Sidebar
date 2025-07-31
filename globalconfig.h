#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

class GlobalConfig : public QObject {
    Q_OBJECT
public:
    explicit GlobalConfig(QObject* parent = nullptr);

    // 加载/保存配置
    bool loadConfig();
    bool saveConfig();
    bool loadConfigFromText(QString);

    // 配置项访问器（示例）
    QString adbPath() const;
    void setAdbPath(const QString& path);

    QString sndcpyApkPath() const;
    void setSndcpyApkPath(const QString& path);

    int sndcpyPort() const;
    void setSndcpyPort(int port);

    QString deviceSerial() const;
    void setDeviceSerial(const QString& serial);

    QJsonObject advancedKeyboardLayout();
    void setAdvancedKeyboardLayout(const QJsonObject& layout);

    QString language() const;

    QString getText(){
        return QString::fromUtf8(m_jsonDoc.toJson());
    }

private:
    QString m_configPath;  // 配置文件路径（%APPDATA%/ScrcpyToolbox/config.json）
    QJsonDocument m_jsonDoc; // 私有JSON文档成员
    void normalizePath();

    void initDefaultConfig();
};

#endif // GLOBALCONFIG_H
