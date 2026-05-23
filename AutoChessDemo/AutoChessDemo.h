#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QSplitter>
#include <QMouseEvent>

#include <QPushButton>
#include "ui_AutoChessDemo.h"
#include "GameCore/GameManager.h"
#include "UI/BoardWidget.h"
#include "UI/BenchWidget.h"
#include "UI/UnitInfoWidget.h"
#include "UI/ShopWidget.h"

using namespace synera;

class AutoChessDemo : public QMainWindow
{
    Q_OBJECT

public:
    AutoChessDemo(QWidget *parent = nullptr);
    ~AutoChessDemo();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // ---- 游戏循环 ----
    void OnTick();                      // 每帧调用 GameManager::Update()

    // ---- 布阵操作 ----
    void OnBoardCellClicked(int row, int col);      // 点击棋盘格子
    void OnBenchSlotClicked(int index);              // 点击备战区槽位
    void OnBenchSellRequested(int index);            // 出售备战区单位

    // ---- 商店操作（阶段三） ----
    void OnShopUnitClicked(int index);              // 点击商店单位查看详情
    void OnConfirmPurchase();                       // 确认购买

    // ---- 按钮事件 ----
    void OnStartCombatClicked();        // 开始战斗
    void OnRefreshShopClicked();        // 刷新商店

private:
    // ---- UI 组件 ----
    Ui::AutoChessDemoClass ui;
    synera::GameManager m_gameManager;

    BoardWidget* m_boardWidget = nullptr;
    BenchWidget* m_benchWidget = nullptr;
    UnitInfoWidget* m_unitInfoWidget = nullptr;
    ShopWidget* m_shopWidget = nullptr;

    QLabel* m_waveLabel = nullptr;      // 显示当前波次
    QLabel* m_goldLabel = nullptr;      // 显示金币
    QLabel* m_hpLabel = nullptr;        // 显示玩家血量
    QLabel* m_phaseLabel = nullptr;     // 显示当前阶段
    QLabel* m_tipLabel = nullptr;       // 操作提示

    QTimer* m_timer = nullptr;          // 60fps 定时器

    // ---- 拖拽预览 ----
    QLabel* m_dragPreview = nullptr;    // 跟随鼠标的浮动单位预览

    // ---- 拖拽状态 ----
    struct DragInfo {
        bool active = false;            // 是否有拖拽操作
        enum class Source { None, Bench, Board } source = Source::None;
        int slotIndex = -1;             // 来源备战区槽位（bench 拖拽）
        int boardRow = -1;              // 来源棋盘坐标（棋盘拖拽）
        int boardCol = -1;
        std::shared_ptr<Unit> unit;     // 拖拽的单位
    };
    DragInfo m_dragInfo;

    // ---- 战斗结果展示 ----
    QString m_combatResultText;
    qint64 m_combatResultEndTime = 0;   // 结果展示自动消失的时间戳（ms）
    QLabel* m_combatResultOverlay = nullptr;

    // ---- 商店预览 ----
    std::shared_ptr<Unit> m_shopPreviewUnit;
    QPushButton* m_confirmPurchaseBtn = nullptr;

    // ---- 初始化 UI 布局 ----
    void SetupUI();
    void UpdateDragPreview(const QPoint& globalPos);  // 更新拖拽预览位置
    void HideDragPreview();
    void UpdateBoardHover(const QPoint& globalPos);   // 更新棋盘悬停高亮
    bool TryDeployFromDrag(int row, int col);         // 从拖拽放置单位
    void CancelDrag();                                 // 取消拖拽/选中
};
