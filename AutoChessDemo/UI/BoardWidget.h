#pragma once

#include <QWidget>
#include <QPainter>
#include <memory>

#include "GameCore/GameManager.h"

namespace synera
{
    // 部署动画：单位从备战区飞到棋盘
    struct DeployAnim {
        bool active = false;
        QPointF fromPos;      // 屏幕起始坐标
        QPointF toPos;        // 屏幕目标坐标
        float progress = 0.0f;
        std::shared_ptr<Unit> unit;
    };

    // 移动动画：战斗中单位走格
    struct MoveAnim {
        bool active = false;
        QPointF fromPos;
        QPointF toPos;
        float progress = 0.0f;
        float speed = 0.03f;
        std::shared_ptr<Unit> unit;
    };

    class BoardWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit BoardWidget(GameManager& gameManager, QWidget* parent = nullptr);

        void SetHighlight(const std::vector<Position>& positions, const QColor& color);
        void ClearHighlights();
        void SetDragZoneHighlight(bool visible);  // 拖拽时高亮己方半场
        void SetDraggedUnitPosition(int row, int col); // 棋盘拖拽时标记被拖单位位置（半透明）
        void ClearDraggedUnitPosition();

        // 动画
        void StartDeployAnimation(std::shared_ptr<Unit> unit, QPointF fromScreenPos);
        void UpdateAnimations();
        bool HasActiveAnim() const { return m_deployAnim.active; }
        QPoint GetCellScreenPos(int row, int col) const;

    signals:
        void CellClicked(int row, int col);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

    private:
        GameManager& m_gameManager;
        QRect GetCellRect(int row, int col) const;
        void DrawUnitAt(QPainter& painter, std::shared_ptr<Unit> unit,
                        const QRect& cellRect, int cellW, int cellH);

        std::vector<std::pair<Position, QColor>> m_highlights;

        DeployAnim m_deployAnim;
        std::vector<MoveAnim> m_moveAnims;
        bool m_dragZoneVisible = false;
        int m_draggedUnitRow = -1;      // 棋盘拖拽时原位置（该处单位用半透明绘制）
        int m_draggedUnitCol = -1;
        int m_lastHoverRow = -1;
        int m_lastHoverCol = -1;
    };
}
