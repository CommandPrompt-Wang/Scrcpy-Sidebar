#include "toolboxmain.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLoggingCategory>

void sendLanguageChangeEvent() {
    QEvent langChangeEvent(QEvent::LanguageChange);
    for (QWidget* widget : qApp->topLevelWidgets()) {
        QCoreApplication::sendEvent(widget, &langChangeEvent);
    }
}

void loadTranslation(const QString& langCode) {
    qDebug() << "Loading language:" << langCode;

    static QTranslator* currentTranslator = nullptr;

    // 移除旧的翻译器
    if (currentTranslator) {
        qApp->removeTranslator(currentTranslator);
        delete currentTranslator;
        currentTranslator = nullptr;
    }

    // 如果是中文（zh_CN, zh_TW 等），直接恢复默认语言
    if (langCode.startsWith("zh")) {
        sendLanguageChangeEvent();
        return;
    }

    // 统一处理英语（en, en_US, en_GB 等都使用 en_US）
    QString actualLangCode = langCode.startsWith("en") ? "en_US" : langCode;

    // 尝试加载新翻译
    QTranslator* translator = new QTranslator(qApp);
    bool loadSuccess = translator->load(":/translations/Scrcpy_Sidebar_" + actualLangCode + ".qm");

    if (loadSuccess) {
        qApp->installTranslator(translator);
        currentTranslator = translator;
        sendLanguageChangeEvent();
    } else {
        // 如果目标语言加载失败，尝试 fallback 到 en_US
        if (actualLangCode != "en_US") {
            qWarning() << "Failed to load translation for" << actualLangCode << ", trying en_US as fallback";
            QTranslator* fallbackTranslator = new QTranslator(qApp);
            if (fallbackTranslator->load(":/translations/Scrcpy_Sidebar_en_US.qm")) {
                qApp->installTranslator(fallbackTranslator);
                currentTranslator = fallbackTranslator;
                sendLanguageChangeEvent();
            } else {
                delete fallbackTranslator;
                qWarning() << "Failed to load fallback translation (en_US)";
            }
        } else {
            qWarning() << "Failed to load translation for en_US";
            delete translator;
        }
    }
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 自动检测系统语言
    QString systemLang = QLocale::system().name();
    loadTranslation(systemLang);

    ToolboxMain w;
    w.show();
    return a.exec();
}
