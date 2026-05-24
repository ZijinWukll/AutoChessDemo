#include "Unit.h"
#include "GameConfig.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <cmath>
#include <cctype>
#include <locale>

namespace synera
{
    Unit::Unit(std::string name, Owner owner,
               int maxHp, int atk, int range,
               int maxMana, std::vector<Trait> traits,
               StarLevel star)
        : m_name(std::move(name))
        , m_owner(owner)
        , m_baseHp(std::max(1, maxHp))
        , m_baseAtk(std::max(0, atk))
        , m_hp(std::max(1, maxHp))
        , m_maxHp(std::max(1, maxHp))
        , m_atk(std::max(0, atk))
        , m_range(std::max(1, range))
        , m_traits(std::move(traits))
        , m_starLevel(star)
        , m_maxMana(std::max(1, maxMana))
    {
        ApplyStarBonus();
    }

    // ========== 阶段一：基础属性 Getter ==========

    const std::string& Unit::GetName() const { return m_name; }
    Owner Unit::GetOwner() const { return m_owner; }
    int Unit::GetHp() const { return m_hp; }
    int Unit::GetMaxHp() const { return m_maxHp; }
    int Unit::GetAtk() const { return m_atk; }
    int Unit::GetRange() const { return m_range; }
    bool Unit::IsAlive() const { return m_hp > 0; }
    const std::vector<Trait>& Unit::GetTraits() const { return m_traits; }
    bool Unit::HasTrait(const Trait& trait) const
    {
        return std::find(m_traits.begin(), m_traits.end(), trait) != m_traits.end();
    }
    StarLevel Unit::GetStarLevel() const { return m_starLevel; }

    void Unit::TakeDamage(int value)
    {
        if (value <= 0 || !IsAlive())
            return;
        m_hp = std::max(0, m_hp - value);
        AddMana(MANA_PER_HIT);
    }

    void Unit::ResetHp()
    {
        m_hp = m_maxHp;
    }

    // ========== 阶段二：法力/技能 ==========

    int Unit::GetMana() const { return m_mana; }
    int Unit::GetMaxMana() const { return m_maxMana; }

    void Unit::AddMana(int value)
    {
        if (value <= 0 || !IsAlive())
            return;
        m_mana = std::min(m_maxMana, m_mana + value);
    }

    bool Unit::IsManaFull() const
    {
        return m_mana >= m_maxMana;
    }

    void Unit::ResetMana()
    {
        m_mana = 0;
    }

    // ========== 阶段二：战斗状态 ==========

    UnitState Unit::GetState() const { return m_state; }
    void Unit::SetState(UnitState state) { m_state = state; }

    int Unit::GetAttackCooldown() const { return m_attackCooldown; }
    void Unit::SetAttackCooldown(int cd) { m_attackCooldown = std::max(0, cd); }
    bool Unit::IsAttackReady() const { return m_attackCooldown <= 0 && IsAlive(); }
    void Unit::TickAttackCooldown()
    {
        if (m_attackCooldown > 0)
            --m_attackCooldown;
    }

    // ========== 阶段二：伤害计算 ==========

    int Unit::CalculateDamage(DamageType type) const
    {
        (void)type;

        double baseDamage = static_cast<double>(m_atk);

        double starMul = 1.0;
        switch (m_starLevel) {
            case StarLevel::One:   starMul = 1.0; break;
            case StarLevel::Two:   starMul = 2.0; break;
            case StarLevel::Three: starMul = 3.0; break;
            default:               starMul = 1.0; break;
        }

        double dmg = baseDamage * starMul;

        constexpr double critRate = 0.1;
        constexpr double critMul = 2.0;

        thread_local static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < critRate)
            dmg *= critMul;

        int finalDmg = static_cast<int>(std::max(0.0, std::round(dmg)));
        return finalDmg;
    }

    // ========== 阶段二：技能施法 ==========

    bool Unit::CastSkill(Unit* target)
    {
        if (!IsAlive() || !IsManaFull())
            return false;
        if (target == nullptr)
            return false;

        SetState(UnitState::Casting);

        std::string lowerName = m_name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lowerName.find("mage") != std::string::npos || lowerName.find("法师") != std::string::npos)
        {
            int damage = static_cast<int>(m_atk * 2.0);
            target->TakeDamage(damage);
        }
        else if (lowerName.find("heal") != std::string::npos || lowerName.find("治疗") != std::string::npos)
        {
            int healAmount = static_cast<int>(m_maxHp * 0.3);
            m_hp = std::min(m_maxHp, m_hp + healAmount);
        }
        else
        {
            int damage = static_cast<int>(m_atk * 1.5);
            target->TakeDamage(damage);
        }

        ResetMana();
        SetState(UnitState::Idle);
        return true;
    }

    // ========== 阶段三：合星 ==========

    void Unit::SetStarLevel(StarLevel star)
    {
        m_starLevel = star;
        ApplyStarBonus();
    }

    void Unit::ApplyStarBonus()
    {
        double starMul = 1.0;
        switch (m_starLevel)
        {
        case StarLevel::One:   starMul = 1.0; break;
        case StarLevel::Two:   starMul = 2.0; break;
        case StarLevel::Three: starMul = 3.0; break;
        }

        m_atk = static_cast<int>(std::round(static_cast<double>(m_baseAtk) * starMul));
        m_maxHp = static_cast<int>(std::round(static_cast<double>(m_baseHp) * starMul));
        m_hp = m_maxHp;
    }

    void Unit::ApplySynergyBuff(float atkMul, float hpMul, int rangeBonus)
    {
        // 在现有属性上叠加羁绊加成（乘积计算，不重复计算星级）
        m_atk = static_cast<int>(std::round(static_cast<double>(m_atk) * atkMul));
        m_maxHp = static_cast<int>(std::round(static_cast<double>(m_maxHp) * hpMul));
        m_hp = m_maxHp;  // 羁绊加血满血
        m_range += rangeBonus;
    }

    void Unit::SetHpDirectly(int hp)
    {
        m_hp = std::max(0, std::min(m_maxHp, hp));
    }

    // ========== 位置 ==========

    void Unit::SetGridPosition(int row, int col) { m_gridRow = row; m_gridCol = col; }
    int Unit::GetGridRow() const { return m_gridRow; }
    int Unit::GetGridCol() const { return m_gridCol; }
}
