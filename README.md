# Scrcpy-Sidebar - Android Screen Mirroring Toolkit
A sidebar toolbox of Scrcpy / Scrcpy的侧边工具箱

English(Current) | [中文帮助](https://github.com/CommandPrompt-Wang/Scrcpy-Sidebar/README_zh)

<img width="960" height="476" alt="{CAB404BA-A61F-41F2-927D-6A540DBADB36}" src="https://github.com/user-attachments/assets/c0a94813-3de1-4cec-abfc-e94322254405" />

**Enhance your scrcpy experience with audio forwarding and virtual buttons!**

## ✨ Features
- 🔊 **Audio Forwarding for Android 10+**  
  Built-in sndcpy support (No VLC required)
  
- 🎮 **Virtual Control Panel**  
  - Power/Volume buttons with long-press support  
  - Customizable layout via JSON config  
  - Real-time updates without restart  

- ⚙️ **Convenience Tools**  
  - Keep screen awake (auto-restore settings)  
  - Multi-device serial management  
  - Chinese punctuation auto-correction  

## 📦 Quick Start
1. **Prerequisites**  
   - [scrcpy](https://github.com/Genymobile/scrcpy) installed  
   - Android device with USB debugging enabled

2. **Installation**  
    Just [Download Here](https://github.com/CommandPrompt-Wang/Scrcpy-Sidebar/releases)

3. **Configuration**  
   Edit `config.json`:
   - Windows at `%APPDATA%/Scrcpy-Sidebar/config.json`
   - Linux at `~/.config/scrcpy-sidebar/config.json`
   
   Default json: 
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

## 🛠️ Advanced Usage
- **Adding Custom Buttons**  
  ```json
  "buttons": {
    "Home": "HOME",
    "Back": "BACK"
  }
  ```

- **Keycode Reference**  
  See [Android KeyEvent docs](https://developer.android.com/reference/android/view/KeyEvent)

## 📝 Notes
- Tested on HarmonyOS 3.0 (Android 10)
- If the error "Remote host closed the connection" occurs repeatedly when using sndcpy, please manually kill sndcpy on **your phone** and try again. Or fall back to VLC
```bash
    vlc -Idummy --demux rawaud --network-caching=0 --play-and-exit tcl://localhost:28200   #if you havn't changed the port number
``` 
See [Sndcpy](https://github.com/rom1v/sndcpy?tab=readme-ov-file#requirements) for details

## 📄 License
LGPL-3.0 | © Command_Prompt
