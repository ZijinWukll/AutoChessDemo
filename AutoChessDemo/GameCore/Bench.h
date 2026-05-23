#pragma once

#include <memory>
#include <vector>

#include "Unit.h"

namespace synera
{
    // 备战区：一维槽位，每个槽位最多放一个单位。
    class Bench
    {
    public:
        explicit Bench(size_t capacity);

        size_t GetCapacity() const;
        bool IsFull() const;

        // 自动放入第一个空槽位。
        bool AddUnit(const std::shared_ptr<Unit>& unit);

        // 指定槽位放置单位。
        bool AddUnitAt(const std::shared_ptr<Unit>& unit, size_t index);

        // 取出并移除单位（成功返回单位，失败返回 nullptr）。
        std::shared_ptr<Unit> RemoveUnit(size_t index);

        std::shared_ptr<Unit> GetUnit(size_t index) const;

        // 获取第一个空槽位，找不到返回 -1。
        int FindFirstEmptySlot() const;

    private:
        std::vector<std::shared_ptr<Unit>> m_slots;
    };
}
