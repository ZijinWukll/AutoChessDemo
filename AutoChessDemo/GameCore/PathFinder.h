#pragma once

#include <vector>
#include <optional>

#include "Position.h"
#include "Board.h"

namespace synera
{
    // 寻路算法：在 Board 上计算两点间最短路径。
    // 阶段二使用 BFS（网格权重均匀），后续可扩展为 A*。
    class PathFinder
    {
    public:
        explicit PathFinder(const Board& board);

        // 使用 BFS 计算从 from 到 to 的最短路径。
        // 返回路径坐标序列（含起点不含终点，供逐格移动），
        // 若不可达则返回 std::nullopt。
        // #TODO: 实现 BFS 逻辑
        std::optional<std::vector<Position>> FindPath(const Position& from, const Position& to) const;

        // 查找从 from 出发，离目标 positions 中最近的可达位置。
        // 用于"找个最近的敌人打"的场景。
        // #TODO: 从所有目标中选 BFS 距离最短的
        std::optional<Position> FindNearestReachable(const Position& from, const std::vector<Position>& targets) const;

        // 获取 from 周围所有可移动的相邻格子（四方向）。
        std::vector<Position> GetNeighbors(const Position& pos) const;

    private:
        const Board& m_board;
    };
}
