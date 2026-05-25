# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Synera: Synergy Auto-Arena** — 自走棋（Auto-battler）桌面应用，使用 Qt 6.8 + MSVC 2022，Visual Studio 2022 项目。

灵感来源于《The Last Flame》和《Teamfight Tactics》。

### 评分权重
- 阶段一（基础框架）：70%
- 阶段二（战斗系统）：10%
- 阶段三（经济/商店/羁绊）：10%
- 阶段四（额外功能+README）：10%

## Build Commands

- 在 Visual Studio 中直接打开 `AutoChessDemo.slnx` 按 **F5** 构建并运行
- 命令行构建（需 VS 开发者命令提示符）：
  ```bash
  msbuild AutoChessDemo.slnx /p:Configuration=Debug /p:Platform=x64
  ```
- Release 构建：
  ```bash
  msbuild AutoChessDemo.slnx /p:Configuration=Release /p:Platform=x64
  ```

## Project Architecture

### 阶段一：基础框架（当前阶段）
- **M×N 棋盘**（默认 8×8），每个格子 1×1，同一时刻最多容纳一个单位
- **备战区 Bench**：8 格，用于存放待上场单位
- **单位 Unit** 属性：HP、ATK、Range（攻击范围）、Max Mana、Mana
- **归属 Owner**：PlayerCtrl / EnemyCtrl
- **羁绊 Traits**：字符串标签，用于触发羁绊效果
- **战场划分**：上半区（EnemyHalf = x < rows/2）、下半区（PlayerHalf = x >= rows/2）
- **敌方单位集合 Er**：每波 PvE 自动生成
- **GUI**：显示棋盘网格、备战区、单位信息、敌方单位

### 阶段二：战斗系统
- 自动战斗（PvE），单位自动搜索敌人并行动
- **单位状态机 FSM**：Idle → Moving → Attacking → Casting → Dead
- **寻路**：BFS / A* 算法，TileMap 网格寻路
- **敌对判定**：`Oppr(u) = {v ∈ Ur | owner(v) ≠ owner(u), v 存活}`
- 攻击间隔、技能施法（3-5 格范围，AOE）
- AI 自动行动循环（60 fps `update()`）

### 阶段三：经济 / 商店 / 羁绊
- **Shop**：每回合刷新 5 个单位，前 2 次免费刷新，之后逐次递增 1 金币。商店等级提升后高费概率大幅增加：
  - 等级1: 1费100%
  - 等级2: 1费65% / 2费30% / 3费5%
  - 等级3: 1费40% / 2费35% / 3费25%
  - 等级4: 1费20% / 2费35% / 3费45%
  - 等级5: 1费10% / 2费30% / 3费60%
  - 等级6+: 1费5% / 2费20% / 3费75%
- **羁绊 Synergy**：统计场上同羁绊数量达到阈值（2/4），战斗前自动给场上单位叠加 ATK/HP/Range 加成（不再只是装饰）
- **合星 Merge**：3 个同名 1★ → 1 个 2★（属性×2.0），3 个同名 2★ → 1 个 3★（属性×3.0）
- **装备 Equipment**：每 4-6 回合掉落，2 件低级合成 1 件高级
- **出售**：点击单位后详情面板底部显示「出售」按钮，或右键备战区出售
- **公式**：胜利金币=`波数×2+8`，失败金币=`波数×2+5`，扣血=`波数×4`，胜利回血=`波数×2`（仅HP<100时）
- **商店刷新**：前 2 次免费，之后每次费用 = 已刷新次数 - 1（第3次=1金，第4次=2金...）

### 阶段四：额外功能
- 15 波 PvE 战役，第 15 波 Boss 战（Boss HP 3024 / ATK 176 @ 3★）
- 动画弹道系统（Projectiles）
- PvP Socket 对战
- README 文档

### 目录结构

```
AutoChessDemo.slnx              # 解决方案文件
CLAUDE.md
AutoChessDemo/
├── main.cpp                    # 入口：QApplication + 主窗口
├── AutoChessDemo.h/.cpp/.ui    # 主窗口（Qt Designer UI）
├── AutoChessDemo.qrc           # Qt 资源文件
├── assets/                     # 游戏资源（图片等）
│   ├── units/
│   ├── ui/
│   └── effects/
├── GameCore/                   # 游戏核心逻辑（无 Qt 依赖）
│   ├── Types.h                 # 公共枚举和类型别名
│   ├── Position.h              # 棋盘坐标结构体
│   ├── GameConfig.h            # 游戏常量配置
│   ├── Unit.h/.cpp             # 单位实体
│   ├── Board.h/.cpp            # M×N 网格棋盘
│   ├── Bench.h/.cpp            # 备战区（线性槽位）
│   ├── PathFinder.h/.cpp       # 寻路算法（BFS/A*）
│   ├── CombatSystem.h/.cpp     # 战斗逻辑/状态机
│   ├── AIController.h/.cpp     # AI 决策
│   ├── ShopSystem.h/.cpp       # 商店系统
│   ├── SynergySystem.h/.cpp    # 羁绊系统
│   ├── MergeSystem.h/.cpp      # 合星系统
│   ├── Equipment.h/.cpp        # 装备系统
│   └── GameManager.h/.cpp      # 顶层游戏状态管理
└── UI/                         # Qt UI 组件
    ├── BoardWidget.h/.cpp      # 棋盘渲染
    ├── BenchWidget.h/.cpp      # 备战区渲染
    ├── UnitInfoWidget.h/.cpp   # 单位详情面板
    └── ShopWidget.h/.cpp       # 商店面板
```

### 阶段三数据参考
- 单位基础属性模板（1 费）：
  - ATK: 20~55, HP: 200~500
  - 星级倍率：1★=1.0x → 2★=2.0x → 3★=3.0x（已加强）
- 经济公式：胜利金币=`波数×2+8`，失败金币=`波数×2+5`，扣血=`波数×4`，胜利回血=`波数×2`
- 商店刷新：前 2 次免费，第 3 次起每次费用 = 已刷新次数 - 1（第3次=1金，第4次=2金...）
- 总波数：15（第15波 Boss 关）
- 装备属性示例：ATK+15, HP+150, 闪避+20%, 减甲-30
- 恢复规则：Mana 满时施法（不普攻），Mana 为 0 时普攻积攒 Mana
  - 每 10 点 Mana 可施法，满 60 自动释放
  - 普攻一次 +10 Mana，受伤也加 Mana

### 命名约定
- 所有核心逻辑代码位于 `synera` 命名空间
- Board 使用 `shared_ptr<Unit>` 管理单位生命周期
- UI 层通过信号/槽与 GameCore 层交互

## 打包发布

### 前置条件
- 确保已用 Visual Studio 2022 以 **Release** 方式成功构建（`msbuild AutoChessDemo.slnx /p:Configuration=Release /p:Platform=x64`）
- 需要 Qt 6.8 的 `windeployqt` 工具（通常在开始菜单中有 "Qt 6.8.3 (MSVC 2022 64-bit)" 命令提示符）

### 打包步骤

1. 生成输出在 `x64\Release\SYNERA.exe`

2. 运行 windeployqt 收集依赖：
   ```bash
   cd E:\cpp项目(VS用）\AutoChessDemo\x64\Release
   windeployqt SYNERA.exe
   ```

3. （可选）建立干净发布目录：
   ```bash
   mkdir E:\SyneraPublish
   copy x64\Release\SYNERA.exe E:\SyneraPublish\
   cd E:\SyneraPublish
   windeployqt SYNERA.exe
   ```

4. 将整个发布文件夹压缩为 ZIP，发送给好友

### 运行环境要求
- Windows 64-bit
- 如缺少 VC++ DLL 报错，需安装 [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)
- 游戏资源已内嵌到 exe（Qt 资源系统），无需额外携带
- BGM/音效 WAV 文件首次运行时自动生成到系统临时目录

### 修改后重新打包
源代码和项目文件不受打包影响，可随时修改 → F5 调试 → Release 构建 → 重新 windeployqt → 替换旧文件发送

