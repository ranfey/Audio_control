# ✨ Audio Control ✨

<p align="center">
  <img src="img/icon.png" alt="Audio Control Logo" width="128" height="128">
</p>

<p align="center">
  <b>🌟 一个轻量、流畅且可爱的 Windows 桌面多音量控制小部件 🌟</b>
  <br>
  <br>
  <i>让每一次音量调节都成为一种丝滑的享受 ~ 🎵</i>
</p>

---

## 📖 简介 | Introduction

**Audio Control** 是一个专为 Windows 平台打造的轻量化桌面音量管理工具。它不仅仅是一个功能性的软件，更是一份桌面上的精致点缀！

在这个喧嚣的数字世界里，我们希望能给你带来最**自然、流畅**的听觉管理体验。无论是调节系统总音量，还是单独控制某个调皮的游戏或音乐软件，它都能优雅搞定。乖巧地在桌面，随叫随到，不仅好用，而且**可爱**！( ⚫︎ ‿ ⚫︎ )



<p align="center">
  <img src="img\demo.gif" alt="Audio Control demo">
</p>


## ⚙️ 配置 | Configuration

在安装目录下的 `config.ini` 可进行个性化配置：

```
[Filter]
SkipNames=system,idle,audiodg.exe,runtimebroker.exe
; 过滤不需要显示的进程

[Style]
MaxWindowHeight=500
; 窗口最大高度

RowHeight=75
; 每栏高度

RowWidth=200
; 每栏宽度

MouseThrough=true
Visible=true
Frameless=true
WindowLevel=0
; 其余参数为退出时状态保存
```

<p align="center">
  <img src="img\demo.png" alt="Audio Control demopng">
</p>

## ✨ 特性 | Features

* **🍬 萌系 UI 设计**：精致的界面布局，视觉舒适，看着就很解压。
* **🌊 极致丝滑**：动画过渡效果，拒绝生硬的弹窗。

## 🔨环境要求 | Environment

* Windows 10/11
* Qt 5.14.2
* MSVC 2017
* CMake 3.10+

## 🛠️ 技术栈 | Tech Stack

本项目基于 C++ 和 Qt 框架构建


| 技术栈                   |
| :----------------------- |
| **C++ 11**               |
| **Qt 5 (Widgets)**       |
| **Qt WinExtras**         |
| **WASAPI (Windows API)** |
| **CMake**                |

## 📂 项目结构 | Structure

```text
Audio_control/
├── 📄 CMakeLists.txt       # 项目构建脚本
├── 📄 main.cpp             # 程序入口，生命周期管理
├── 📄 resources.qrc        # 资源文件管理 (图标、样式等)
├── 📂 img/                 # 🎨 图标与图片素材库
│   ├── icon.png
│   ├── icon.ico
│   └── ...
└── 📂 src/                 # 💻 核心源代码
    ├── 🎹 audiocontroller  # [后端] 音频控制核心逻辑，封装 WASAPI
    ├── 🖥️ mainwindow       # [前端] 主界面窗口，负责 UI 呈现与交互
    ├── 🧊 maintray         # [组件] 系统托盘图标与菜单管理
    └── 📝 sessionrow       # [组件] 单个应用程序的音量控制条 (UI + 逻辑)
```

## 🤝 贡献 | Contributing

<a href="https://github.com/ranfey/Audio_control/contributors" target="_blank">
  <img
    src="https://contrib.rocks/image?repo=ranfey/Audio_control"
  />
  <img
    src="img/chatgpt.png"
    alt="ChatGPT"
    title="ChatGPT"
    style="
      height:64px;
      width:64px;
    "
  />
  <img
    src="img/Gemini.png"
    alt="Gemini"
    title="Gemini"
    style="
      height:64px;
      width:64px;
      border-radius:50%;
      vertical-align:middle;
      background:white;
    "
  />
</a>

让我们一起把它变得更棒！(≧∇≦)ﾉ
----------------------------------

<p align="center">
  README by <b>Gemini 3 Pro</b>
</p>
