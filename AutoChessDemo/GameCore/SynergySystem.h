#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Unit.h"

namespace synera
{
    // 羁绊系统状态：某个羁绊的激活信息。
    struct SynergyInfo
    {
        Trait trait;            // 羁绊名称
        int unitCount = 0;      // 场上拥有该羁绊的单位数
        int activeThreshold = 0; // 当前激活的最高档位（0=未激活, 2=第一档, 4=第二档）
    };

    // 羁绊系统：统计场上单位的羁绊数量并计算激活效果。
    // 阶段三实现。
    class SynergySystem
    {
    public:
        SynergySystem() = default;

        // ---- 更新羁绊状态 ----
        // 遍历场上所有单位，统计各羁绊数量。
        // #TODO: 实现统计逻辑
        void Update(const std::vector<std::shared_ptr<Unit>>& units);

        // ---- 获取激活的羁绊列表 ----
        const std::vector<SynergyInfo>& GetActiveSynergies() const;

        // ---- 获取某个羁绊的激活信息 ----
        SynergyInfo GetSynergyInfo(const Trait& trait) const;

        // ---- 检查羁绊是否已激活到指定档位 ----
        bool IsSynergyActive(const Trait& trait, int threshold) const;

        // ---- 清除所有羁绊状态 ----
        void Clear();

    private:
        std::unordered_map<Trait, int> m_traitCounts;   // 羁绊 → 数量
        std::vector<SynergyInfo> m_activeSynergies;

        // 根据 n_t 返回激活的最高档位阈值（2 或 4），不足返回 0。
        // #TODO: 可扩展更多档位
        static int GetActivatedThreshold(int count);
    };
}
