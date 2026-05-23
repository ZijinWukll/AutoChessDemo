#pragma once

#include <QWidget>
#include <memory>

#include "GameCore/GameManager.h"

namespace synera
{
    class BenchWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit BenchWidget(GameManager& gameManager, QWidget* parent = nullptr);

        QRect GetSlotRect(int index) const;

        // 选中状态管理
        void SetSelectedSlot(int index);
        void ClearSelectedSlot();

    signals:
        void SlotClicked(int index);
        void SellRequested(int index);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        GameManager& m_gameManager;
        int m_slotCount = 8;
        int m_selectedSlot = -1;

        int CountSameNameStar(const std::string& name, StarLevel star) const;
    };
}
