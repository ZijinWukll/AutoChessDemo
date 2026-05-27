#include "ShopSystem.h"
#include "GameConfig.h"

#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace synera
{
    // 单位模板（与 AIController 共享相同的定义，但这里是商店用的玩家单位）
    struct ShopUnitTemplate {
        std::string name;
        int hp, atk, range, maxMana;
        std::vector<Trait> traits;
        int cost;   // 费用: 1-5
    };

    static const ShopUnitTemplate s_shopTemplates[] = {
        // 1 费（cost=1）
        {"步兵",    300, 25, 1, MAX_MANA, {"人类", "战士"}, 1},
        {"弓箭手",  200, 30, 3, MAX_MANA, {"精灵", "远程"}, 1},
        {"侍从",    200, 15, 1, MAX_MANA, {"人类", "侍从"}, 1},
        // 2 费（cost=2）
        {"法师",    250, 40, 2, MAX_MANA, {"人类", "法师"}, 2},
        {"骑士",    500, 20, 1, MAX_MANA, {"人类", "骑士"}, 2},
        {"治疗师",  300, 15, 2, MAX_MANA, {"精灵", "治疗"}, 2},
        // 3 费（cost=3）
        {"刺客",    200, 50, 1, MAX_MANA, {"精灵", "刺客"}, 3},
        {"狂战士",  400, 45, 1, MAX_MANA, {"兽人", "战士"}, 3},
        {"狙击手",  250, 55, 4, MAX_MANA, {"精灵", "远程"}, 3},
    };

    static constexpr int NUM_1_COST = 3;  // 索引 0-2
    static constexpr int NUM_2_COST = 3;  // 索引 3-5
    static constexpr int NUM_3_COST = 3;  // 索引 6-8

    ShopSystem::ShopSystem()
        : m_shopUnits(SHOP_SIZE, nullptr)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void ShopSystem::Refresh(int shopLevel)
    {
        m_shopUnits.clear();
        for (int i = 0; i < SHOP_SIZE; ++i)
        {
            auto unit = GenerateRandomUnit(shopLevel);
            m_shopUnits.push_back(unit);
        }
    }

    std::shared_ptr<Unit> ShopSystem::BuyUnit(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_shopUnits.size()))
            return nullptr;
        if (m_shopUnits[index] == nullptr)
            return nullptr;

        auto unit = m_shopUnits[index];
        m_shopUnits[index] = nullptr;
        return unit;
    }

    int ShopSystem::GetRefreshCost() const
    {
        // 第1次免费，第2次2金币，第3次3金币...第N次N金币
        if (m_refreshCount == 0)
            return 0;
        return m_refreshCount + 1;
    }

    int ShopSystem::GetFreeRefreshesRemaining() const
    {
        return (m_refreshCount == 0) ? 1 : 0;
    }

    int ShopSystem::GetRefreshCount() const { return m_refreshCount; }

    void ShopSystem::RecordRefresh()
    {
        ++m_refreshCount;
    }

    void ShopSystem::ResetRefreshCount()
    {
        m_refreshCount = 0;
    }

    const std::vector<std::shared_ptr<Unit>>& ShopSystem::GetShopUnits() const
    {
        return m_shopUnits;
    }

    std::shared_ptr<Unit> ShopSystem::GenerateRandomUnit(int shopLevel) const
    {
        // 根据商店等级确定费用概率
        // 等级 | 1费  | 2费  | 3费
        //  1   | 100% |  0%  |  0%
        //  2   | 65%  | 30%  |  5%
        //  3   | 40%  | 35%  | 25%
        //  4   | 20%  | 35%  | 45%
        //  5   | 10%  | 30%  | 60%
        //  6+  |  5%  | 20%  | 75%

        int cost = 1;
        int roll = std::rand() % 100;

        // clamped shop level
        int lvl = std::max(1, std::min(shopLevel, 9));

        // 费用概率决定
        if (lvl == 1)
            cost = 1;
        else if (lvl == 2)
        {
            if (roll < 65) cost = 1;
            else if (roll < 95) cost = 2;
            else cost = 3;
        }
        else if (lvl == 3)
        {
            if (roll < 40) cost = 1;
            else if (roll < 75) cost = 2;
            else cost = 3;
        }
        else if (lvl == 4)
        {
            if (roll < 20) cost = 1;
            else if (roll < 55) cost = 2;
            else cost = 3;
        }
        else if (lvl == 5)
        {
            if (roll < 10) cost = 1;
            else if (roll < 40) cost = 2;
            else cost = 3;
        }
        else  // lvl >= 6
        {
            if (roll < 5) cost = 1;
            else if (roll < 25) cost = 2;
            else cost = 3;
        }

        // 在对应费用的模板池中随机选择一个
        int templateIndex = 0;
        int poolSize = 0;

        switch (cost)
        {
        case 1:
            templateIndex = std::rand() % NUM_1_COST;  // 0~2
            poolSize = NUM_1_COST;
            break;
        case 2:
            templateIndex = NUM_1_COST + (std::rand() % NUM_2_COST);  // 3~5
            poolSize = NUM_2_COST;
            break;
        case 3:
            templateIndex = NUM_1_COST + NUM_2_COST + (std::rand() % NUM_3_COST);  // 6~8
            poolSize = NUM_3_COST;
            break;
        }

        const auto& tpl = s_shopTemplates[templateIndex];

        return std::make_shared<Unit>(
            tpl.name, Owner::PlayerCtrl,
            tpl.hp, tpl.atk, tpl.range,
            tpl.maxMana, tpl.traits,
            StarLevel::One
        );
    }
}
