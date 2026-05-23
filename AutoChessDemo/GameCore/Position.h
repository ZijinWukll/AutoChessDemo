#pragma once

namespace synera
{
    // 棋盘坐标：x 表示行，y 表示列，均使用 0-based 索引。
    struct Position
    {
        int x = 0;
        int y = 0;

        Position() = default;
        Position(int row, int col) : x(row), y(col) {}

        bool operator==(const Position& other) const
        {
            return x == other.x && y == other.y;
        }

        bool operator!=(const Position& other) const
        {
            return !(*this == other);
        }
    };
}
