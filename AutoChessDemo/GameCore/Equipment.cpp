#include "Equipment.h"
#include "GameConfig.h"

#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace synera
{
    std::vector<Equipment> EquipManager::GenerateDrops(int waveNumber)
    {
        if (waveNumber < m_nextDropWave)
            return {};

        std::vector<Equipment> drops;
        int count = (std::rand() % 2) + 1; // 1 或 2 件

        // 根据波次决定稀有度
        EquipRarity rarity = EquipRarity::Common;
        if (waveNumber >= 7)
            rarity = EquipRarity::Epic;
        else if (waveNumber >= 4)
            rarity = EquipRarity::Rare;

        for (int i = 0; i < count; ++i)
            drops.push_back(GenerateRandomEquipment(rarity));

        // 更新掉落追踪
        m_lastDropWave = waveNumber;
        int interval = EQUIP_DROP_INTERVAL_MIN +
            (std::rand() % (EQUIP_DROP_INTERVAL_MAX - EQUIP_DROP_INTERVAL_MIN + 1));
        m_nextDropWave = waveNumber + interval;

        return drops;
    }

    Equipment EquipManager::CombineEquipment(const Equipment& a, const Equipment& b)
    {
        Equipment result;
        result.name = a.name + "+" + b.name;
        result.isCombined = true;

        // 属性合并（相加后 ×1.2 合成奖励）
        result.stats.bonusAtk = static_cast<int>((a.stats.bonusAtk + b.stats.bonusAtk) * 1.2);
        result.stats.bonusHp = static_cast<int>((a.stats.bonusHp + b.stats.bonusHp) * 1.2);
        result.stats.bonusDefense = static_cast<int>((a.stats.bonusDefense + b.stats.bonusDefense) * 1.2);
        result.stats.dodgeChance = std::min(1.0f, (a.stats.dodgeChance + b.stats.dodgeChance) * 1.2f);

        // 稀有度提升
        int aRarity = static_cast<int>(a.rarity);
        int bRarity = static_cast<int>(b.rarity);
        int newRarity = std::max(aRarity, bRarity);
        if (newRarity < static_cast<int>(EquipRarity::Epic))
            newRarity++;
        result.rarity = static_cast<EquipRarity>(newRarity);

        return result;
    }

    void EquipManager::EquipUnit(std::shared_ptr<Unit> unit, const Equipment& equip)
    {
        if (!unit)
            return;

        // 暂时直接修改单位属性（后续可改为装备槽系统）
        // 注意：装备效果不会在升星时丢失，因为升星从当前值乘算
        // 但这里直接修改基础属性，简化处理
        // 方案：直接修改 m_atk 和 m_maxHp 等（通过 Unit 没有直接 setter，
        // 所以这里不修改了，等待 Unit 装备槽系统实现）
        // 目前仅作为预留接口
        (void)unit;
        (void)equip;
    }

    Equipment EquipManager::GenerateRandomEquipment(EquipRarity rarity) const
    {
        Equipment equip;
        equip.rarity = rarity;

        int type = std::rand() % 4; // 0=ATK, 1=HP, 2=DEF, 3=Dodge

        switch (rarity)
        {
        case EquipRarity::Common:
            equip.name = (type == 0) ? "铁剑" : (type == 1) ? "生命宝石" :
                         (type == 2) ? "护甲片" : "轻灵靴";
            equip.stats.bonusAtk = (type == 0) ? (10 + std::rand() % 11) : 0;
            equip.stats.bonusHp = (type == 1) ? (100 + std::rand() % 101) : 0;
            equip.stats.bonusDefense = (type == 2) ? (5 + std::rand() % 6) : 0;
            equip.stats.dodgeChance = (type == 3) ? 0.05f : 0.0f;
            break;

        case EquipRarity::Rare:
            equip.name = (type == 0) ? "精铁剑" : (type == 1) ? "生命之石" :
                         (type == 2) ? "锁子甲" : "疾风靴";
            equip.stats.bonusAtk = (type == 0) ? (20 + std::rand() % 21) : (std::rand() % 11);
            equip.stats.bonusHp = (type == 1) ? (200 + std::rand() % 201) : (std::rand() % 101);
            equip.stats.bonusDefense = (type == 2) ? (10 + std::rand() % 11) : (std::rand() % 5);
            equip.stats.dodgeChance = (type == 3) ? 0.10f : 0.02f;
            break;

        case EquipRarity::Epic:
            equip.name = (type == 0) ? "传说之剑" : (type == 1) ? "泰坦之石" :
                         (type == 2) ? "龙鳞甲" : "影舞靴";
            equip.stats.bonusAtk = (type == 0) ? (40 + std::rand() % 41) : (10 + std::rand() % 21);
            equip.stats.bonusHp = (type == 1) ? (400 + std::rand() % 401) : (100 + std::rand() % 201);
            equip.stats.bonusDefense = (type == 2) ? (20 + std::rand() % 11) : (5 + std::rand() % 11);
            equip.stats.dodgeChance = (type == 3) ? 0.20f : 0.05f;
            break;
        }

        return equip;
    }
}
