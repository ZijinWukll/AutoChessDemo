#pragma once

#include <QWidget>
#include <memory>

#include "GameCore/Unit.h"

namespace synera
{
    // 单位详情面板：显示选中单位的完整属性信息。
    class UnitInfoWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit UnitInfoWidget(QWidget* parent = nullptr);

        // 显示指定单位的信息。
        void ShowUnit(std::shared_ptr<Unit> unit);

        // 清空显示。
        void Clear();

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        std::shared_ptr<Unit> m_selectedUnit;

        // #TODO: 详细渲染
        //   显示内容：
        //   - 单位名称（含星级 ★★★）
        //   - HP / MaxHP（血量条）
        //   - ATK（攻击力）
        //   - Range（攻击范围）
        //   - Mana / MaxMana（法力条）
        //   - 羁绊标签列表
        //   - 装备列表（阶段三）
    };
}
