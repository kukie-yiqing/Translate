# Translate - Streamlined Desktop Dictionary & Translator
> **Translate** is an ultra-lightweight, native Win32 C++ desktop translation and dictionary tool.
> **Translate** 是一款超轻量级、原生 Win32 C++ 开发的桌面划词翻译与词典工具。

---

## Language / 语言
- [English](#english-documentation)
- [中文文档](#中文文档)

---

# English Documentation

## Introduction
Translate is designed for users who need a clean, distraction-free, and fast desktop translator. With a tiny footprint (under 1.5MB) and no heavy dependencies (like Java, Node.js, or Electron), it stays quiet in your system tray and responds instantly. All icon resources are compiled directly into the binary, meaning the generated `Translate.exe` is 100% self-contained and standalone with zero external dependencies.

## Key Features
- **Ultra Lightweight & Fast**: Written in pure C++ using direct Win32 API and GDI double-buffering. Very low CPU and memory usage.
- **Double Ctrl+C translation**: Simply select any text and press `Ctrl+C` twice to wake up the window and show the translation immediately.
- **Custom Hotkey Wake-up**: Press `Ctrl+Shift+X` (configurable) to wake up the empty window for quick manual searches.
- **Bilingual Phonetics & Speech**: Displays UK/US phonetics for English words and supports real-time text-to-speech pronunciation.
- **Clean Interactive History**: Keeps track of recent queries. Supports clicking to backfill/re-translate and deleting individual or all history records.
- **Dynamic Input Box**: Includes a modern Apple-style clear button (`×`) that appears dynamically when text is present.
- **Intelligent Mixed-Language Translation**: Automatically detects mixed Chinese and English text, utilizing the translation engine's fallback logic to translate sentences based on contextual dominance (e.g. translating "我是Heloise" to "I'm Heloise").
- **Flexible Configuration**: Change the theme accent color, modifier keys, and primary hotkey via `config.ini`.

## Installation & Build
This project can be built using standard Windows LLVM/Clang or MSVC tools.

### Prerequisites
- Windows 10/11
- LLVM Clang or MSVC compiler (supporting C++17 or later)
- Win32 Platform SDK

### Build Instructions
Execute the build batch script in the repository root:
```cmd
.\build.bat
```
This generates the standalone binary `Translate.exe`.

## Configuration (`config.ini`)
The configuration file is automatically created in the executable's directory upon first run.
```ini
[Settings]
ThemeColor=007AFF
HotkeyMod=Ctrl+Shift
HotkeyKey=X
```
- **ThemeColor**: Hex color code (e.g., `007AFF` for Apple Blue, `FF3B30` for Red).
- **HotkeyMod**: Combinations of `Ctrl`, `Shift`, `Alt`, `Win` joined by `+`.
- **HotkeyKey**: The primary activation key (e.g. `X`, `Q`).

---

# 中文文档

## 简介
Translate 专为追求极致轻量、快速、无干扰的桌面翻译用户设计。程序体积小于 1.5MB，无任何笨重的第三方框架（如 Java, Node.js 或 Electron）。它平时静默于系统托盘中，并在被唤醒时提供秒级响应。所有图标资源均已直接编译进二进制文件中，生成的 `Translate.exe` 是一件 100% 独立且完全自包含的单文件，无需依赖任何外部资源或图片文件夹。

## 核心特性
- **极致轻量与性能**：纯 C++ 开发，使用原生 Win32 API 与 GDI 双缓冲绘制，极低的 CPU 和内存占用。
- **划词双击 Ctrl+C 唤醒**：选中任意文本，连续按下两次 `Ctrl+C` 即可自动捕获文本、唤醒主面板并展示翻译。
- **自定义全局快捷键**：默认 `Ctrl+Shift+X` 唤醒空白输入框进行手动检索。
- **双语发音与音标**：提供英/美音标，支持异步高保真真人发音。
- **交互式历史记录**：保存最近查询的词条，支持点击回填/重新翻译，支持单条删除及一键清空。
- **动态输入清空**：输入框内含现代苹果风一键清空按钮，仅在输入框有内容时动态显示。
- **智能中英混译**：自动识别并分析中英文混杂的文本，利用翻译引擎自适应语境的 fallback 机制进行智能互译（例如自动将“我是Heloise”翻译为“I'm Heloise”）。
- **高度个性化**：可通过主目录下的 `config.ini` 修改主题色、唤醒修饰键及热键。

## 编译与构建
本项目可直接使用 Windows 平台下的 LLVM/Clang 或 MSVC 工具链进行构建。

### 开发要求
- Windows 10 或 Windows 11
- LLVM Clang 或 MSVC 编译器 (支持 C++17 或更高版本)
- Windows Platform SDK

### 构建命令
在项目根目录下运行编译批处理文件：
```cmd
.\build.bat
```
编译成功后将生成独立可执行程序 `Translate.exe`。

## 配置文件说明 (`config.ini`)
程序首次运行时会在同级目录下自动生成配置文件：
```ini
[Settings]
ThemeColor=007AFF
HotkeyMod=Ctrl+Shift
HotkeyKey=X
```
- **ThemeColor**: 十六进制主题色（例如 `007AFF` 苹果蓝，`FF3B30` 红色）。
- **HotkeyMod**: 全局快捷键修饰键，支持 `Ctrl`、`Shift`、`Alt`、`Win`，用 `+` 拼接。
- **HotkeyKey**: 全局激活字母键（例如 `X`、`Q`）。

---

## Legal Disclaimer & Legal Risk Mitigation
## 法律免责声明与风险规避

### English
1. **Third-Party API Usage**: This application queries translation services using public, keyless API web endpoints. This software is **NOT** affiliated with, sponsored by, or endorsed by Youdao (NetEase) or any other translation service providers. 
2. **Educational & Personal Use Only**: This project is developed purely for educational, research, and personal productivity purposes. You assume full legal responsibility if you misuse this software, make excessive API queries that burden the endpoint servers, or utilize it for commercial gain.
3. **Hotkey Hooks & Clipboard Emulation**: To enable the "double-Ctrl+C" text capture, this program registers system hotkeys and implements a low-level keyboard hook (`SetWindowsHookEx`) to emulate standard keystrokes. It does **NOT** log, save, or transmit user keystrokes to any third parties. All operations are strictly local.
4. **No Warranties**: This software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement.

### 中文
1. **第三方 API 使用申明**：本软件通过网络公开、免密钥的 API 接口获取翻译与词典数据。本软件**并非**有道（网易集团）或其他翻译服务商的官方发布版本，亦无任何商业授权或隶属关系。
2. **仅限个人学习与研究**：本项目仅用于个人学习、C++ Win32 开发研究和个人效率提升。任何商业性使用、恶意高频并发请求或对接口提供方服务器造成负担的行为，均由使用者本人承担全部法律责任。
3. **热键与剪贴板模拟**：为实现“双击 Ctrl+C”划词捕获，本程序注册了系统级全局热键并注册了底层键盘钩子（`SetWindowsHookEx`）以模拟按键。本程序**绝不**记录、保存或向任何外部服务器发送用户的任何按键记录或个人隐私，所有拦截和模拟操作完全在本地执行。
4. **无担保条款**：本软件按“原样”提供，不提供任何明示或暗示的保证，包括但不限于对适销性、特定用途的适用性和非侵权性的保证。

---

## License / 许可协议
This project is licensed under a custom Non-Commercial License (see [LICENSE](LICENSE) for details).
本项目使用自定义非商业用途许可协议（详情请参阅 [LICENSE](LICENSE)）。
