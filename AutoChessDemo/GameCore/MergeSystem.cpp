#include "MergeSystem.h"
#include "GameConfig.h"
#include <unordered_map>
#include <algorithm>

namespace synera
{
    bool MergeSystem::TryMerge(std::vector<std::shared_ptr<Unit>>& units)
    {
        // 反复寻找可合星组合，直到无法再合
        bool anyMerged = false;
        while (true)
        {
            auto groups = FindMergeGroups(units);
            if (groups.empty())
                break;

            // 每组固定 3 个成员（3 同名同星 或 2 同名同星 + 1 侍从）
            auto& g = groups.front();
            auto u1 = g.members[0];
            auto u2 = g.members[1];
            auto u3 = g.members[2];

            auto newUnit = CreateMergedUnit({ u1, u2, u3 });
            if (!newUnit)
                break;

            auto removeUnit = [&](std::shared_ptr<Unit> u) {
                auto it = std::find(units.begin(), units.end(), u);
                if (it != units.end()) units.erase(it);
            };
            removeUnit(u1);
            removeUnit(u2);
            removeUnit(u3);

            units.push_back(newUnit);
            anyMerged = true;
        }
        return anyMerged;
    }

    bool MergeSystem::HasMergeable(const std::vector<std::shared_ptr<Unit>>& units) const
    {
        return !FindMergeGroups(units).empty();
    }

    std::vector<MergeSystem::MergeGroup> MergeSystem::FindMergeGroups(const std::vector<std::shared_ptr<Unit>>& units) const
    {
        // 第 1 步：将侍从与其他单位分开
        std::vector<std::shared_ptr<Unit>> squires;
        std::unordered_map<std::string, std::vector<std::shared_ptr<Unit>>> groups;

        for (auto& unit : units)
        {
            if (!unit || !unit->IsAlive())
                continue;
            if (unit->GetName() == SQUIRE_NAME)
                squires.push_back(unit);
            else
            {
                std::string key = unit->GetName() + "_"
                    + std::to_string(static_cast<int>(unit->GetStarLevel()));
                groups[key].push_back(unit);
            }
        }

        // 统计各星级侍从数量
        int squireAtStar[4] = { 0, 0, 0, 0 };
        for (auto& s : squires)
            squireAtStar[static_cast<int>(s->GetStarLevel())]++;

        std::vector<MergeGroup> result;

        // 第 2 步：处理非侍从单位的合星
        for (auto& pair : groups)
        {
            auto& members = pair.second;
            int starInt = static_cast<int>(members[0]->GetStarLevel());

            // 2a. 3+ 同名同星 → 标准合星（优先，不消耗侍从）
            while (members.size() >= 3)
            {
                MergeGroup mg;
                mg.name = members[0]->GetName();
                mg.star = members[0]->GetStarLevel();
                mg.members = { members[0], members[1], members[2] };
                members.erase(members.begin(), members.begin() + 3);
                result.push_back(std::move(mg));
            }

            // 2b. 2 同名同星 + 侍从 → 催化合星（消耗侍从）
            while (members.size() >= 2 && squireAtStar[starInt] > 0)
            {
                auto squireIt = std::find_if(squires.begin(), squires.end(),
                    [starInt](const auto& s) {
                        return s && static_cast<int>(s->GetStarLevel()) == starInt;
                    });
                if (squireIt == squires.end())
                    break;

                MergeGroup mg;
                mg.name = members[0]->GetName();
                mg.star = members[0]->GetStarLevel();
                mg.members = { members[0], members[1], *squireIt };
                members.erase(members.begin(), members.begin() + 2);
                squires.erase(squireIt);
                squireAtStar[starInt]--;
                result.push_back(std::move(mg));
            }
        }

        // 第 3 步：处理剩余侍从的自我合星（3 侍从 → 2★ 侍从）
        std::unordered_map<std::string, std::vector<std::shared_ptr<Unit>>> squireGroups;
        for (auto& s : squires)
        {
            std::string key = std::string(SQUIRE_NAME) + "_"
                + std::to_string(static_cast<int>(s->GetStarLevel()));
            squireGroups[key].push_back(s);
        }
        for (auto& pair : squireGroups)
        {
            auto& members = pair.second;
            while (members.size() >= 3)
            {
                MergeGroup mg;
                mg.name = SQUIRE_NAME;
                mg.star = members[0]->GetStarLevel();
                mg.members = { members[0], members[1], members[2] };
                members.erase(members.begin(), members.begin() + 3);
                result.push_back(std::move(mg));
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
