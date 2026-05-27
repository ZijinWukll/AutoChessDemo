#pragma once

#include <memory>
#include <vector>

#include "Unit.h"

namespace synera
{
    // 合星系统：管理同名同星单位的合成升星。
    // 阶段三实现。
    class MergeSystem
    {
    public:
        static constexpr const char* SQUIRE_NAME = "侍从";

        MergeSystem() = default;

        // ---- 尝试合星 ----
        // 在 units 中查找所有可合成的组合并执行合成。
        // 合成规则：
        //   3 个同名同星 → 1 个高星同名单位（标准合星）
        //   2 个同名同星 + 1 个侍从 → 1 个高星同名单位（催化合星，侍从被消耗）
        //   3 个侍从 → 2★ 侍从（自我合星）
        //   返回是否发生了合成
        bool TryMerge(std::vector<std::shared_ptr<Unit>>& units);

        // ---- 检查是否有可合成组合 ----
        bool HasMergeable(const std::vector<std::shared_ptr<Unit>>& units) const;

    private:
        // 在 units 中查找所有可合星的组合（标准 3 同名 + 催化 2同名+侍从 + 侍从自我合星）
        //   返回每个组合的成员列表（每组固定 3 个成员）
        struct MergeGroup
        {
            std::string name;
            StarLevel star;
            std::vector<std::shared_ptr<Unit>> members;
        };
        std::vector<MergeGroup> FindMergeGroups(const std::vector<std::shared_ptr<Unit>>& units) const;

        // #TODO: 创建一个高星单位（属性提升）
        std::shared_ptr<Unit> CreateMergedUnit(const std::vector<std::shared_ptr<Unit>>& sameUnits) const;
    };
}
