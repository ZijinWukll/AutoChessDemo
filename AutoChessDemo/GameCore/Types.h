#pragma once

#include <string>
#include <vector>

namespace synera
{
    // 控制归属：用于区分我方单位与敌方单位。
    enum class Owner
    {
        PlayerCtrl,
        EnemyCtrl
    };

    // 战斗阶段单位状态机。
    enum class UnitState
    {
        Idle,       // 空闲
        Moving,     // 移动中
        Attacking,  // 攻击中
        Casting,    // 施法中
        Dead        // 死亡
    };

    // 阶段二：攻击类型。
    enum class AttackType
    {
        Melee,      // 近战
        Ranged      // 远程
    };

    // 阶段二：伤害类型（用于技能/AOE）。
    enum class DamageType
    {
        Physical,   // 物理伤害
        Magic       // 魔法伤害
    };

    // 阶段二：技能目标选择类型。
    enum class SkillTarget
    {
        NearestEnemy,   // 最近敌人
        LowestHpEnemy,  // 血量最低敌人
        AOE,            // 范围伤害
        SelfBuff        // 自身增益
    };

    // 阶段三：装备稀有度。
    enum class EquipRarity
    {
        Common,     // 普通
        Rare,       // 稀有
        Epic        // 史诗
    };

    // 阶段三：合星星级。
    enum class StarLevel
    {
        One = 1,
        Two = 2,
        Three = 3
    };

    // 阶段三：游戏阶段。
    enum class GamePhase
    {
        Preparation,    // 准备阶段（布阵、购物）
        Combat          // 战斗阶段（自动进行）
    };

    // 羁绊标签类型。
    using Trait = std::string;
}
