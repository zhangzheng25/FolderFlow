# FolderFlow

FolderFlow is a lightweight Windows desktop utility for quickly returning to the folders you use most often. It watches the active File Explorer window, records folders that stay in focus for a short moment, and turns that history into a searchable timeline, hot ranking, and favorite list.

FolderFlow 是一个轻量级 Windows 桌面工具，用来快速找回你经常打开的文件夹。它会监听当前处于前台的资源管理器窗口，在你停留一小段时间后记录该目录，并把这些记录整理成时间线、常用排行榜和收藏列表。

The project is built with Qt Widgets and C++17.

本项目基于 Qt Widgets 和 C++17 开发。

## Features / 功能

- Automatically track the active Windows File Explorer folder.
- 自动跟踪当前前台的 Windows 资源管理器目录。
- Show recently visited folders in a timeline.
- 以时间线方式展示最近访问的目录。
- Rank frequently used folders by access count and freshness.
- 根据访问次数和新鲜度生成常用目录排行榜。
- Favorite folders with custom aliases for a compact daily-use list.
- 支持收藏目录并设置别名，方便日常快速打开。
- Pin important folders to the timeline.
- 支持把重要目录置顶到时间线。
- Search by folder path or alias.
- 支持按路径或别名搜索。
- Open folders quickly by double-clicking list items.
- 双击列表项即可快速打开目录。
- Copy paths, open folders in Command Prompt, favorite, unfavorite, pin, and unpin from the context menu.
- 右键菜单支持复制路径、在命令行打开、收藏、取消收藏、置顶和取消置顶。
- Run in the system tray and toggle the main window with a global shortcut.
- 支持托盘运行，并可用全局快捷键显示或隐藏主窗口。
- Configure history retention, ranking weight, query limits, ignored folder names, and startup behavior.
- 支持配置历史保留天数、排行榜权重、查询数量、忽略目录名和开机启动。

## Shortcuts / 快捷键

| Shortcut / 快捷键 | Action / 操作 |
| --- | --- |
| `Ctrl + Space` | Show or hide FolderFlow / 显示或隐藏 FolderFlow |
| `Ctrl + L` | Focus the search box / 聚焦搜索框 |
| `Alt + 1..6` | Open the first 6 timeline entries / 打开时间线前 6 项 |
| `Enter` | Search / 执行搜索 |
| `Esc` | Clear search or hide the window / 清空搜索或隐藏窗口 |

## Build / 构建

### Requirements / 环境要求

- Windows
- Qt 5.15.x for MSVC
- Visual Studio 2019 C++ toolchain
- Git

### Build with qmake / 使用 qmake 构建

Open a Visual Studio Developer Command Prompt or a Qt-enabled terminal, then run:

打开 Visual Studio Developer Command Prompt，或已配置 Qt 环境的终端，然后执行：

```powershell
cd E:\workspace\code\FolderFlow
qmake FolderFlow.pro
nmake
```

For Qt Creator, open `FolderFlow.pro`, select a MSVC Qt kit, then build and run.

如果使用 Qt Creator，直接打开 `FolderFlow.pro`，选择 MSVC Qt Kit 后构建运行即可。

## Runtime Data / 运行数据

FolderFlow stores local settings and history in the current user's application data/config location through Qt's `QStandardPaths`. The SQLite history database and local settings are runtime data and are not meant to be committed to the repository.

FolderFlow 会通过 Qt 的 `QStandardPaths` 把本地配置和历史记录保存到当前用户的应用数据或配置目录中。SQLite 历史数据库和本地设置都属于运行时数据，不应该提交到仓库。

## Repository Notes / 仓库说明

Build outputs and machine-local files are ignored by `.gitignore`, including:

`.gitignore` 已经排除了构建产物和本机配置文件，包括：

- `build/`
- `debug/`
- `release/`
- `.vs/`
- `.qtcreator/`
- generated object/debug/binary files such as `*.obj`, `*.pdb`, `*.exe`, and `*.dll`
- 生成的对象文件、调试文件和二进制文件，例如 `*.obj`、`*.pdb`、`*.exe`、`*.dll`

## Dependencies / 依赖

- Qt Widgets
- Qt SQL
- Windows Shell/COM APIs
- [QHotkey](https://github.com/Skycoder42/QHotkey), included in the repository under `QHotkey/`
- [QHotkey](https://github.com/Skycoder42/QHotkey)，已放在仓库的 `QHotkey/` 目录下

## License / 许可证

No license has been added yet. Add one before distributing or accepting external contributions.

当前还没有添加许可证。如果需要分发项目或接受外部贡献，建议先补充 License。
