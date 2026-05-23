#pragma once

#include <memory>
#include <vector>

#include "Unit.h"
#include "Board.h"

namespace synera
{
    // AI 控制器：生成敌方单位阵容，控制敌方在战斗中的行为。
    // 阶段二 PvE 使用，后续可扩展到 PvP。
    class AIController
    {
    public:
        AIController() = default;

        // ---- 生成敌方阵容 ----
        // 根据当前波次 waveNumber 生成对应的敌方单位阵容。
        // #TODO: 实现波次生成逻辑
        //   第 1-3 波：少量低属性单位
        //   第 4-6 波：中等属性，开始出现羁绊组合
        //   第 7-9 波：高属性单位
        //   第 10 波：Boss 波
        std::vector<std::shared_ptr<Unit>> GenerateEnemyWave(int waveNumber) const;

        // ---- 敌方布阵 ----
        // 将生成的敌方单位自动布置到棋盘上半区。
        // #TODO: 实现自动布阵逻辑（分散放置，近战在前远程在后）
        void DeployEnemies(const std::vector<std::shared_ptr<Unit>>& enemies, Board& board);

    private:
        // 根据波次计算敌方单位的属性倍率。
        // #TODO: 随波次线性增长
        float GetWaveMultiplier(int waveNumber) const;
    };
}
