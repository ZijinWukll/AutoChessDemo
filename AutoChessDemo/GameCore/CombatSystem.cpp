#include "CombatSystem.h"
#include "GameConfig.h"

#include <climits>

namespace synera
{
    CombatSystem::CombatSystem(Board& board, PathFinder& pathFinder)
        : m_board(board)
        , m_pathFinder(pathFinder)
    {
    }

    void CombatSystem::StartCombat(const std::vector<std::shared_ptr<Unit>>& playerUnits,
                                   const std::vector<std::shared_ptr<Unit>>& enemyUnits)
    {
        // 清空棋盘
        m_board.Clear();

        // 部署玩家单位到棋盘下半区
        for (auto& unit : playerUnits)
        {
            if (!unit || !unit->IsAlive())
                continue;
            int r = unit->GetGridRow();
            int c = unit->GetGridCol();
            Position pos(r, c);
            if (m_board.IsInBounds(pos) && m_board.IsPlayerHalf(pos) && !m_board.IsOccupied(pos))
            {
                m_board.PlaceUnit(unit, pos);
                unit->SetState(UnitState::Idle);
                unit->SetAttackCooldown(0.0f);
            }
            else
            {
                // 如果位置已被占用或无效，找玩家半场空位
                bool placed = false;
                for (int rr = m_board.GetRows() / 2; rr < m_board.GetRows() && !placed; ++rr)
                    for (int cc = 0; cc < m_board.GetCols() && !placed; ++cc)
                    {
                        Position p(rr, cc);
                        if (!m_board.IsOccupied(p))
                        {
                            m_board.PlaceUnit(unit, p);
                            unit->SetGridPosition(rr, cc);
                            unit->SetState(UnitState::Idle);
                            unit->SetAttackCooldown(0.0f);
                            placed = true;
                        }
                    }
            }
        }

        // 部署敌方单位到棋盘上半区
        DeployEnemiesToBoard(enemyUnits);

        m_active = true;
        m_combatTimer = 0.0f;
    }

    void CombatSystem::DeployEnemiesToBoard(const std::vector<std::shared_ptr<Unit>>& enemyUnits)
    {
        // 按近战/远程分开
        std::vector<std::shared_ptr<Unit>> melee, ranged;
        for (auto& u : enemyUnits)
        {
            if (!u || !u->IsAlive())
                continue;
            if (u->GetRange() <= 1)
                melee.push_back(u);
            else
                ranged.push_back(u);
        }

        // 远程放行 0-1，近战放行 2-3
        int rIdx = 0;
        for (auto& u : ranged)
        {
            bool placed = false;
            for (int rr = rIdx; rr <= 1 && !placed; ++rr)
                for (int cc = 0; cc < m_board.GetCols() && !placed; ++cc)
                {
                    Position p(rr, cc);
                    if (!m_board.IsOccupied(p))
                    {
                        m_board.PlaceUnit(u, p);
                        u->SetGridPosition(rr, cc);
                        u->SetState(UnitState::Idle);
                        u->SetAttackCooldown(0.0f);
                        placed = true;
                    }
                }
            if (placed) rIdx = u->GetGridRow();
        }

        for (auto& u : melee)
        {
            bool placed = false;
            for (int rr = 2; rr < m_board.GetRows() / 2 && !placed; ++rr)
                for (int cc = 0; cc < m_board.GetCols() && !placed; ++cc)
                {
                    Position p(rr, cc);
                    if (!m_board.IsOccupied(p))
                    {
                        m_board.PlaceUnit(u, p);
                        u->SetGridPosition(rr, cc);
                        u->SetState(UnitState::Idle);
                        u->SetAttackCooldown(0.0f);
                        placed = true;
                    }
                }
        }
    }

    void CombatSystem::Update(float deltaTime)
    {
        if (!m_active)
            return;

        m_combatTimer += deltaTime;
        if (m_combatTimer > MAX_COMBAT_TIME)
        {
            m_active = false;
            return;
        }

        auto aliveUnits = GetAllAliveUnits();
        for (auto& unit : aliveUnits)
        {
            ProcessUnit(unit, deltaTime);
        }

        // 检查战斗是否结束
        bool playerAlive = false, enemyAlive = false;
        for (auto& u : aliveUnits)
        {
            if (u->GetOwner() == Owner::PlayerCtrl)
                playerAlive = true;
            else
                enemyAlive = true;
        }
        if (!playerAlive || !enemyAlive)
        {
            m_active = false;
        }
    }

    bool CombatSystem::IsCombatActive() const
    {
        return m_active;
    }

    std::vector<std::shared_ptr<Unit>> CombatSystem::GetAllAliveUnits() const
    {
        std::vector<std::shared_ptr<Unit>> result;
        for (int r = 0; r < m_board.GetRows(); ++r)
            for (int c = 0; c < m_board.GetCols(); ++c)
            {
                auto unit = m_board.GetOccupant(Position(r, c));
                if (unit && unit->IsAlive())
                    result.push_back(unit);
            }
        return result;
    }

    std::vector<std::shared_ptr<Unit>> CombatSystem::GetEnemiesFor(const Unit& unit) const
    {
        auto all = GetAllAliveUnits();
        std::vector<std::shared_ptr<Unit>> enemies;
        for (auto& u : all)
        {
            if (u->GetOwner() != unit.GetOwner())
                enemies.push_back(u);
        }
        return enemies;
    }

    void CombatSystem::ProcessUnit(std::shared_ptr<Unit> unit, float dt)
    {
        if (!unit || !unit->IsAlive())
            return;

        switch (unit->GetState())
        {
        case UnitState::Dead:
            return;

        case UnitState::Idle:
        {
            auto enemies = GetEnemiesFor(*unit);
            if (enemies.empty())
                return;

            // 找最近的敌人
            auto nearest = FindNearestEnemy(*unit, enemies);
            if (!nearest)
                return;

            int dist = ManhattanDist(*unit, *nearest);

            if (dist <= unit->GetRange())
            {
                // 在攻击范围内
                if (unit->IsManaFull())
                {
                    unit->CastSkill(nearest.get());
                }
                else
                {
                    unit->SetState(UnitState::Attacking);
                    PerformAttack(unit, nearest);
                }
            }
            else
            {
                unit->SetState(UnitState::Moving);
            }
            break;
        }

        case UnitState::Moving:
        {
            // 移动冷却 — 控制走位速度，让玩家看清每一步移动
            unit->TickMoveCooldown(dt);
            if (!unit->IsMoveReady())
                break;
            unit->SetMoveCooldown(MOVE_COOLDOWN_TIME);
            MoveTowardEnemy(unit);
            break;
        }

        case UnitState::Attacking:
        {
            unit->TickAttackCooldown(dt);
            if (unit->IsAttackReady())
            {
                auto enemies = GetEnemiesFor(*unit);
                if (enemies.empty())
                    return;

                auto nearest = FindNearestEnemy(*unit, enemies);
                if (!nearest)
                    return;

                int dist = ManhattanDist(*unit, *nearest);
                if (dist <= unit->GetRange())
                {
                    if (unit->IsManaFull())
                    {
                        unit->CastSkill(nearest.get());
                    }
                    else
                    {
                        PerformAttack(unit, nearest);
                    }
                }
                else
                {
                    unit->SetState(UnitState::Moving);
                }
            }
            break;
        }

        case UnitState::Casting:
        {
            auto enemies = GetEnemiesFor(*unit);
            if (!enemies.empty())
            {
                auto nearest = FindNearestEnemy(*unit, enemies);
                if (nearest)
                    unit->CastSkill(nearest.get());
            }
            unit->SetState(UnitState::Idle);
            break;
        }
        }
    }

    void CombatSystem::PerformAttack(std::shared_ptr<Unit> unit, std::shared_ptr<Unit> target)
    {
        if (!unit->IsAlive() || !target->IsAlive())
            return;

        // 检查距离
        int dist = ManhattanDist(*unit, *target);
        if (dist > unit->GetRange())
        {
            unit->SetState(UnitState::Moving);
            return;
        }

        // 计算伤害并造成伤害
        int damage = unit->CalculateDamage(DamageType::Physical);
        target->TakeDamage(damage);

        // 记录攻击事件（供 UI 渲染攻击特效）
        m_attackEvents.push_back({
            unit->GetGridRow(), unit->GetGridCol(),
            target->GetGridRow(), target->GetGridCol(),
            damage,
            unit->GetOwner() == Owner::PlayerCtrl
        });

        // 攻击方回复法力
        unit->AddMana(MANA_PER_ATTACK);

        // 重置攻击冷却
        unit->SetAttackCooldown(ATTACK_COOLDOWN_TIME);

        // 如果目标死亡 → 立即从棋盘移除（否则尸体堵塞路径，导致其他单位卡死）
        if (!target->IsAlive())
        {
            target->SetState(UnitState::Dead);
            m_board.RemoveUnit(Position(target->GetGridRow(), target->GetGridCol()));
        }
    }

    void CombatSystem::MoveTowardEnemy(std::shared_ptr<Unit> unit)
    {
        auto enemies = GetEnemiesFor(*unit);
        if (enemies.empty())
        {
            unit->SetState(UnitState::Idle);
            return;
        }

        // 收集敌人位置
        std::vector<Position> enemyPositions;
        for (auto& e : enemies)
        {
            if (e->IsAlive())
                enemyPositions.emplace_back(e->GetGridRow(), e->GetGridCol());
        }

        Position from(unit->GetGridRow(), unit->GetGridCol());

        // 先用曼哈顿距离找最近的敌人
        auto nearestManhattan = FindNearestEnemy(*unit, enemies);
        if (!nearestManhattan)
        {
            unit->SetState(UnitState::Idle);
            return;
        }

        // 尝试寻路
        auto nearest = m_pathFinder.FindNearestReachable(from, enemyPositions);
        if (nearest.has_value())
        {
            auto path = m_pathFinder.FindPath(from, nearest.value());
            if (path.has_value() && !path->empty())
            {
                Position nextStep = path->front();
                if (!m_board.IsOccupied(nextStep))
                {
                    // 执行移动
                    m_board.MoveUnit(from, nextStep);
                    unit->SetGridPosition(nextStep.x, nextStep.y);

                    // 检查是否进入攻击范围
                    for (auto& e : enemies)
                    {
                        if (!e->IsAlive())
                            continue;
                        int dist = abs(nextStep.x - e->GetGridRow()) + abs(nextStep.y - e->GetGridCol());
                        if (dist <= unit->GetRange())
                        {
                            unit->SetState(UnitState::Attacking);
                            return;
                        }
                    }
                    return;
                }
            }
        }

        // 寻路失败 → 向最近的敌人曼哈顿方向逼近一格
        Position targetPos(nearestManhattan->GetGridRow(), nearestManhattan->GetGridCol());
        if (MoveOneStepToward(*unit, targetPos))
            return;

        // 完全无法移动 → 回到 Idle 让下一帧重新决策
        unit->SetState(UnitState::Idle);
    }

    bool CombatSystem::MoveOneStepToward(const Unit& unit, const Position& target)
    {
        Position from(unit.GetGridRow(), unit.GetGridCol());
        if (!m_board.IsInBounds(from)) return false;

        int dr = (target.x > from.x) ? 1 : (target.x < from.x) ? -1 : 0;
        int dc = (target.y > from.y) ? 1 : (target.y < from.y) ? -1 : 0;

        // 优先尝试主方向（先走后右/左）
        Position candidates[4];
        int nCand = 0;
        if (dr != 0) candidates[nCand++] = Position(from.x + dr, from.y);
        if (dc != 0) candidates[nCand++] = Position(from.x, from.y + dc);
        if (dr != 0 && dc != 0) candidates[nCand++] = Position(from.x + dr, from.y + dc); // 斜角

        for (int i = 0; i < nCand; ++i)
        {
            const Position& next = candidates[i];
            if (!m_board.IsInBounds(next)) continue;
            if (m_board.IsOccupied(next)) continue;

            // 可以移动
            m_board.MoveUnit(from, next);
            const_cast<Unit&>(unit).SetGridPosition(next.x, next.y);

            // 检查是否进入攻击范围 — 注意 const_cast 不算好但这里安全
            return true;
        }

        return false;
    }

    std::shared_ptr<Unit> CombatSystem::FindNearestEnemy(const Unit& unit, const std::vector<std::shared_ptr<Unit>>& enemies) const
    {
        std::shared_ptr<Unit> nearest;
        int minDist = INT_MAX;
        for (auto& e : enemies)
        {
            if (!e->IsAlive())
                continue;
            int dist = ManhattanDist(unit, *e);
            if (dist < minDist)
            {
                minDist = dist;
                nearest = e;
            }
        }
        return nearest;
    }

    int CombatSystem::ManhattanDist(const Unit& a, const Unit& b) const
    {
        return abs(a.GetGridRow() - b.GetGridRow()) + abs(a.GetGridCol() - b.GetGridCol());
    }
}
