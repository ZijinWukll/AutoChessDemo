#include "AIController.h"
#include "GameConfig.h"

#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace synera
{
    // 单位模板
    struct UnitTemplate {
        std::string name;
        int hp, atk, range;
        std::vector<Trait> traits;
        int cost;       // 费用（1-5）
    };

    static const UnitTemplate s_unitTemplates[] = {
        // 1 费
        {"步兵",    300, 25, 1, {"人类", "战士"}, 1},
        {"弓箭手",  200, 30, 3, {"精灵", "远程"}, 1},
        {"侍从",    250, 20, 1, {"人类", "侍从"}, 1},
        // 2 费
        {"法师",    250, 40, 2, {"人类", "法师"}, 2},
        {"骑士",    500, 20, 1, {"人类", "骑士"}, 2},
        {"治疗师",  300, 15, 2, {"精灵", "治疗"}, 2},
        // 3 费
        {"刺客",    200, 50, 1, {"精灵", "刺客"}, 3},
        {"狂战士",  400, 45, 1, {"兽人", "战士"}, 3},
        {"狙击手",  250, 55, 4, {"精灵", "远程"}, 3},
    };

    std::vector<std::shared_ptr<Unit>> AIController::GenerateEnemyWave(int waveNumber) const
    {
        std::vector<std::shared_ptr<Unit>> enemies;
        float mult = GetWaveMultiplier(waveNumber);

        auto createUnit = [&](const UnitTemplate& tpl) -> std::shared_ptr<Unit> {
            return std::make_shared<Unit>(
                tpl.name, Owner::EnemyCtrl,
                static_cast<int>(tpl.hp * mult),
                static_cast<int>(tpl.atk * mult),
                tpl.range,
                MAX_MANA,
                tpl.traits,
                StarLevel::One
            );
        };

        // 波次生成策略
        switch (waveNumber)
        {
        case 1:
            // 2 个步兵
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[0]));
            break;
        case 2:
            // 2 步兵 + 1 弓箭手
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[1]));
            break;
        case 3:
            // 2 步兵 + 2 弓箭手
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[1]));
            enemies.push_back(createUnit(s_unitTemplates[1]));
            break;
        case 4:
            // 1 骑士 + 2 步兵 + 1 法师
            enemies.push_back(createUnit(s_unitTemplates[4])); // 骑士
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[3])); // 法师
            break;
        case 5:
            // 2 骑士 + 2 法师 + 1 治疗
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[5])); // 治疗
            break;
        case 6:
            // 1 狂战士 + 1 刺客 + 2 步兵 + 1 弓箭
            enemies.push_back(createUnit(s_unitTemplates[7])); // 狂战士
            enemies.push_back(createUnit(s_unitTemplates[6])); // 刺客
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[0]));
            enemies.push_back(createUnit(s_unitTemplates[1]));
            break;
        case 7:
            // 2 狂战士 + 2 法师 + 1 狙击手
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[8])); // 狙击手
            break;
        case 8:
            // 2 刺客 + 1 狙击 + 2 骑士 + 1 治疗
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        case 9:
            // 3 狂战士 + 2 狙击 + 1 法师
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            break;
        case 10:
            // 精英关：3 狂战士 + 2 狙击手 + 2 法师 + 1 治疗
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        case 11:
            // 4 狂战士 + 2 狙击手 + 2 刺客 + 1 治疗
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        case 12:
            // 全 3 费精锐：3 狂战士 + 3 狙击手 + 2 刺客 + 1 治疗
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        case 13:
            // 混合大军：2 狂战士 + 2 狙击手 + 2 骑士 + 2 法师 + 2 刺客
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[4]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[3]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            break;
        case 14:
            // 终极精英：4 狂战士 + 3 狙击手 + 2 刺客 + 1 治疗（10 个单位）
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        case 15:
            // Boss 关：Boss + 4 狂战士 + 3 狙击手 + 2 刺客 + 1 治疗
        {
            // Boss 使用刺客模板但大幅增强（超高属性）
            auto boss = std::make_shared<Unit>(
                "Boss", Owner::EnemyCtrl,
                static_cast<int>(1200 * mult),
                static_cast<int>(70 * mult),
                2, MAX_MANA,
                std::vector<Trait>{"恶魔", "Boss"},
                StarLevel::Three  // Boss 是 3★
            );
            enemies.push_back(boss);
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[7]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[8]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[6]));
            enemies.push_back(createUnit(s_unitTemplates[5]));
            break;
        }
        default:
            // 超高难度：8 个随机高费单位
            for (int i = 0; i < 8; ++i)
            {
                int idx = 3 + (std::rand() % 6); // 只选 2-3 费单位
                enemies.push_back(createUnit(s_unitTemplates[idx]));
            }
            break;
        }

        return enemies;
    }

    void AIController::DeployEnemies(const std::vector<std::shared_ptr<Unit>>& enemies, Board& board)
    {
        // 近战（Range<=1）放行 2-3，远程（Range>1）放行 0-1
        std::vector<std::shared_ptr<Unit>> melee, ranged;
        for (auto& u : enemies)
        {
            if (!u) continue;
            if (u->GetRange() <= 1)
                melee.push_back(u);
            else
                ranged.push_back(u);
        }

        auto placeUnits = [&](std::vector<std::shared_ptr<Unit>>& units, int startRow, int endRow) {
            for (auto& u : units)
            {
                bool placed = false;
                for (int r = startRow; r <= endRow && !placed; ++r)
                    for (int c = 0; c < board.GetCols() && !placed; ++c)
                    {
                        Position p(r, c);
                        if (!board.IsOccupied(p))
                        {
                            board.PlaceUnit(u, p);
                            u->SetGridPosition(r, c);
                            u->SetState(UnitState::Idle);
                            placed = true;
                        }
                    }
            }
        };

        placeUnits(ranged, 0, 1);
        placeUnits(melee, 2, board.GetRows() / 2 - 1);
    }

    float AIController::GetWaveMultiplier(int waveNumber) const
    {
        // 平滑递增：每波 +8%，第 10 波后额外 +8%（原先是15%，跳跃太大）
        float base = 1.0f + (waveNumber - 1) * 0.08f;
        if (waveNumber >= 10)
            base *= 1.08f;
        if (waveNumber >= 13)
            base *= 1.1f;
        return std::min(4.5f, base);
    }
}
