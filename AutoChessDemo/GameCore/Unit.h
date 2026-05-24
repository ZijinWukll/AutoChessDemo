#pragma once

#include <string>
#include <vector>
#include "Types.h"
#include "GameConfig.h"

namespace synera
{
    // 单位类：我方与敌方单位统一使用该结构。
    // 覆盖阶段一（基础属性）、阶段二（战斗属性）、阶段三（星级）。
    class Unit
    {
    public:
        // ---- 构造函数 ----
        Unit(std::string name, Owner owner,
             int maxHp, int atk, int range,
             int maxMana = synera::MAX_MANA,
             std::vector<Trait> traits = {},
             StarLevel star = StarLevel::One);

        // ---- 阶段一：基础属性 Getter ----
        const std::string& GetName() const;
        Owner GetOwner() const;

        int GetHp() const;
        int GetMaxHp() const;
        int GetAtk() const;
        int GetRange() const;
        bool IsAlive() const;

        const std::vector<Trait>& GetTraits() const;
        bool HasTrait(const Trait& trait) const;
        StarLevel GetStarLevel() const;

        // ---- 合星用：获取基础属性（不含星级加成） ----
        int GetBaseHp() const { return m_baseHp; }
        int GetBaseAtk() const { return m_baseAtk; }

        // ---- 阶段一：生命周期 ----
        void TakeDamage(int value);
        void ResetHp();

        // ---- 阶段二：法力/技能 ----
        int GetMana() const;
        int GetMaxMana() const;
        void AddMana(int value);                // 普攻/受击增加法力
        bool IsManaFull() const;                // 判断可否施法
        void ResetMana();                       // 施法后清空法力

        // ---- 阶段二：战斗状态 ----
        UnitState GetState() const;
        void SetState(UnitState state);

        int GetAttackCooldown() const;
        void SetAttackCooldown(int cd);
        bool IsAttackReady() const;             // 攻击冷却是否就绪
        void TickAttackCooldown();              // 每帧减少冷却

        // ---- 阶段二：攻击/施法 ----
        // #TODO: 计算实际造成的伤害（可考虑护甲/魔抗/羁绊加成）
        int CalculateDamage(DamageType type = DamageType::Physical) const;

        // #TODO: 执行技能效果，返回（是否成功施法）
        //  参数 target 为目标单位指针（AOE 时为主目标）
        //  需要在 CombatSystem 中实现具体技能逻辑
        bool CastSkill(Unit* target);

        // ---- 阶段三：合星 ----
        void SetStarLevel(StarLevel star);
        void ApplyStarBonus();
        // ---- 阶段三：羁绊加成（临时，战斗前应用，战后通过 ApplyStarBonus 清除） ----
        void ApplySynergyBuff(float atkMul, float hpMul, int rangeBonus);
        void SetHpDirectly(int hp); // 仅供羁绊系统临时设置血量

        // ---- 阶段二：位置追踪 ----
        // 单位在棋盘上的位置（由 Board 管理，单位仅记录副本用于寻路/渲染）
        void SetGridPosition(int row, int col);
        int GetGridRow() const;
        int GetGridCol() const;

    private:
        // ---- 阶段一：基础属性（不含星级加成） ----
        std::string m_name;
        Owner m_owner = Owner::PlayerCtrl;
        int m_baseHp = 1;
        int m_baseAtk = 20;

        // ---- 当前属性（含星级加成） ----
        int m_hp = 1;
        int m_maxHp = 1;
        int m_atk = 20;                     // 攻击力
        int m_range = 1;                    // 攻击范围（格数）
        std::vector<Trait> m_traits;
        StarLevel m_starLevel = StarLevel::One;

        // ---- 阶段二：战斗属性 ----
        int m_mana = 0;
        int m_maxMana = synera::MAX_MANA;
        UnitState m_state = UnitState::Idle;
        int m_attackCooldown = 0;           // 0 = 可攻击

        // ---- 位置记录 ----
        int m_gridRow = -1;
        int m_gridCol = -1;
    };
}
