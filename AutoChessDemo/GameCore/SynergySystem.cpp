#include "SynergySystem.h"
#include "GameConfig.h"

namespace synera
{
    void SynergySystem::Update(const std::vector<std::shared_ptr<Unit>>& units)
    {
        m_traitCounts.clear();
        m_activeSynergies.clear();

        // 统计羁绊：按星级加权（1★=1, 2★=3, 3★=9）
        for (const auto& unit : units)
        {
            if (!unit)
                continue;

            int weight = 1;
            switch (unit->GetStarLevel())
            {
            case StarLevel::Two:   weight = 3; break;
            case StarLevel::Three: weight = 9; break;
            default:               weight = 1; break;
            }

            for (const auto& trait : unit->GetTraits())
            {
                m_traitCounts[trait] += weight;
            }
        }

        // 判断各羁绊激活的档位
        for (const auto& pair : m_traitCounts)
        {
            const auto& trait = pair.first;
            int count = pair.second;
            int threshold = GetActivatedThreshold(count);
            if (threshold > 0)
            {
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
        if (count >= SYNERGY_THRESHOLD_3)
            return SYNERGY_THRESHOLD_3;
        if (count >= SYNERGY_THRESHOLD_2)
            return SYNERGY_THRESHOLD_2;
        if (count >= SYNERGY_THRESHOLD_1)
            return SYNERGY_THRESHOLD_1;
        return 0;
    }
}
