#pragma once

#include <QWidget>
#include <memory>

#include "GameCore/GameManager.h"

namespace synera
{
    // 商店面板：显示可购买的单位。
    // 阶段三实现。
    class ShopWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit ShopWidget(GameManager& gameManager, QWidget* parent = nullptr);

        // 刷新显示。
        void RefreshDisplay();

    signals:
        // 玩家点击购买了第 index 个单位。
        void UnitPurchased(int index);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private:
        GameManager& m_gameManager;

        // #TODO: 刷新按钮
        // #TODO: 显示刷新次数和金币
    };
}
