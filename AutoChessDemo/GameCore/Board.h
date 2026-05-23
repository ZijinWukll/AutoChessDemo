#pragma once

#include <memory>
#include <vector>

#include "Position.h"
#include "Unit.h"

namespace synera
{
    // 主棋盘：M x N 网格，每个地块同一时刻最多容纳一个单位。
    class Board
    {
    public:
        Board(int rows, int cols);

        int GetRows() const;
        int GetCols() const;

        // 判断坐标是否在棋盘范围内。
        bool IsInBounds(const Position& pos) const;

        // 按作业定义：上半区为敌方半场，下半区为玩家半场。
        bool IsEnemyHalf(const Position& pos) const;
        bool IsPlayerHalf(const Position& pos) const;

        bool IsOccupied(const Position& pos) const;
        std::shared_ptr<Unit> GetOccupant(const Position& pos) const;

        // 放置单位：目标格必须合法且为空。
        bool PlaceUnit(const std::shared_ptr<Unit>& unit, const Position& pos);

        // 移动单位：起点必须有单位，终点必须为空且合法。
        bool MoveUnit(const Position& from, const Position& to);

        // 移除某个地块上的单位。
        bool RemoveUnit(const Position& pos);

        // 清空棋盘。
        void Clear();

    private:
        int m_rows = 0;
        int m_cols = 0;
        std::vector<std::vector<std::shared_ptr<Unit>>> m_grid;
    };
}
