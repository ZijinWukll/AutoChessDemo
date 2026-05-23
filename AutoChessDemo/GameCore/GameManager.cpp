#include "GameManager.h"
#include <algorithm>

namespace synera
{
    GameManager::GameManager()
        : m_board(DEFAULT_BOARD_ROWS, DEFAULT_BOARD_COLS)
        , m_bench(BENCH_CAPACITY)
        , m_pathFinder(m_board)
        , m_combatSystem(m_board, m_pathFinder)
    {
    }

    void GameManager::Initialize()
    {
        m_board.Clear();
        m_bench = Bench(BENCH_CAPACITY);
        m_gold = 10;
        m_currentWave = 1;
        m_playerHp = 100;
        m_phase = GamePhase::Preparation;

        // 生成第一波敌方暂存
        m_enemyWave = m_aiController.GenerateEnemyWave(1);

        // 初始刷新商店 (shopLevel = 1)
        m_shopSystem.Refresh(1);

        // 给玩家一些初始单位方便测试
        auto soldier = std::make_shared<Unit>("步兵", Owner::PlayerCtrl,
            300, 25, 1, MAX_MANA,
            std::vector<Trait>{"人类", "战士"}, StarLevel::One);
        m_bench.AddUnit(soldier);
        m_playerUnits.push_back(soldier);

        auto archer = std::make_shared<Unit>("弓箭手", Owner::PlayerCtrl,
            200, 30, 3, MAX_MANA,
            std::vector<Trait>{"精灵", "远程"}, StarLevel::One);
        m_bench.AddUnit(archer);
        m_playerUnits.push_back(archer);

        auto mage = std::make_shared<Unit>("法师", Owner::PlayerCtrl,
            250, 40, 2, MAX_MANA,
            std::vector<Trait>{"人类", "法师"}, StarLevel::One);
        m_bench.AddUnit(mage);
        m_playerUnits.push_back(mage);

        UpdateSynergies();
    }

    GamePhase GameManager::GetCurrentPhase() const { return m_phase; }

    void GameManager::StartPreparationPhase()
    {
        // 恢复棋盘上己方单位状态
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto unit = m_board.GetOccupant(Position(r, c));
                if (unit && unit->GetOwner() == Owner::PlayerCtrl)
                {
                    unit->ResetHp();
                    unit->ResetMana();
                    unit->SetState(UnitState::Idle);
                    unit->SetAttackCooldown(0);
                }
            }

        // 刷新商店
        int shopLevel = std::min(9, 1 + m_currentWave / 2);
        m_shopSystem.Refresh(shopLevel);

        // 重置免费刷新次数
        m_shopSystem.SetFreeRefreshes(FREE_REFRESH_PER_ROUND);

        // 尝试合星
        TryMergeUnits();

        // 更新羁绊
        UpdateSynergies();

        // 生成下一波敌人
        m_enemyWave = m_aiController.GenerateEnemyWave(m_currentWave);

        m_phase = GamePhase::Preparation;
    }

    void GameManager::StartCombatPhase()
    {
        // 收集棋盘上所有己方单位
        std::vector<std::shared_ptr<Unit>> playerCombatUnits;
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto u = m_board.GetOccupant(Position(r, c));
                if (u && u->GetOwner() == Owner::PlayerCtrl)
                    playerCombatUnits.push_back(u);
            }

        // 开始战斗
        m_combatSystem.StartCombat(playerCombatUnits, m_enemyWave);
        m_phase = GamePhase::Combat;
    }

    void GameManager::Update(float deltaTime)
    {
        (void)deltaTime; // 暂未使用 deltaTime，帧率由定时器保证

        if (m_phase == GamePhase::Combat)
        {
            m_combatSystem.Update();

            if (!m_combatSystem.IsCombatActive())
            {
                // 判断胜负
                bool playerWon = false;
                for (int r = 0; r < m_board.GetRows(); ++r)
                    for (int c = 0; c < m_board.GetCols(); ++c)
                    {
                        auto u = m_board.GetOccupant(Position(r, c));
                        if (u && u->GetOwner() == Owner::PlayerCtrl && u->IsAlive())
                            playerWon = true;
                    }
                OnCombatEnd(playerWon);
            }
        }
    }

    bool GameManager::MoveUnitOnBoard(int fromRow, int fromCol, int toRow, int toCol)
    {
        if (m_phase != GamePhase::Preparation)
            return false;

        // 目标就是起点 → 什么都不做
        if (fromRow == toRow && fromCol == toCol)
            return false;

        auto unit = m_board.GetOccupant(Position(fromRow, fromCol));
        if (!unit || unit->GetOwner() != Owner::PlayerCtrl)
            return false;

        // 目标格有己方单位 → 交换
        auto target = m_board.GetOccupant(Position(toRow, toCol));
        if (target)
        {
            if (target->GetOwner() != Owner::PlayerCtrl)
                return false;
            return SwapUnitsOnBoard(fromRow, fromCol, toRow, toCol);
        }

        // 目标格为空 → 移动
        Position from(fromRow, fromCol);
        Position to(toRow, toCol);
        if (!m_board.IsInBounds(to) || !m_board.IsPlayerHalf(to))
            return false;

        m_board.RemoveUnit(from);
        m_board.PlaceUnit(unit, to);
        unit->SetGridPosition(toRow, toCol);
        return true;
    }

    // ========== 布阵操作 ==========

    bool GameManager::DeployUnit(std::shared_ptr<Unit> unit, int row, int col)
    {
        if (m_phase != GamePhase::Preparation)
            return false;

        // 在备战区找到该单位
        int benchIndex = -1;
        for (size_t i = 0; i < m_bench.GetCapacity(); ++i)
        {
            if (m_bench.GetUnit(i) == unit)
            {
                benchIndex = static_cast<int>(i);
                break;
            }
        }

        if (benchIndex < 0)
            return false;

        Position pos(row, col);
        if (!m_board.IsInBounds(pos) || m_board.IsOccupied(pos) || !m_board.IsPlayerHalf(pos))
            return false;

        // 从备战区移除
        auto removed = m_bench.RemoveUnit(static_cast<size_t>(benchIndex));
        if (!removed)
            return false;

        // 放置到棋盘
        if (!m_board.PlaceUnit(removed, pos))
        {
            // 放回备战区
            m_bench.AddUnit(removed);
            return false;
        }

        removed->SetGridPosition(row, col);
        UpdateSynergies();
        TryMergeUnits();  // 部署后立即检测合星
        return true;
    }

    bool GameManager::RecallUnit(int row, int col)
    {
        if (m_phase != GamePhase::Preparation)
            return false;

        auto unit = m_board.GetOccupant(Position(row, col));
        if (!unit || unit->GetOwner() != Owner::PlayerCtrl)
            return false;

        if (m_bench.IsFull())
            return false;

        m_board.RemoveUnit(Position(row, col));
        m_bench.AddUnit(unit);
        unit->SetGridPosition(-1, -1);
        UpdateSynergies();
        TryMergeUnits();  // 撤回备战区后检测合星
        return true;
    }

    bool GameManager::SwapUnitsOnBoard(int r1, int c1, int r2, int c2)
    {
        if (m_phase != GamePhase::Preparation)
            return false;

        auto u1 = m_board.GetOccupant(Position(r1, c1));
        auto u2 = m_board.GetOccupant(Position(r2, c2));

        if (!u1 || !u2)
            return false;
        if (u1->GetOwner() != Owner::PlayerCtrl || u2->GetOwner() != Owner::PlayerCtrl)
            return false;

        // 执行交换
        m_board.RemoveUnit(Position(r1, c1));
        m_board.RemoveUnit(Position(r2, c2));
        m_board.PlaceUnit(u1, Position(r2, c2));
        m_board.PlaceUnit(u2, Position(r1, c1));

        u1->SetGridPosition(r2, c2);
        u2->SetGridPosition(r1, c1);
        return true;
    }

    // ========== 商店操作 ==========

    ShopSystem& GameManager::GetShop() { return m_shopSystem; }

    void GameManager::BuyUnitFromShop(int index)
    {
        if (m_phase != GamePhase::Preparation)
            return;

        // 先从商店预览读取信息（不修改商店状态）
        const auto& shopUnits = m_shopSystem.GetShopUnits();
        if (index < 0 || index >= static_cast<int>(shopUnits.size()) || !shopUnits[index])
            return;

        const std::string& name = shopUnits[index]->GetName();
        int cost = 1;
        if (name == "法师" || name == "骑士" || name == "治疗师")
            cost = 2;
        else if (name == "刺客" || name == "狂战士" || name == "狙击手")
            cost = 3;

        // 先检查金币（修改商店前）
        if (m_gold < cost)
            return;

        // 先检查备战区空位，满了则尝试合星腾空间
        if (m_bench.IsFull())
        {
            TryMergeUnits();
            if (m_bench.IsFull())
                return;  // 合星后仍然满 → 放弃购买
        }

        // 全部检查通过，从商店取走单位
        auto unit = m_shopSystem.BuyUnit(index);
        if (!unit)
            return;

        m_bench.AddUnit(unit);
        m_gold -= cost;
        m_playerUnits.push_back(unit);
        UpdateSynergies();
        TryMergeUnits();
    }

    static int GetUnitCostForSell(const std::string& name)
    {
        if (name == "步兵" || name == "弓箭手" || name == "侍从") return 1;
        if (name == "法师" || name == "骑士" || name == "治疗师") return 2;
        if (name == "刺客" || name == "狂战士" || name == "狙击手") return 3;
        return 1;
    }

    void GameManager::SellUnitFromBench(int slotIndex)
    {
        if (m_phase != GamePhase::Preparation)
            return;

        auto unit = m_bench.GetUnit(static_cast<size_t>(slotIndex));
        if (!unit)
            return;

        // 计算出售价格：费用 × 星级倍率（1★=1, 2★=3, 3★=9）
        int cost = GetUnitCostForSell(unit->GetName());
        int starMul = 1;
        switch (unit->GetStarLevel())
        {
        case StarLevel::Two:   starMul = 3;  break;
        case StarLevel::Three: starMul = 9;  break;
        default:               starMul = 1;  break;
        }
        int sellPrice = cost * starMul;

        // 从备战区移除
        m_bench.RemoveUnit(static_cast<size_t>(slotIndex));

        // 从玩家单位列表中移除
        auto it = std::find(m_playerUnits.begin(), m_playerUnits.end(), unit);
        if (it != m_playerUnits.end())
            m_playerUnits.erase(it);

        // 加金币
        m_gold += sellPrice;

        UpdateSynergies();
    }

    // ========== 羁绊系统 ==========

    const std::vector<SynergyInfo>& GameManager::GetActiveSynergies() const
    {
        return m_synergySystem.GetActiveSynergies();
    }

    void GameManager::UpdateSynergies()
    {
        m_synergySystem.Update(m_playerUnits);
    }

    // ========== 合星 ==========

    bool GameManager::HasMergeableUnits() const
    {
        // 收集棋盘+备战区所有玩家单位
        std::vector<std::shared_ptr<Unit>> allUnits;
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto u = m_board.GetOccupant(Position(r, c));
                if (u && u->GetOwner() == Owner::PlayerCtrl)
                    allUnits.push_back(u);
            }
        for (size_t i = 0; i < m_bench.GetCapacity(); ++i)
        {
            auto u = m_bench.GetUnit(i);
            if (u) allUnits.push_back(u);
        }
        return m_mergeSystem.HasMergeable(allUnits);
    }

    bool GameManager::TryMergeUnits()
    {
        // 第1步：收集棋盘+备战区所有玩家单位，并保存棋盘位置
        std::vector<std::shared_ptr<Unit>> allUnits;
        std::vector<Position> savedBoardPositions;

        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto u = m_board.GetOccupant(Position(r, c));
                if (u && u->GetOwner() == Owner::PlayerCtrl)
                {
                    allUnits.push_back(u);
                    savedBoardPositions.emplace_back(r, c);
                }
            }

        for (size_t i = 0; i < m_bench.GetCapacity(); ++i)
        {
            auto u = m_bench.GetUnit(i);
            if (u)
                allUnits.push_back(u);
        }

        // 没有可合的 → 直接返回
        if (!m_mergeSystem.HasMergeable(allUnits))
            return false;

        bool anyMerged = false;
        while (m_mergeSystem.TryMerge(allUnits)) { anyMerged = true; }

        // 清空棋盘和备战区
        m_board.Clear();
        m_bench = Bench(BENCH_CAPACITY);
        m_playerUnits.clear();

        // 标记已使用的位置
        std::vector<bool> posUsed(savedBoardPositions.size(), false);

        // 重新放置
        for (auto& u : allUnits)
        {
            m_playerUnits.push_back(u);
            int gr = u->GetGridRow();
            int gc = u->GetGridCol();
            bool placed = false;

            // 优先放回原位
            if (gr >= 0 && gc >= 0)
            {
                Position p(gr, gc);
                if (m_board.IsInBounds(p) && !m_board.IsOccupied(p))
                {
                    m_board.PlaceUnit(u, p);
                    placed = true;
                }
            }

            // 原位不可用 → 用保存的棋盘空位（合星产生的新单位会落到这里）
            if (!placed)
            {
                for (size_t i = 0; i < savedBoardPositions.size(); ++i)
                {
                    if (posUsed[i]) continue;
                    Position p = savedBoardPositions[i];
                    if (!m_board.IsOccupied(p))
                    {
                        m_board.PlaceUnit(u, p);
                        u->SetGridPosition(p.x, p.y);
                        posUsed[i] = true;
                        placed = true;
                        break;
                    }
                }
            }

            if (!placed)
            {
                m_bench.AddUnit(u);
                u->SetGridPosition(-1, -1);
            }
        }

        UpdateSynergies();
        return true;
    }

    // ========== 装备 ==========

    EquipManager& GameManager::GetEquipManager() { return m_equipManager; }

    // ========== 访问器 ==========

    Board& GameManager::GetBoard() { return m_board; }
    Bench& GameManager::GetBench() { return m_bench; }
    int GameManager::GetCurrentWave() const { return m_currentWave; }
    int GameManager::GetGold() const { return m_gold; }
    void GameManager::SpendGold(int amount) { if (m_gold >= amount) m_gold -= amount; }
    void GameManager::AddGold(int amount) { m_gold += amount; }
    int GameManager::GetPlayerHp() const { return m_playerHp; }

    const std::vector<std::shared_ptr<Unit>>& GameManager::GetAllPlayerUnits() const
    {
        return m_playerUnits;
    }

    // ========== 内部函数 ==========

    void GameManager::OnCombatEnd(bool playerWon)
    {
        m_lastCombatResult = playerWon;

        if (playerWon)
        {
            // 胜利奖励（逐波递增）
            {
                int wave = m_currentWave;
                int reward = 5 + wave;
                if (wave > 3) reward += 3;
                if (wave > 6) reward += 4;
                m_gold += reward;
            }

            // 掉落装备检查
            auto drops = m_equipManager.GenerateDrops(m_currentWave);
            // 装备暂存到 inventory（预留接口）
            for (auto& equip : drops)
            {
                m_equipmentInventory.push_back(equip);
            }
        }
        else
        {
            // 失败惩罚：前4局扣10，5-7局扣15，之后扣20
            int damage = 10 + m_currentWave;
            m_playerHp -= damage;
            if (m_playerHp <= 0)
                m_playerHp = 0;

            // 失败补偿 5 金币，保证后续还能玩
            m_gold += 5;
        }

        // 清空棋盘上的死亡单位
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto unit = m_board.GetOccupant(Position(r, c));
                if (unit && !unit->IsAlive())
                    m_board.RemoveUnit(Position(r, c));
            }

        // 战后重置己方单位到玩家半场首行，按血量排序
        ResetPlayerUnitsAfterCombat();

        NextWave();
    }

    void GameManager::ResetPlayerUnitsAfterCombat()
    {
        // 收集所有存活的己方单位
        std::vector<std::shared_ptr<Unit>> alivePlayerUnits;
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto u = m_board.GetOccupant(Position(r, c));
                if (u && u->GetOwner() == Owner::PlayerCtrl && u->IsAlive())
                    alivePlayerUnits.push_back(u);
            }

        if (alivePlayerUnits.empty())
            return;

        // 按当前 HP 从高到低排序（坦克在前）
        std::sort(alivePlayerUnits.begin(), alivePlayerUnits.end(),
            [](const std::shared_ptr<Unit>& a, const std::shared_ptr<Unit>& b) {
                return a->GetHp() > b->GetHp();
            });

        // 从棋盘上移除己方单位
        for (auto& u : alivePlayerUnits)
        {
            Position p(u->GetGridRow(), u->GetGridCol());
            m_board.RemoveUnit(p);
        }

        // 在我方半场首行（row = rows/2）依次放置
        int startRow = m_board.GetRows() / 2;
        int col = 0;
        for (auto& u : alivePlayerUnits)
        {
            if (col >= m_board.GetCols())
            {
                // 超过一行放不下，全部放第一列
                m_bench.AddUnit(u);
                u->SetGridPosition(-1, -1);
                continue;
            }
            Position p(startRow, col);
            if (!m_board.IsOccupied(p))
            {
                m_board.PlaceUnit(u, p);
                u->SetGridPosition(startRow, col);
            }
            else
            {
                // 被占（不应发生）→ 放备战区
                m_bench.AddUnit(u);
                u->SetGridPosition(-1, -1);
            }
            col++;
        }
    }

    void GameManager::NextWave()
    {
        m_currentWave++;

        if (m_currentWave > 10)
        {
            // 通关 — 保持当前状态，UI 可显示胜利信息
            m_phase = GamePhase::Preparation;
            return;
        }

        StartPreparationPhase();
    }
}
