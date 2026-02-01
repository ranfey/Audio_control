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

## ✨ 特性 | Features

*   **🍬 萌系 UI 设计**：精致的界面布局，视觉舒适，看着就很解压。
*   **🌊 极致丝滑**：
    *   内置平滑滚动算法 (`SmoothScrollArea`)，列表滑动如丝般顺滑。
    *   精心调教的动画过渡效果，拒绝生硬的弹窗。
*   **🚀 轻量 & 高效**：
    *   基于 **C++** 原生开发，极低的内存和 CPU 占用。
    *   启动迅速，瞬间响应你的每一次点击。
*   **🎚️ 强大的应用级控制**：
    *   智能识别系统中的音频会话。
    *   **单独控制**每一个应用程序的音量，想静音谁就静音谁！
*   **🧩 系统深度集成**：
    *   完美支持 Windows 系统托盘。
    *   实时监听系统音频状态变化。

## 🛠️ 技术栈 | Tech Stack

本项目基于现代 C++ 和 Qt 框架构建，融合了 Windows 原生 API 技术。

| 核心技术 | 说明 |
| :--- | :--- |
| **C++ 11** | 坚实的底层语言，保证高性能与低延迟 |
| **Qt 5 (Widgets)** | 强大的跨平台 GUI 框架，构建精美界面 |
| **Qt WinExtras** | 深度集成 Windows 特有的任务栏与窗口功能 |
| **WASAPI (Windows API)** | 微软核心音频 API (`MMDeviceAPI`, `AudioPolicy`)，实现精准音量控制 |
| **CMake** | 现代化的项目构建系统 |

### 🔧 核心技术点解析

1.  **音频会话管理 (Audio Session Management)**
    *   通过 Windows Core Audio APIs (`IAudioSessionManager2`, `ISimpleAudioVolume`) 深入系统底层，获取并控制每个进程的音频流。
    *   实时监听音频端点 (Endpoint) 的状态变化。

2.  **流畅交互体验 (Smooth Experience)**
    *   重写 `QScrollArea` 实现动量滚动和平滑阻尼效果。
    *   使用 `QPropertyAnimation` 实现窗口的淡入淡出和位置变换。
    *   利用 `QGraphicsDropShadowEffect` 绘制柔和的阴影，提升 UI 层次感。

3.  **系统级交互**
    *   系统托盘 (`QSystemTrayIcon`) 的深度定制。
    *   Windows 消息循环处理与原生事件响应。

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

### 🔨环境要求 | Environment
*   Windows 10/11
*   Qt 5.15+
*   Visual Studio 2019+ (MSVC 编译器)
*   CMake 3.10+


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
---

<p align="center">
  README by <b>Gemini 3 Pro</b>
</p>
