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
        MergeSystem() = default;

        // ---- 尝试合星 ----
        // 在 units 中查找所有可合成的组合并执行合成。
        // 合成规则：
        //   3 个同名同星 → 1 个高星同名单位
        //   2★ 合成需要 3 个同名 1★ → 1 个 2★
        //   3★ 合成需要 3 个同名 2★ → 1 个 3★
        // #TODO: 实现合星逻辑
        //   返回是否发生了合成
        bool TryMerge(std::vector<std::shared_ptr<Unit>>& units);

        // ---- 检查是否有可合成组合 ----
        // #TODO: 检测但不执行
        bool HasMergeable(const std::vector<std::shared_ptr<Unit>>& units) const;

    private:
        // #TODO: 在 units 中查找同名的单位，按星级分组
        //   返回：同名单位列表，按星级归类
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
