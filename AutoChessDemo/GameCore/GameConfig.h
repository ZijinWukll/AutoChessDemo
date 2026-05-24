#pragma once

#include <cstddef>

namespace synera
{
    // ============================================================
    // 游戏全局常量配置
    // 所有魔法数字集中在此，方便调整平衡性
    // ============================================================

    // ---- 阶段一：棋盘 ----
    constexpr int DEFAULT_BOARD_ROWS = 8;
    constexpr int DEFAULT_BOARD_COLS = 8;

    // ---- 阶段一：备战区 ----
    constexpr size_t BENCH_CAPACITY = 8;

    // ---- 阶段二：战斗 ----
    constexpr int FPS = 60;                         // 游戏帧率
    constexpr int DEFAULT_ATTACK_COOLDOWN = 20;     // 攻击间隔帧数（约 0.33 秒）
    constexpr int MAX_MANA = 60;                    // 默认最大法力值
    constexpr int MANA_PER_ATTACK = 10;             // 每次普攻回复法力
    constexpr int MANA_PER_HIT = 10;                // 每次受击回复法力
    constexpr int SKILL_CAST_THRESHOLD = 60;        // 满法力施法阈值

    // ---- 阶段二：寻路 ----
    constexpr int MOVE_SPEED = 2;                   // 每帧移动格数（用于动画插值）

    // ---- 阶段三：商店 ----
    constexpr int SHOP_SIZE = 5;                    // 商店每回合刷新数量
    constexpr int FREE_REFRESH_PER_ROUND = 1;       // 每回合免费刷新次数
    constexpr int MAX_SHOP_LEVEL = 9;               // 商店最高等级

    // ---- 阶段三：合星 ----
    constexpr int MERGE_COUNT = 3;                  // 3 个同名同星合成升星

    // ---- 阶段三：装备 ----
    constexpr int EQUIP_DROP_INTERVAL_MIN = 4;      // 装备掉落最小间隔回合
    constexpr int EQUIP_DROP_INTERVAL_MAX = 6;      // 装备掉落最大间隔回合

    // ---- 游戏轮数 ----
    constexpr int MAX_WAVES = 15;                    // 总波数（第 15 波为 Boss 关）

    // ---- 阶段三：羁绊 ----
    constexpr int SYNERGY_THRESHOLD_1 = 2;           // 羁绊生效阈值档位1
    constexpr int SYNERGY_THRESHOLD_2 = 4;           // 羁绊生效阈值档位2
}
