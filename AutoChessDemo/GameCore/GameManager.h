#pragma once

#include <memory>
#include <vector>

#include "Types.h"
#include "Board.h"
#include "Bench.h"
#include "GameConfig.h"
#include "PathFinder.h"
#include "CombatSystem.h"
#include "AIController.h"
#include "ShopSystem.h"
#include "SynergySystem.h"
#include "MergeSystem.h"
#include "Equipment.h"

namespace synera
{
    // 游戏管理器：顶层单例，负责管理游戏状态、阶段切换和各子系统协调。
    // 是整个游戏的"大脑"，UI 层通过它与游戏核心交互。
    class GameManager
    {
    public:
        GameManager();
        ~GameManager() = default;

        // ---- 初始化 ----
        // 创建棋盘、备战区，初始化各子系统。
        void Initialize();

        // ---- 游戏阶段切换 ----
        GamePhase GetCurrentPhase() const;
        void StartPreparationPhase();   // 进入准备阶段（新回合）
        void StartCombatPhase();        // 进入战斗阶段（自动开始）

        // ---- 每帧更新 ----
        // UI 每帧调用（60fps）。
        void Update(float deltaTime);

        // ---- 阶段一：布阵操作 ----
        // 从备战区将 unit 放置到棋盘 (row, col)。
        bool DeployUnit(std::shared_ptr<Unit> unit, int row, int col);

        // 从棋盘 (row, col) 撤回单位到备战区。
        bool RecallUnit(int row, int col);

        // 在棋盘上交换两个单位的位置。
        bool SwapUnitsOnBoard(int r1, int c1, int r2, int c2);

        // 在棋盘上移动己方单位到目标格（目标为空则移动，有己方单位则交换）
        bool MoveUnitOnBoard(int fromRow, int fromCol, int toRow, int toCol);

        // ---- 阶段三：商店操作 ----
        ShopSystem& GetShop();
        void BuyUnitFromShop(int index);        // 购买并放入备战区
        void SellUnitFromBench(int slotIndex);  // 出售备战区单位获得金币
        void SellUnit(std::shared_ptr<Unit> unit); // 出售任意己方单位（棋盘或备战区）

        // ---- 阶段三：羁绊系统 ----
        const std::vector<SynergyInfo>& GetActiveSynergies() const;
        void UpdateSynergies();

        // ---- 阶段三：合星 ----
        bool TryMergeUnits();
        bool HasMergeableUnits() const;   // 检查是否有可合星的单位
        // 购买某个单位（同名同星）是否能在满备战区时触化合星（已有2个，买后变3合并腾空间）
        bool CanMergeOnPurchase(const std::string& name, StarLevel star) const;

        // ---- 阶段三：装备 ----
        EquipManager& GetEquipManager();

        // ---- 访问 GameCore 对象 ----
        Board& GetBoard();
        Bench& GetBench();
        int GetCurrentWave() const;
        int GetGold() const;
        void SpendGold(int amount);      // 消耗金币
        void AddGold(int amount);        // 增加金币
        int GetPlayerHp() const;

        // ---- 棋盘人数限制 ----
        int GetMaxDeploySlots() const;           // 当前最大可部署人数
        int GetPurchasedSlots() const { return m_purchasedSlots; }
        int GetCurrentDeployedCount() const;     // 当前棋盘上我方单位数
        bool IsSlotLimitReached() const;         // 是否已达上限
        int GetSlotPurchaseCost() const;         // 当前购买一个席位所需金币
        bool PurchaseSlot();                     // 尝试购买一个席位

        // ---- 全局 Getter ----
        const std::vector<std::shared_ptr<Unit>>& GetAllPlayerUnits() const;

    private:
        // ---- 子系统 ----
        Board m_board;
        Bench m_bench;
        PathFinder m_pathFinder;
        CombatSystem m_combatSystem;
        AIController m_aiController;
        ShopSystem m_shopSystem;
        SynergySystem m_synergySystem;
        MergeSystem m_mergeSystem;
        EquipManager m_equipManager;

        // ---- 游戏状态 ----
        GamePhase m_phase = GamePhase::Preparation;
        int m_currentWave = 1;
        int m_gold = 0;
        int m_playerHp = 100;        // 玩家总血量（战败扣血）
        int m_purchasedSlots = 0;    // 已购买的额外棋盘席位（永久）

        // ---- 单位集合 ----
        std::vector<std::shared_ptr<Unit>> m_playerUnits;   // 玩家所有单位
        std::vector<std::shared_ptr<Unit>> m_enemyWave;     // 当前波次敌方单位

        // ---- 装备背包 ----
        std::vector<Equipment> m_equipmentInventory;

        // ---- 战斗事件缓存（UI 特效用） ----
        std::vector<AttackEvent> m_recentAttackEvents;

        // ---- 战斗结果 ----
        bool m_lastCombatResult = false;
        bool m_gameOver = false;    // 游戏失败（HP=0 或第15波失败）
        bool m_gameWon = false;     // 游戏通关（打完第15波全部）

    public:
        bool GetLastCombatResult() const { return m_lastCombatResult; }
        bool IsGameOver() const { return m_gameOver; }
        bool IsGameWon() const { return m_gameWon; }

        // ---- 攻击事件（供 UI 渲染战斗特效） ----
        const std::vector<AttackEvent>& GetRecentAttackEvents() const { return m_recentAttackEvents; }

        // ---- 内部函数 ----
        // #TODO: 战斗结束后的处理（扣血、发放金币、掉落装备）
        void OnCombatEnd(bool playerWon);

        // 战斗前给己方单位应用羁绊加成
        void ApplySynergyBonuses();

        // #TODO: 进入下一波
        void NextWave();

        // 战后重置己方单位：按血量排序在我方半场首行排开
        void ResetPlayerUnitsAfterCombat();

        // 战败后：将越界的敌方存活单位移回敌方半场首行
        void RepositionSurvivingEnemies();
    };
}
