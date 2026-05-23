#pragma once

#include <memory>
#include <vector>

#include "Board.h"
#include "Unit.h"
#include "PathFinder.h"

namespace synera
{
    // 战斗系统：管理 PvE 自动战斗循环。
    // 负责单位状态机转换（Idle → Moving → Attacking → Casting → Dead）、
    // 寻路、攻击、技能施法等核心战斗逻辑。
    class CombatSystem
    {
    public:
        CombatSystem(Board& board, PathFinder& pathFinder);

        // ---- 初始化战斗 ----
        // 将玩家单位和敌方单位部署到棋盘，开始战斗。
        void StartCombat(const std::vector<std::shared_ptr<Unit>>& playerUnits,
                         const std::vector<std::shared_ptr<Unit>>& enemyUnits);

        // ---- 每帧更新 ----
        // 60 fps 调用，驱动所有单位的行动。
        // #TODO: 实现主循环逻辑
        void Update();

        // ---- 战斗状态 ----
        bool IsCombatActive() const;

    private:
        // ---- 单位管理 ----
        // 获取战场上所有存活单位。
        std::vector<std::shared_ptr<Unit>> GetAllAliveUnits() const;

        // 获取某个单位的所有敌方存活单位。
        std::vector<std::shared_ptr<Unit>> GetEnemiesFor(const Unit& unit) const;

        // ---- 核心行为 ----
        void ProcessUnit(std::shared_ptr<Unit> unit);
        void PerformAttack(std::shared_ptr<Unit> unit, std::shared_ptr<Unit> target);
        void MoveTowardEnemy(std::shared_ptr<Unit> unit);

        // ---- 辅助方法 ----
        // 在 enemies 中找离 unit 最近的存活敌人（曼哈顿距离）
        std::shared_ptr<Unit> FindNearestEnemy(const Unit& unit, const std::vector<std::shared_ptr<Unit>>& enemies) const;
        int ManhattanDist(const Unit& a, const Unit& b) const;

        // 在无法寻路时，向最近的敌人曼哈顿方向移动一格（绕过障碍）
        bool MoveOneStepToward(const Unit& unit, const Position& target);

        // 将敌方单位自动部署到棋盘上半区
        void DeployEnemiesToBoard(const std::vector<std::shared_ptr<Unit>>& enemyUnits);

        // ---- 超时保护 ----
        int m_combatFrameCount = 0;
        static constexpr int MAX_COMBAT_FRAMES = 3000;  // ~30s at 100fps

        // ---- 引用 ----
        Board& m_board;
        PathFinder& m_pathFinder;
        bool m_active = false;
    };
}
