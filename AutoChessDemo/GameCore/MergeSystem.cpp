#include "MergeSystem.h"
#include "GameConfig.h"
#include <unordered_map>
#include <algorithm>

namespace synera
{
    bool MergeSystem::TryMerge(std::vector<std::shared_ptr<Unit>>& units)
    {
        auto groups = FindMergeGroups(units);
        bool merged = false;

        for (auto& group : groups)
        {
            while (group.members.size() >= MERGE_COUNT)
            {
                // 取前 3 个
                auto u1 = group.members[0];
                auto u2 = group.members[1];
                auto u3 = group.members[2];

                // 创建高星单位
                auto newUnit = CreateMergedUnit({u1, u2, u3});
                if (!newUnit)
                    break;

                // 从 units 中删除这 3 个
                auto it1 = std::find(units.begin(), units.end(), u1);
                if (it1 != units.end()) units.erase(it1);
                auto it2 = std::find(units.begin(), units.end(), u2);
                if (it2 != units.end()) units.erase(it2);
                auto it3 = std::find(units.begin(), units.end(), u3);
                if (it3 != units.end()) units.erase(it3);

                // 从 group.members 中删除前 3 个
                group.members.erase(group.members.begin(), group.members.begin() + 3);

                // 把新单位加入 units
                units.push_back(newUnit);

                merged = true;
            }
        }

        return merged;
    }

    bool MergeSystem::HasMergeable(const std::vector<std::shared_ptr<Unit>>& units) const
    {
        auto groups = FindMergeGroups(units);
        for (const auto& g : groups)
        {
            if (g.members.size() >= MERGE_COUNT)
                return true;
        }
        return false;
    }

    std::vector<MergeSystem::MergeGroup> MergeSystem::FindMergeGroups(const std::vector<std::shared_ptr<Unit>>& units) const
    {
        std::unordered_map<std::string, std::vector<std::shared_ptr<Unit>>> groups;

        for (auto& unit : units)
        {
            if (!unit || !unit->IsAlive())
                continue;
            std::string key = unit->GetName() + "_" + std::to_string(static_cast<int>(unit->GetStarLevel()));
            groups[key].push_back(unit);
        }

        std::vector<MergeGroup> result;
        for (auto& pair : groups)
        {
            if (pair.second.size() >= MERGE_COUNT)
            {
                MergeGroup mg;
                mg.name = pair.second[0]->GetName();
                mg.star = pair.second[0]->GetStarLevel();
                mg.members = std::move(pair.second);
                result.push_back(mg);
            }
        }
        return result;
    }

    std::shared_ptr<Unit> MergeSystem::CreateMergedUnit(const std::vector<std::shared_ptr<Unit>>& sameUnits) const
    {
        if (sameUnits.size() < 3 || !sameUnits[0])
            return nullptr;

        auto& tpl = sameUnits[0];

        // 计算新星级
        StarLevel newStar;
        switch (tpl->GetStarLevel())
        {
        case StarLevel::One:
            newStar = StarLevel::Two;
            break;
        case StarLevel::Two:
            newStar = StarLevel::Three;
            break;
        case StarLevel::Three:
        default:
            return nullptr; // 3★ 不能再升
        }

        // 创建新单位（使用基础属性避免重复加成）
        auto newUnit = std::make_shared<Unit>(
            tpl->GetName(),
            tpl->GetOwner(),
            tpl->GetBaseHp(),   // 使用基础 HP，不含星级加成
            tpl->GetBaseAtk(),  // 使用基础 ATK，不含星级加成
            tpl->GetRange(),
            tpl->GetMaxMana(),
            tpl->GetTraits(),
            StarLevel::One
        );

        newUnit->SetStarLevel(newStar);
        return newUnit;
    }
}
