# Scrcpy-Sidebar - 安卓投屏工具 Scrcpy 的侧边工具箱

[English](README.md) | 中文(当前)

<img width="960" height="476" alt="{CAB404BA-A61F-41F2-927D-6A540DBADB36}" src="https://github.com/user-attachments/assets/c0a94813-3de1-4cec-abfc-e94322254405" />

**为 scrcpy 添加音频转发和虚拟按键功能！**

## ✨ 功能特性
- 🔊 **Android 10+ 音频转发**  
  内置 sndcpy 支持 (无需 VLC)
  
- 🎮 **虚拟控制面板**  
  - 支持长按操作的按钮  
  - 通过 GUI 或 JSON 配置文件自定义布局
  - 实时生效无需重启  

- ⚙️ **实用工具**  
  - 保持屏幕常亮 (退出自动恢复设置)  
  - 中文标点自动转换  

## 📦 快速开始
1. **准备工作**  
   - 已安装 [scrcpy](https://github.com/Genymobile/scrcpy)  
   - 安卓设备已开启 USB 调试模式

2. **安装**  
   直接[下载最新版本](https://github.com/CommandPrompt-Wang/Scrcpy-Sidebar/releases)

3. **配置**  
   可以编辑 `config.json`:
   - Windows 路径: `%APPDATA%/Scrcpy-Sidebar/config.json`
   - Linux 路径: `~/.config/scrcpy-sidebar/config.json`
   默认配置:
   ```json
   {
    "adbPath": "adb.exe",
    "deviceSerial": "",
    "note": "请尽量从GUI更改配置, 不合适的修改可能导致错误。Try to change the configuration from the GUI. Inappropriate modifications may cause errors.",
    "sndcpyApkPath": "sndcpy.apk",
    "sndcpyPort": 28200,
    "wndInfoOfAdvancedKeyboard": {
        "buttons": {
            "HOME": "HOME"
        },
        "height": 47,
        "width": 259
    }
   ```

## 🛠️ 高级使用
- **添加自定义按钮**  
  ```json
  "buttons": {
    "主页": "HOME",
    "返回": "BACK"
  }
  ```

- **键码参考**  
  查看 [Android KeyEvent 文档](https://developer.android.com/reference/android/view/KeyEvent)

## 📝 注意事项
- 已在鸿蒙 3.0 (Android 10) 测试通过
- 如果使用 sndcpy 时反复出现 "Remote host closed the connection" 错误，请手动在**手机端**终止 sndcpy 进程后重试，或回退使用 VLC
```bash
    vlc -Idummy --demux rawaud --network-caching=0 --play-and-exit tcl://localhost:28200   #如果未更改默认端口号
``` 
详见 [Sndcpy 文档](https://github.com/rom1v/sndcpy?tab=readme-ov-file#requirements)

## 📄 开源协议
LGPL-3.0 | © Command_Prompt

## 🍬 彩蛋
新程序 求测试
> 求你了，来测吧！
![69df16970cdc6106bd01acf9658593ca7dfece20](https://github.com/user-attachments/assets/5735e80e-addf-4b37-9c19-bd2c8114e641)
