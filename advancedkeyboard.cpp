#include "advancedkeyboard.h"
#include "ui_advancedkeyboard.h"

AdvancedKeyboard::AdvancedKeyboard(GlobalConfig* config, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AdvancedKeyboard)
    , m_lastPos(QPoint(0, 0))
    , m_lastsize(QSize(560,80))
    , conf(config)
    , longPressTimer(new QTimer(this))
    , currentPressedButton(nullptr)
{
    ui->setupUi(this);

    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                   Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint|
                   Qt::Tool);

    proc = new ExternalProcess(this);
    longPressTimer->setSingleShot(true);

    connect(proc, &ExternalProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus, const QString &output){
                emit settingProcessFinished(exitCode, exitStatus, output);
            });

    connect(longPressTimer, &QTimer::timeout, this, &AdvancedKeyboard::handleLongPress);

    updateButtonLayout();
}

void AdvancedKeyboard::updateButtonLayout()
{
    // 清除现有按钮
    QLayoutItem* item;
    while ((item = ui->horizontalLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    buttonKeyMap.clear();

    // 从配置加载布局
    QJsonObject layout = conf->advancedKeyboardLayout();

    // 设置窗口大小
    if (layout.contains("width") && layout.contains("height")) {
        resize(layout["width"].toInt(), layout["height"].toInt());
    }

    // 添加配置的按钮
    if (layout.contains("buttons")) {
        QJsonObject buttons = layout["buttons"].toObject();
        for (auto it = buttons.begin(); it != buttons.end(); ++it) {
            QString label = it.key();
            QString key = it.value().toString();

            QPushButton *btn = new QPushButton(label, this);
            btn->setProperty("keyCode", key);  // 存储keyCode
            btn->setContextMenuPolicy(Qt::CustomContextMenu);

            // 短按/长按处理
            connect(btn, &QPushButton::pressed, [this, btn]() {
                currentPressedButton = btn;
                longPressTimer->start(500); // 500ms长按阈值
            });

            connect(btn, &QPushButton::released, [this, btn]() {
                if (longPressTimer->isActive()) {
                    longPressTimer->stop();
                    QString key = btn->property("keyCode").toString();
                    sendAdbKey(key, false);
                }
                currentPressedButton = nullptr;
            });

            // 右键菜单
            connect(btn, &QPushButton::customContextMenuRequested,
                    this, &AdvancedKeyboard::showContextMenu);

            ui->horizontalLayout->addWidget(btn);
            buttonKeyMap[btn] = key;
        }
    }

    // 添加"+"按钮
    QPushButton *addButton = new QPushButton("+", this);
    addButton->setProperty("isAddButton", true);
    connect(addButton, &QPushButton::clicked, this, &AdvancedKeyboard::showAddButtonDialog);
    ui->horizontalLayout->addWidget(addButton);
}

void AdvancedKeyboard::handleLongPress()
{
    if (!currentPressedButton || !buttonKeyMap.contains(currentPressedButton))
        return;

    QString key = buttonKeyMap[currentPressedButton];
    sendAdbKey(key, true);
}

void AdvancedKeyboard::sendAdbKey(const QString &keyevent, bool isLongPress)
{
    QString adb = conf->adbPath();

    QStringList args;
    if (!conf->deviceSerial().isEmpty()) {
        args << conf->deviceSerial();
    }
    args << "shell" << "input" << "keyevent";
    if (isLongPress) {
        args << "--longpress";
    }
    args << keyevent;

    proc->set(adb, args, this);
    proc->run();
}

bool AdvancedKeyboard::validateKeyCode(const QString &key)
{
    static const QStringList validKeys =
        {"UNKNOWN", "MENU", "SOFT_RIGHT", "HOME", "BACK", "CALL", "ENDCALL",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "STAR", "POUND", "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "DPAD_CENTER",
        "VOLUME_UP", "VOLUME_DOWN", "POWER", "CAMERA", "CLEAR",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "COMMA", "PERIOD", "ALT_LEFT", "ALT_RIGHT", "SHIFT_LEFT", "SHIFT_RIGHT",
        "TAB", "SPACE", "SYM", "EXPLORER", "ENVELOPE", "ENTER", "DEL", "GRAVE", "MINUS", "EQUALS",
        "LEFT_BRACKET", "RIGHT_BRACKET", "BACKSLASH", "SEMICOLON", "APOSTROPHE", "SLASH",
        "AT", "NUM", "HEADSETHOOK", "FOCUS", "PLUS", "MENU", "NOTIFICATION", "SEARCH", "TAG_LAST_KEYCODE"
    };
    return validKeys.contains(key);
}

void AdvancedKeyboard::showAddButtonDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("添加按钮"));
    dialog.setWindowModality(Qt::WindowModal); // 设置为模态对话框

    QFormLayout form(&dialog);
    QLineEdit labelEdit(&dialog);
    QLineEdit keyEdit(&dialog);

    form.addRow(tr("按钮标签:"), &labelEdit);
    form.addRow(tr("按键代码:"), &keyEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString label = labelEdit.text().trimmed();
        QString key = keyEdit.text().trimmed().toUpper();

        if (label.isEmpty() || key.isEmpty()) {
            QMessageBox::warning(this, tr("错误"), tr("标签和按键代码不能为空"));
            return;
        }

        if (!validateKeyCode(key)) {
            QMessageBox::warning(this, tr("错误"), tr("无效的按键代码"));
            return;
        }

        // 更新配置
        QJsonObject layout = conf->advancedKeyboardLayout();
        QJsonObject buttons = layout["buttons"].toObject();
        buttons[label] = key;
        layout["buttons"] = buttons;
        conf->setAdvancedKeyboardLayout(layout);

        if (!conf->saveConfig()) {
            QMessageBox::warning(this, tr("错误"), tr("无法保存配置"));
            return;
        }

        updateButtonLayout();
    }
}

void AdvancedKeyboard::showContextMenu(const QPoint &pos)
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn || btn->property("isAddButton").toBool()) return;

    QMenu menu;
    QAction* editAction = menu.addAction(tr("编辑"));
    QAction* deleteAction = menu.addAction(tr("删除"));

    // 将按钮指针存储为Action的data
    editAction->setData(QVariant::fromValue(static_cast<QWidget*>(btn)));
    deleteAction->setData(QVariant::fromValue(static_cast<QWidget*>(btn)));

    connect(editAction, &QAction::triggered, this, &AdvancedKeyboard::editButton);
    connect(deleteAction, &QAction::triggered, this, &AdvancedKeyboard::removeButton);

    menu.exec(btn->mapToGlobal(pos));
}

void AdvancedKeyboard::editButton()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    // 从action的data中获取按钮指针
    QPushButton* btn = qobject_cast<QPushButton*>(action->data().value<QWidget*>());
    if (!btn || !buttonKeyMap.contains(btn)) return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("编辑按钮"));
    dialog.setWindowModality(Qt::WindowModal); // 设置为模态对话框

    QFormLayout form(&dialog);
    QLineEdit labelEdit(btn->text(), &dialog);
    QLineEdit keyEdit(buttonKeyMap[btn], &dialog);

    form.addRow(tr("按钮标签:"), &labelEdit);
    form.addRow(tr("按键代码:"), &keyEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString newLabel = labelEdit.text().trimmed();
        QString newKey = keyEdit.text().trimmed().toUpper();

        if (newLabel.isEmpty() || newKey.isEmpty()) {
            QMessageBox::warning(this, tr("错误"), tr("标签和按键代码不能为空"));
            return;
        }

        if (!validateKeyCode(newKey)) {
            QMessageBox::warning(this, tr("错误"), tr("无效的按键代码"));
            return;
        }

        QJsonObject layout = conf->advancedKeyboardLayout();
        QJsonObject buttons = layout["buttons"].toObject();

        // 如果标签改变了，先删除旧的
        if (newLabel != btn->text()) {
            buttons.remove(btn->text());
        }

        buttons[newLabel] = newKey;
        layout["buttons"] = buttons;
        conf->setAdvancedKeyboardLayout(layout);

        if (!conf->saveConfig()) {
            QMessageBox::warning(this, tr("错误"), tr("无法保存配置"));
            return;
        }

        updateButtonLayout();
    }
}

void AdvancedKeyboard::removeButton()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    // 从action的data中获取按钮指针
    QPushButton* btn = qobject_cast<QPushButton*>(action->data().value<QWidget*>());
    if (!btn || !buttonKeyMap.contains(btn)) return;

    if (QMessageBox::question(this, tr("确认删除"),
                              tr("确定要删除按钮 '%1' 吗?").arg(btn->text()),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QJsonObject layout = conf->advancedKeyboardLayout();
    QJsonObject buttons = layout["buttons"].toObject();
    buttons.remove(btn->text());
    layout["buttons"] = buttons;
    conf->setAdvancedKeyboardLayout(layout);

    if (!conf->saveConfig()) {
        QMessageBox::warning(this, tr("错误"), tr("无法保存配置"));
        return;
    }

    updateButtonLayout();
}

void AdvancedKeyboard::closeEvent(QCloseEvent *event)
{
    // 保存当前窗口大小到配置
    QJsonObject layout = conf->advancedKeyboardLayout();
    layout["width"] = width();
    layout["height"] = height();
    conf->setAdvancedKeyboardLayout(layout);
    conf->saveConfig(); // 立即保存

    m_lastPos = pos();
    m_lastsize = size();
    emit closed();
    QWidget::closeEvent(event);
}

void AdvancedKeyboard::showEvent(QShowEvent *event)
{
    QJsonObject layout = conf->advancedKeyboardLayout();

    qDebug() << "launchSize: " <<layout["width"].toInt() << " " <<layout["height"].toInt() <<"\n";

    // if (layout.contains("width") && layout.contains("height")) {
    //     resize(layout["width"].toInt(), layout["height"].toInt());
    // }

    // 恢复上次位置（如果有）
    if (!m_lastPos.isNull()) {
        move(m_lastPos);
    }

    updateButtonLayout();

    QWidget::showEvent(event);
}

AdvancedKeyboard::~AdvancedKeyboard()
{
    delete ui;
}
