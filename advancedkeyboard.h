#ifndef ADVANCEDKEYBOARD_H
#define ADVANCEDKEYBOARD_H

#include <QWidget>
#include <QEvent>
#include <QTimer>
#include <QPushButton>
#include <QJsonObject>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <globalconfig.h>
#include <externalprocess.h>

namespace Ui {
class AdvancedKeyboard;
}

class AdvancedKeyboard : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedKeyboard(GlobalConfig* conf, QWidget *parent = nullptr);
    ~AdvancedKeyboard();

public slots:
    void updateButtonLayout();

signals:
    void settingProcessFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &output);
    void closed();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void handleLongPress();
    void showAddButtonDialog();
    void showContextMenu(const QPoint &pos);
    void editButton();
    void removeButton();

private:
    void sendAdbKey(const QString &keyevent, bool isLongPress = false);
    void saveCurrentLayout();
    bool validateKeyCode(const QString &key);

    void changeEvent(QEvent *event) override;

    Ui::AdvancedKeyboard *ui;
    QPoint m_lastPos;
    QSize m_lastsize;
    ExternalProcess *proc;
    GlobalConfig *conf;
    QTimer *longPressTimer;
    QPushButton *currentPressedButton;
    QMap<QPushButton*, QString> buttonKeyMap;
};

#endif // ADVANCEDKEYBOARD_H
