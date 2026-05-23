#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Unit.h"
#include "Types.h"

namespace synera
{
    // 装备属性加成结构。
    struct EquipStats
    {
        int bonusHp = 0;
        int bonusAtk = 0;
        int bonusDefense = 0;       // 减伤
        float dodgeChance = 0.0f;   // 闪避概率（0.0 ~ 1.0）
    };

    // 装备物品。
    struct Equipment
    {
        std::string name;
        EquipRarity rarity;
        EquipStats stats;
        bool isCombined = false;    // 是否为合成装备
    };

    // 装备系统：管理装备掉落、合成和佩戴。
    // 阶段三实现。
    class EquipManager
    {
    public:
        EquipManager() = default;

        // ---- 生成装备掉落 ----
        // 根据当前波次决定是否掉落装备。
        // 每 4-6 回合掉落一次（随机间隔）。
        // #TODO: 实现掉落逻辑
        std::vector<Equipment> GenerateDrops(int waveNumber);

        // ---- 装备合成 ----
        // 2 件低级装备合成 1 件高级装备。
        // #TODO: 实现合成公式
        Equipment CombineEquipment(const Equipment& a, const Equipment& b);

        // ---- 佩戴装备 ----
        // 将装备附加到单位上，修改单位属性。
        // #TODO: 实现装备效果应用
        void EquipUnit(std::shared_ptr<Unit> unit, const Equipment& equip);

    private:
        int m_lastDropWave = 0;     // 上次掉落波次
        int m_nextDropWave = 4;     // 下次掉落波次（初始第 4 波）

        // 根据概率生成随机稀有度的装备。
        // #TODO: 实现随机生成
        Equipment GenerateRandomEquipment(EquipRarity rarity) const;
    };
}
