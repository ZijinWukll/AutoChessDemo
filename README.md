# Synera: Synergy Auto-Arena

一款基于 **Qt 6.8 + MSVC 2022** 的桌面自走棋（Auto-Battler）游戏。

> 灵感来源于《The Last Flame》和《Teamfight Tactics》。

---

## 快速开始

### 环境要求

| 工具 | 版本 |
|------|------|
| Visual Studio | 2022（含"使用 C++ 的桌面开发"工作负载） |
| Qt | 6.8.3 (MSVC 2022 64-bit) |
| Windows SDK | 10.0 |

### 构建 & 运行

1. **安装 Qt 6.8.3**，选择 MSVC 2022 64-bit 组件
2. **配置 Qt MSBuild**：Visual Studio 中打开菜单 `扩展 → Qt VS Tools → Qt Versions`，添加 `D:\Qt\6.8.3\msvc2022_64`
3. **打开解决方案**：双击 `AutoChessDemo.slnx`
4. **按 F5** 构建并运行

### 命令行构建

打开"VS 开发者命令提示符"：

```bash
msbuild AutoChessDemo.slnx /p:Configuration=Debug /p:Platform=x64
```

Release 构建：

```bash
msbuild AutoChessDemo.slnx /p:Configuration=Release /p:Platform=x64
```

---

## 游戏玩法

### 基本流程

游戏共 **10 波** PvE 回合，每回合分为两个阶段：

1. **准备阶段** — 从商店购买单位，部署到棋盘上
2. **战斗阶段** — 单位自动与敌方 AI 交战

击败所有敌人即为胜利，击败全部 10 波即为通关。

### 操作指南

| 操作 | 方式 |
|------|------|
| 购买单位 | 点击商店中的单位卡片，确认购买 |
| 部署上场 | 从**备战区**拖拽单位到棋盘我方半场（绿色区域） |
| 撤回备战区 | 从**棋盘**拖拽我方单位到备战区 |
| 出售单位 | 右键点击**备战区**中的单位 |
| 刷新商店 | 点击「刷新商店」按钮 |
| 开始战斗 | 部署完成后点击「开始战斗」 |

### 经济系统

- 每回合胜利获得金币奖励（随波数递增）
- 失败获得 5 金币补偿
- 出售单位返还金币（费用 × 星级倍率：1★×1，2★×3，3★×9）
- 金币用于购买单位和刷新商店

### 合星系统

- **3 个同名 1★ 单位** → **1 个 2★ 单位**
- **2 个同名 2★ + 1 个同名 1★** → **1 个 3★ 单位**
- 星级提升大幅增强单位属性

### 羁绊系统

每个单位拥有**羁绊标签**（如"人类"、"战士"、"法师"等），场上同羁绊数量达到阈值时激活队伍加成。

### 装备系统

每 4-6 回合胜利可掉落装备，装备可提升单位属性。

---

## 项目结构

```
AutoChessDemo/
├── AutoChessDemo.slnx              # 解决方案文件
├── CLAUDE.md                       # 项目说明（Claude Code）
├── README.md                       # 本文档
├── .gitignore
│
├── AutoChessDemo/                  # 项目主目录
│   ├── main.cpp                    # 入口：QApplication + 主窗口
│   ├── AutoChessDemo.h/.cpp       # 主窗口（UI 逻辑 + 游戏循环）
│   ├── AutoChessDemo.ui            # Qt Designer 界面布局
│   ├── AutoChessDemo.qrc           # Qt 资源文件
│   ├── AutoChessDemo.vcxproj       # VS 项目文件
│   ├── Icon.rc                     # 应用图标资源
│   ├── gameicon.ico                # 应用图标
│   │
│   ├── GameCore/                   # 游戏核心逻辑（无 Qt 依赖）
│   │   ├── Types.h                 # 公共枚举和类型别名
│   │   ├── Position.h              # 棋盘坐标
│   │   ├── GameConfig.h            # 游戏常量
│   │   ├── Unit.h/.cpp             # 单位实体
│   │   ├── Board.h/.cpp            # M×N 棋盘网格
│   │   ├── Bench.h/.cpp            # 备战区
│   │   ├── PathFinder.h/.cpp       # 寻路算法（BFS）
│   │   ├── CombatSystem.h/.cpp     # 战斗系统
│   │   ├── AIController.h/.cpp     # AI 决策
│   │   ├── ShopSystem.h/.cpp       # 商店系统
│   │   ├── SynergySystem.h/.cpp    # 羁绊系统
│   │   ├── MergeSystem.h/.cpp      # 合星系统
│   │   ├── Equipment.h/.cpp        # 装备系统
│   │   ├── AudioManager.h/.cpp     # 音效管理
│   │   └── GameManager.h/.cpp      # 顶层游戏状态管理
│   │
│   ├── UI/                         # Qt 界面组件
│   │   ├── BoardWidget.h/.cpp      # 棋盘渲染
│   │   ├── BenchWidget.h/.cpp      # 备战区渲染
│   │   ├── ShopWidget.h/.cpp       # 商店面板
│   │   └── UnitInfoWidget.h/.cpp   # 单位详情
│   │
│   └── assets/                     # 游戏资源（贴图等）
│       ├── units/                  # 单位精灵图
│       ├── ui/                     # UI 元素
│       └── effects/                # 特效
│
└── x64/                            # 构建输出（git 忽略）
```

---

## 技术栈

| 层面 | 技术 |
|------|------|
| 语言 | C++17 |
| UI 框架 | Qt 6.8 (Widgets + QPainter) |
| 构建工具 | MSBuild / Visual Studio 2022 |
| 音效 | Windows PlaySound API (winmm) |
| 寻路 | BFS 网格寻路 |
| 战斗 | 状态机 FSM（Idle → Moving → Attacking → Casting → Dead） |

### 核心设计

- **纯逻辑与 UI 分离**：`GameCore/` 下所有代码不依赖 Qt，可在无 GUI 环境下复用
- **自绘界面**：所有 UI 组件使用 QPainter 自定义绘制，不依赖 QML
- **游戏循环**：10ms 定时器驱动（~100fps），`GameManager::Update()` 驱动战斗逻辑

---

## 打包发布

1. 以 **Release** 方式构建：`msbuild AutoChessDemo.slnx /p:Configuration=Release /p:Platform=x64`
2. 输出位于 `x64/Release/SYNERA.exe`
3. 运行 windeployqt 收集依赖：
   ```bash
   cd x64/Release
   windeployqt SYNERA.exe
   ```
4. 将整个文件夹压缩，发送给好友即可
5. 如果对方缺少 VC++ DLL，安装 [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

---

## 开发日志

- **阶段一**：基础框架（棋盘、备战区、单位、部署/拖拽）
- **阶段二**：战斗系统（寻路、AI、状态机、FSM）
- **阶段三**：经济系统（商店、合星、羁绊、装备）
- **阶段四**：额外功能（Boss 战、弹道动画、音效、UI 美化）
