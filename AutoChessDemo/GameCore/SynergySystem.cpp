#include "SynergySystem.h"
#include "GameConfig.h"

namespace synera
{
    void SynergySystem::Update(const std::vector<std::shared_ptr<Unit>>& units)
    {
        // #TODO: 统计羁绊（遍历所有单位，统计各羁绊数量，计算激活档位）
        // === 需要做什么 ===
        // 遍历场上（棋盘+备战区）所有单位，统计每种羁绊的持有数量，
        // 然后根据数量判断哪些羁绊"激活"了。
        //
        // === 实现流程 ===
        // 1. m_traitCounts.clear()
        // 2. m_activeSynergies.clear()
        // 3. for (auto& unit : units)
        //        for (auto& trait : unit->GetTraits())
        //            m_traitCounts[trait]++
        // 4. for (auto& [trait, count] : m_traitCounts)
        //        int threshold = GetActivatedThreshold(count);
        //        if (threshold > 0) {
        //            SynergyInfo info{trait, count, threshold};
        //            m_activeSynergies.push_back(info);
        //        }
        //
        // === 调用时机 ===
        // - 每次玩家部署/撤回单位后
        // - 每次合星后（星级不影响羁绊，但单位数量可能变化）
        // - 每次购买新单位后
        // 由 GameManager::UpdateSynergies() 触发。
		m_traitCounts.clear();
		m_activeSynergies.clear();

		for (const auto& unit : units)
        {
            if (!unit)
                continue;

			for (const auto& trait : unit->GetTraits())
            {
                m_traitCounts[trait]++;
            }
        }

        for (const auto& pair : m_traitCounts) {
            const auto& trait = pair.first;
            int count = pair.second;
            int threshold = GetActivatedThreshold(count);
            if (threshold > 0) {
                SynergyInfo info{trait, count, threshold};
                m_activeSynergies.push_back(info);
			}

        }




    }

    const std::vector<SynergyInfo>& SynergySystem::GetActiveSynergies() const
    {
        return m_activeSynergies;
    }

    SynergyInfo SynergySystem::GetSynergyInfo(const Trait& trait) const
    {
        auto it = m_traitCounts.find(trait);
        if (it == m_traitCounts.end())
            return {trait, 0, 0};

        return {trait, it->second, GetActivatedThreshold(it->second)};
    }

    bool SynergySystem::IsSynergyActive(const Trait& trait, int threshold) const
    {
        auto it = m_traitCounts.find(trait);
        if (it == m_traitCounts.end())
            return false;
        return GetActivatedThreshold(it->second) >= threshold;
    }

    void SynergySystem::Clear()
    {
        m_traitCounts.clear();
        m_activeSynergies.clear();
    }

    int SynergySystem::GetActivatedThreshold(int count)
    {
        // 档位设计：
        //   count >= 4 → 激活第二档（阈值 4）
        //   count >= 2 → 激活第一档（阈值 2）
        //   否则 → 未激活（0）
        if (count >= SYNERGY_THRESHOLD_2)
            return SYNERGY_THRESHOLD_2;
        if (count >= SYNERGY_THRESHOLD_1)
            return SYNERGY_THRESHOLD_1;
        return 0;
    }
}
