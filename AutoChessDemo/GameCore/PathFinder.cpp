#include "PathFinder.h"

#include <queue>
#include <unordered_set>
#include <algorithm>

namespace synera
{
    PathFinder::PathFinder(const Board& board)
        : m_board(board)
    {
    }

    std::optional<std::vector<Position>> PathFinder::FindPath(const Position& from, const Position& to) const
    {
        if (!m_board.IsInBounds(from) || !m_board.IsInBounds(to))
            return std::nullopt;

        const int rows = m_board.GetRows();
        const int cols = m_board.GetCols();

        // visited 和 prev 二维数组
        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
        std::vector<std::vector<std::pair<int, int>>> prev(rows, std::vector<std::pair<int, int>>(cols, {-1, -1}));

        std::queue<Position> q;
        visited[from.x][from.y] = true;
        q.push(from);

        while (!q.empty())
        {
            Position cur = q.front();
            q.pop();

            if (cur == to)
                break;

            for (const Position& neighbor : GetNeighbors(cur))
            {
                if (!visited[neighbor.x][neighbor.y] &&
                    (!m_board.IsOccupied(neighbor) || neighbor == to))
                {
                    visited[neighbor.x][neighbor.y] = true;
                    prev[neighbor.x][neighbor.y] = {cur.x, cur.y};
                    q.push(neighbor);
                }
            }
        }

        if (!visited[to.x][to.y])
            return std::nullopt;

        // 重建路径（不含起点 from，含终点 to）
        std::vector<Position> path;
        Position cur = to;
        while (cur != from)
        {
            path.push_back(cur);
            cur = {prev[cur.x][cur.y].first, prev[cur.x][cur.y].second};
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    std::optional<Position> PathFinder::FindNearestReachable(const Position& from, const std::vector<Position>& targets) const
    {
        std::optional<Position> bestTarget;
        size_t bestDist = SIZE_MAX;

        for (const auto& target : targets)
        {
            auto path = FindPath(from, target);
            if (path.has_value() && path->size() < bestDist)
            {
                bestDist = path->size();
                bestTarget = target;
            }
        }
        return bestTarget;
    }

    std::vector<Position> PathFinder::GetNeighbors(const Position& pos) const
    {
        std::vector<Position> neighbors;
        const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (const auto& d : dirs)
        {
            Position next(pos.x + d[0], pos.y + d[1]);
            if (m_board.IsInBounds(next))
                neighbors.push_back(next);
        }
        return neighbors;
    }
}
