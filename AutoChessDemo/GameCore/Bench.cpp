#include "Bench.h"

namespace synera
{
    Bench::Bench(size_t capacity)
        : m_slots(capacity, nullptr)
    {
    }

    size_t Bench::GetCapacity() const
    {
        return m_slots.size();
    }

    bool Bench::IsFull() const
    {
        return FindFirstEmptySlot() == -1;
    }

    bool Bench::AddUnit(const std::shared_ptr<Unit>& unit)
    {
        const int slot = FindFirstEmptySlot();
        if (!unit || slot < 0)
        {
            return false;
        }

        m_slots[static_cast<size_t>(slot)] = unit;
        return true;
    }

    bool Bench::AddUnitAt(const std::shared_ptr<Unit>& unit, size_t index)
    {
        if (!unit || index >= m_slots.size() || m_slots[index] != nullptr)
        {
            return false;
        }

        m_slots[index] = unit;
        return true;
    }

    std::shared_ptr<Unit> Bench::RemoveUnit(size_t index)
    {
        if (index >= m_slots.size())
        {
            return nullptr;
        }

        std::shared_ptr<Unit> unit = m_slots[index];
        m_slots[index] = nullptr;
        return unit;
    }

    std::shared_ptr<Unit> Bench::GetUnit(size_t index) const
    {
        if (index >= m_slots.size())
        {
            return nullptr;
        }

        return m_slots[index];
    }

    int Bench::FindFirstEmptySlot() const
    {
        for (size_t i = 0; i < m_slots.size(); ++i)
        {
            if (m_slots[i] == nullptr)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }
}
