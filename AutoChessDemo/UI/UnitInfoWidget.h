#pragma once

#include <QWidget>
#include <QPushButton>
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
        // isShopPreview = true 表示该单位来自商店预览（不显示出售按钮）
        void ShowUnit(std::shared_ptr<Unit> unit, bool isShopPreview = false);

        // 清空显示。
        void Clear();

    signals:
        // 点击出售按钮时发出
        void SellRequested(std::shared_ptr<Unit> unit);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        std::shared_ptr<Unit> m_selectedUnit;
        bool m_isShopPreview = false;
        QPushButton* m_sellButton = nullptr;

        void UpdateSellButton();
    };
}
