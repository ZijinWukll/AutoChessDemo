#include "Board.h"

namespace synera
{
    Board::Board(int rows, int cols)
        : m_rows(rows > 0 ? rows : 1),
          m_cols(cols > 0 ? cols : 1),
          m_grid(static_cast<size_t>(m_rows), std::vector<std::shared_ptr<Unit>>(static_cast<size_t>(m_cols), nullptr))
    {
    }

    int Board::GetRows() const
    {
        return m_rows;
    }

    int Board::GetCols() const
    {
        return m_cols;
    }

    bool Board::IsInBounds(const Position& pos) const
    {
        return pos.x >= 0 && pos.x < m_rows && pos.y >= 0 && pos.y < m_cols;
    }

    bool Board::IsEnemyHalf(const Position& pos) const
    {
        return IsInBounds(pos) && pos.x < (m_rows / 2);
    }

    bool Board::IsPlayerHalf(const Position& pos) const
    {
        return IsInBounds(pos) && pos.x >= (m_rows / 2);
    }

    bool Board::IsOccupied(const Position& pos) const
    {
        if (!IsInBounds(pos))
        {
            return false;
        }

        return m_grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)] != nullptr;
    }

    std::shared_ptr<Unit> Board::GetOccupant(const Position& pos) const
    {
        if (!IsInBounds(pos))
        {
            return nullptr;
        }

        return m_grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)];
    }

    bool Board::PlaceUnit(const std::shared_ptr<Unit>& unit, const Position& pos)
    {
        if (!unit || !IsInBounds(pos) || IsOccupied(pos))
        {
            return false;
        }

        m_grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)] = unit;
        return true;
    }

    bool Board::MoveUnit(const Position& from, const Position& to)
    {
        if (!IsInBounds(from) || !IsInBounds(to) || !IsOccupied(from) || IsOccupied(to))
        {
            return false;
        }

        m_grid[static_cast<size_t>(to.x)][static_cast<size_t>(to.y)] = m_grid[static_cast<size_t>(from.x)][static_cast<size_t>(from.y)];
        m_grid[static_cast<size_t>(from.x)][static_cast<size_t>(from.y)] = nullptr;
        return true;
    }

    bool Board::RemoveUnit(const Position& pos)
    {
        if (!IsInBounds(pos) || !IsOccupied(pos))
        {
            return false;
        }

        m_grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)] = nullptr;
        return true;
    }

    void Board::Clear()
    {
        for (auto& row : m_grid)
        {
            for (auto& cell : row)
            {
                cell = nullptr;
            }
        }
    }
}
