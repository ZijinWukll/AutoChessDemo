#pragma once

#include <memory>
#include <vector>

#include "Unit.h"

namespace synera
{
    // 商店系统：管理每回合的单位刷新、购买。
    // 阶段三实现。
    class ShopSystem
    {
    public:
        ShopSystem();

        // ---- 刷新商店 ----
        // 根据当前商店等级刷新 SHOP_SIZE 个单位。
        // #TODO: 实现刷新逻辑
        //   等级越高，高费单位出现概率越大
        void Refresh(int shopLevel);

        // ---- 购买单位 ----
        // 购买商店中第 index 个单位（0-based）。
        // 返回单位指针，售罄或索引越界返回 nullptr。
        // #TODO: 实现扣费逻辑
        std::shared_ptr<Unit> BuyUnit(int index);

        // ---- 剩余免费刷新次数 ----
        int GetFreeRefreshes() const;
        void SetFreeRefreshes(int count);
        void UseFreeRefresh();          // 消耗一次免费刷新

        // ---- 商店数据 ----
        const std::vector<std::shared_ptr<Unit>>& GetShopUnits() const;

    private:
        std::vector<std::shared_ptr<Unit>> m_shopUnits;
        int m_freeRefreshes = 1;

        // #TODO: 根据概率表随机生成一个单位
        //   1 费：70%, 2 费：25%, 3 费：5%
        //   等级提升后高费概率增加
        std::shared_ptr<Unit> GenerateRandomUnit(int shopLevel) const;
    };
}
