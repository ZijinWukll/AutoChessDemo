#include "AutoChessDemo.h"
#include "GameCore/AudioManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QApplication>
#include <QScreen>
#include <QCursor>
#include <QMessageBox>
#include <QIcon>
#include <QPainter>
#include <QPainter>
#include <QMap>
#include <QDateTime>
#include <algorithm>

AutoChessDemo::AutoChessDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 初始化游戏核心
    m_gameManager.Initialize();

    // 构建 UI
    SetupUI();

    // 启动游戏循环定时器（~100fps ≈ 10ms）
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AutoChessDemo::OnTick);
    m_timer->start(10);

    setWindowTitle("Synera: Synergy Auto-Arena");
    resize(800, 850);
    setMinimumSize(750, 750);
    // 居中显示
    if (auto* screen = QGuiApplication::primaryScreen())
    {
        QRect screenRect = screen->availableGeometry();
        move((screenRect.width() - width()) / 2, (screenRect.height() - height()) / 2);
    }

    // 初始化音频（仅用于回合结束提示音，不播放 BGM）
    synera::AudioManager::Init();

    // 程序化生成游戏图标
    {
        QPixmap iconPx(64, 64);
        iconPx.fill(Qt::transparent);
        QPainter ip(&iconPx);
        ip.setRenderHint(QPainter::Antialiasing);

        // 圆角方形背景 — 深蓝紫渐变
        QLinearGradient bg(0, 0, 64, 64);
        bg.setColorAt(0, QColor(60, 48, 150));
        bg.setColorAt(1, QColor(30, 20, 85));
        ip.setBrush(bg);
        ip.setPen(QPen(QColor(120, 100, 220, 160), 2));
        ip.drawRoundedRect(2, 2, 60, 60, 14, 14);

        // 外圈光晕
        ip.setPen(QPen(QColor(180, 160, 255, 30), 6));
        ip.setBrush(Qt::NoBrush);
        ip.drawRoundedRect(0, 0, 64, 64, 16, 16);

        // 大号 "S" — 亮金色，深色描边
        QFont sf = ip.font();
        sf.setPointSize(34);
        sf.setBold(true);
        ip.setFont(sf);

        // 深色描边
        ip.setPen(QPen(QColor(20, 12, 55, 220), 8));
        ip.drawText(QRect(0, 0, 64, 62), Qt::AlignCenter, "S");
        // 亮金色填充
        ip.setPen(QPen(QColor(255, 215, 0), 4));
        ip.drawText(QRect(0, 0, 64, 62), Qt::AlignCenter, "S");

        // 金色装饰 — 四角括号（粗大，不遮挡 S）
        ip.setPen(QPen(QColor(255, 215, 0, 160), 3));
        int c = 17, m = 10; // corner offset, margin
        // 左上
        ip.drawLine(m, c, m, m); ip.drawLine(m, m, c, m);
        // 右上
        ip.drawLine(64 - c, m, 64 - m, m); ip.drawLine(64 - m, m, 64 - m, c);
        // 左下
        ip.drawLine(m, 64 - c, m, 64 - m); ip.drawLine(m, 64 - m, c, 64 - m);
        // 右下
        ip.drawLine(64 - c, 64 - m, 64 - m, 64 - m); ip.drawLine(64 - m, 64 - m, 64 - m, 64 - c);

        ip.end();
        setWindowIcon(QIcon(iconPx));
    }

    // 安装事件过滤器（用于拖拽预览和悬停高亮）
    installEventFilter(this);
    m_boardWidget->installEventFilter(this);
    m_benchWidget->installEventFilter(this);
    m_shopWidget->installEventFilter(this);

    setMouseTracking(true);
    m_boardWidget->setMouseTracking(true);
    m_benchWidget->setMouseTracking(true);

    // 应用全局 QSS 暗色主题 — 现代光滑风格
    setStyleSheet(R"(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #14121e, stop:1 #1a1826);
        }
        QLabel {
            color: #d0d0e0;
            font-size: 13px;
            padding: 3px 6px;
            background: transparent;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #5a50a8, stop:1 #4a4080);
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 8px 24px;
            font-size: 13px;
            font-weight: bold;
            min-height: 28px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #6a60c0, stop:1 #5a50a0);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #3a3070, stop:1 #4a4080);
        }
        QGroupBox {
            color: #b0b0d0;
            font-size: 12px;
            font-weight: bold;
            border: 1px solid #3a3850;
            border-radius: 10px;
            margin-top: 10px;
            padding: 24px 10px 10px 10px;
            background: rgba(26, 24, 38, 0.6);
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 3px 14px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #4a4080, stop:1 #2a2840);
            border-radius: 5px;
            color: #c8c0e8;
            font-size: 12px;
        }
        QSplitter::handle {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #2a2840, stop:0.5 #4a4080, stop:1 #2a2840);
            width: 4px;
            margin: 6px 0;
            border-radius: 2px;
        }
    )");
}

AutoChessDemo::~AutoChessDemo()
{
    m_timer->stop();
}

void AutoChessDemo::SetupUI()
{
    // ---- 创建中央控件 ----
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(8);

    // ---- 状态栏标签 ----
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_phaseLabel = new QLabel("准备阶段", this);
    m_waveLabel = new QLabel("第 1 波", this);
    m_goldLabel = new QLabel("金币: 0", this);
    m_hpLabel = new QLabel("HP: 100", this);
    statusLayout->addWidget(m_phaseLabel);
    statusLayout->addWidget(m_waveLabel);
    statusLayout->addStretch();
    statusLayout->addSpacing(16);
    statusLayout->addWidget(m_hpLabel);
    statusLayout->addSpacing(12);
    statusLayout->addWidget(m_goldLabel);
    mainLayout->addLayout(statusLayout);

    // ---- 羁绊状态栏 ----
    m_synergyLabel = new QLabel("", this);
    m_synergyLabel->setStyleSheet("color: #b090e0; font-size: 11px; padding: 2px 8px; "
                                   "background: rgba(40, 35, 60, 0.5); border-radius: 4px;");
    m_synergyLabel->setWordWrap(true);
    m_synergyLabel->setMaximumHeight(40);
    mainLayout->addWidget(m_synergyLabel);

    // ---- 棋盘 + 详情面板（水平分割） ----
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 棋盘
    m_boardWidget = new BoardWidget(m_gameManager, this);
    m_boardWidget->setMinimumSize(450, 400);
    connect(m_boardWidget, &BoardWidget::CellClicked, this, &AutoChessDemo::OnBoardCellClicked);
    mainSplitter->addWidget(m_boardWidget);

    // 战斗结果覆盖层（置于棋盘上方居中，默认隐藏）
    m_combatResultOverlay = new QLabel("", m_boardWidget);
    m_combatResultOverlay->setAlignment(Qt::AlignCenter);
    m_combatResultOverlay->setFixedSize(280, 90);
    m_combatResultOverlay->move(m_boardWidget->width() / 2 - 140, m_boardWidget->height() / 2 - 45);
    m_combatResultOverlay->hide();

    // 右侧：单位详情 + 确认购买按钮
    QVBoxLayout* rightPanel = new QVBoxLayout();
    m_unitInfoWidget = new UnitInfoWidget(this);
    rightPanel->addWidget(m_unitInfoWidget);

    // 确认购买按钮（默认隐藏，点击商店单位后显示）
    m_confirmPurchaseBtn = new QPushButton("确认购买", this);
    m_confirmPurchaseBtn->setStyleSheet(
        "QPushButton { background-color: #4a8f4a; color: white; border: none; "
        "border-radius: 5px; padding: 8px 18px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #5aaa5a; }"
        "QPushButton:pressed { background-color: #3a7a3a; }");
    m_confirmPurchaseBtn->hide();
    connect(m_confirmPurchaseBtn, &QPushButton::clicked, this, &AutoChessDemo::OnConfirmPurchase);
    rightPanel->addWidget(m_confirmPurchaseBtn);

    rightPanel->addStretch();

    QWidget* rightContainer = new QWidget(this);
    rightContainer->setLayout(rightPanel);
    rightContainer->setFixedWidth(220);
    mainSplitter->addWidget(rightContainer);

    mainLayout->addWidget(mainSplitter, 1); // stretch = 1

    // ---- 备战区 ----
    m_benchWidget = new BenchWidget(m_gameManager, this);
    m_benchWidget->setFixedHeight(80);
    connect(m_benchWidget, &BenchWidget::SlotClicked, this, &AutoChessDemo::OnBenchSlotClicked);
    connect(m_benchWidget, &BenchWidget::SellRequested, this, &AutoChessDemo::OnBenchSellRequested);
    mainLayout->addWidget(m_benchWidget);

    // ---- 商店 + 按钮（阶段三） ----
    QGroupBox* shopGroup = new QGroupBox("商店", this);
    QVBoxLayout* shopLayout = new QVBoxLayout(shopGroup);
    shopLayout->setContentsMargins(6, 4, 6, 4);
    shopLayout->setSpacing(4);

    m_shopWidget = new ShopWidget(m_gameManager, this);
    m_shopWidget->setFixedHeight(110);
    connect(m_shopWidget, &ShopWidget::UnitPurchased, this, &AutoChessDemo::OnShopUnitClicked);
    shopLayout->addWidget(m_shopWidget);

    // 连接出售按钮
    connect(m_unitInfoWidget, &UnitInfoWidget::SellRequested, this, [this](std::shared_ptr<Unit> unit) {
        if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
            return;
        if (!unit || m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
            return;

        // 取消拖拽/预览状态
        if (m_dragInfo.active) CancelDrag();
        if (m_shopPreviewUnit)
        {
            m_shopPreviewUnit.reset();
            if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
        }

        // 获取出售前金币用于显示
        int goldBefore = m_gameManager.GetGold();
        m_gameManager.SellUnit(unit);
        int goldEarned = m_gameManager.GetGold() - goldBefore;

        if (goldEarned > 0)
        {
            m_tipLabel->setText(QString("💰 出售成功！获得 %1 金币").arg(goldEarned));
        }
        m_unitInfoWidget->Clear();
        m_boardWidget->update();
        m_benchWidget->update();
    });

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* startCombatBtn = new QPushButton("⚔ 开始战斗", this);
    startCombatBtn->setToolTip("将备战区单位部署到棋盘上，然后点击开始战斗");
    connect(startCombatBtn, &QPushButton::clicked, this, &AutoChessDemo::OnStartCombatClicked);
    btnLayout->addWidget(startCombatBtn);

    QPushButton* refreshBtn = new QPushButton("🔄 刷新商店", this);
    refreshBtn->setToolTip("前2次免费，之后逐次递增1金币重新随机商店单位");
    connect(refreshBtn, &QPushButton::clicked, this, &AutoChessDemo::OnRefreshShopClicked);
    btnLayout->addWidget(refreshBtn);

    // 提示标签
    m_tipLabel = new QLabel("", this);
    m_tipLabel->setStyleSheet("color: #8888aa; font-size: 10px; padding: 0px;");
    btnLayout->addWidget(m_tipLabel);

    shopLayout->addLayout(btnLayout);
    mainLayout->addWidget(shopGroup);

    // 初始教程提示
    m_tipLabel->setText(
        "🎮 欢迎来到 Synera！ 备战阶段： ① 从商店点击英雄查看详情 → 确认购买  "
        "② 从备战区拖拽英雄到棋盘布阵 ③ 右键备战区出售英雄 ④ 点击「开始战斗」迎敌");
}

// ========== 游戏循环 ==========

void AutoChessDemo::OnTick()
{
    m_gameManager.Update(1.0f / 60.0f);

    // 更新动画
    m_boardWidget->UpdateAnimations();

    // 刷新 UI 显示（备战区合星、购买等操作后立即反映）
    m_boardWidget->update();
    m_benchWidget->update();

    // 更新状态栏（用 HTML 加颜色）
    auto phase = m_gameManager.GetCurrentPhase();
    static auto prevPhase = phase;
    QString phaseColor = (phase == synera::GamePhase::Preparation) ? "#4ade80" : "#f87171";
    m_phaseLabel->setText(QString("<span style='color:%1;font-weight:bold'>%2</span>")
                              .arg(phaseColor)
                              .arg(phase == synera::GamePhase::Preparation ? "准备阶段" : "战斗中"));

    m_waveLabel->setText(QString("<b>第 %1 波</b>").arg(m_gameManager.GetCurrentWave()));

    m_hpLabel->setText(QString("HP: <b style='color:%1'>%2</b>")
                           .arg(m_gameManager.GetPlayerHp() > 30 ? "#4ade80" : "#f87171")
                           .arg(m_gameManager.GetPlayerHp()));
    m_goldLabel->setText(QString("金币: <b style='color:#fbbf24'>%1</b>").arg(m_gameManager.GetGold()));

    // 更新羁绊显示
    {
        const auto& synergies = m_gameManager.GetActiveSynergies();
        if (synergies.empty())
        {
            m_synergyLabel->setText("未激活羁绊");
        }
        else
        {
            // 羁绊效果描述
            static const QMap<QString, QString> effectDesc = {
                {"人类", "ATK+"}, {"战士", "HP+"}, {"兽人", "ATK+"},
                {"精灵", "ATK+"}, {"远程", "射程+"}, {"法师", "ATK+"},
                {"骑士", "HP+"}, {"治疗", "全属性+"}, {"刺客", "ATK+"},
                {"侍从", "HP+"}
            };
            QStringList parts;
            for (auto& info : synergies)
            {
                QString traitName = QString::fromStdString(info.trait);
                QString desc = effectDesc.value(traitName, "");
                QString bonus = (info.activeThreshold >= 4) ? "Ⅱ" : "Ⅰ";
                parts << QString("%1(%2) %3%4").arg(traitName).arg(info.unitCount).arg(desc).arg(bonus);
            }
            m_synergyLabel->setText("⚡ " + parts.join("  "));
        }
    }

    // 战斗结果展示（基于时间戳，不受帧率影响）
    bool resultExpired = (m_combatResultEndTime > 0 && QDateTime::currentMSecsSinceEpoch() >= m_combatResultEndTime);
    if (resultExpired)
    {
        m_combatResultEndTime = 0;
        if (m_combatResultOverlay) m_combatResultOverlay->hide();
    }
    if (m_combatResultEndTime > 0)
    {
        m_tipLabel->setText(m_combatResultText);
    }

    // 战斗结果覆盖层（显示在棋盘中央）
    if (m_combatResultOverlay && m_combatResultOverlay->isVisible())
    {
        m_combatResultOverlay->move(
            m_boardWidget->width() / 2 - 140,
            m_boardWidget->height() / 2 - 45);
    }

    // 检测战斗结束（从 Combat 切换到 Preparation）
    if (prevPhase == synera::GamePhase::Combat && phase == synera::GamePhase::Preparation)
    {
        bool won = m_gameManager.GetLastCombatResult();
        int wave = m_gameManager.GetCurrentWave() - 1;  // 已经打完的波次

        // 检查是否游戏通关或结束
        bool gameWon = m_gameManager.IsGameWon();
        bool gameOver = m_gameManager.IsGameOver();

        if (gameWon)
        {
            // 游戏通关 — 显示通关字样，3 秒后自动关闭
            m_combatResultText = "🏆 恭喜通关！你击败了所有 15 波敌人！";
            m_tipLabel->setText(m_combatResultText);
            synera::AudioManager::PlayVictory();

            if (m_combatResultOverlay)
            {
                m_combatResultOverlay->setStyleSheet(
                    "background: rgba(0,0,0,200); color: #ffd700; font-size: 32px; font-weight: bold; "
                    "border: 3px solid #ffd700; border-radius: 15px; padding: 10px;");
                m_combatResultOverlay->setText("游 戏 通 关");
                m_combatResultOverlay->show();
            }
            // 3 秒后自动关闭游戏
            QTimer::singleShot(3000, qApp, &QApplication::quit);
        }
        else if (gameOver)
        {
            // 游戏结束 — 显示结束字样，3 秒后自动关闭
            m_combatResultText = "💀 游戏结束！点击窗口任意位置关闭";
            m_tipLabel->setText(m_combatResultText);
            synera::AudioManager::PlayDefeat();

            if (m_combatResultOverlay)
            {
                m_combatResultOverlay->setStyleSheet(
                    "background: rgba(0,0,0,200); color: #f87171; font-size: 32px; font-weight: bold; "
                    "border: 3px solid #f87171; border-radius: 15px; padding: 10px;");
                m_combatResultOverlay->setText("游 戏 结 束");
                m_combatResultOverlay->show();
            }
            // 3 秒后自动关闭游戏
            QTimer::singleShot(3000, qApp, &QApplication::quit);
        }
        else
        {
            // 普通胜负（第 1-14 波）
            int goldReward = wave * 4 + 5;
            int hpLoss = (wave + 1) * 4;  // 扣血 = 当前波数 × 4
            // 注意：wave 是已经打完的第 N 波，所以实际扣血是按 wave+1 算
            // 但在 OnCombatEnd 里 damage = m_currentWave * 4，当时 m_currentWave == wave+1
            // 所以显示的时候应该用 wave+1，跟实际扣血一致
            int actualWave = wave + 1;
            m_combatResultText = won
                ? QString("🎉 第 %1 波 胜利！获得 %2 金币").arg(actualWave).arg(goldReward)
                : QString("💀 第 %1 波 失败！损失 %2 生命值").arg(actualWave).arg(actualWave * 4);

            // 播放战斗结果音效
            if (won) synera::AudioManager::PlayVictory();
            else synera::AudioManager::PlayDefeat();

            // 设置棋盘中央覆盖层
            if (m_combatResultOverlay)
            {
                QString title = won ? "胜  利" : "失  败";
                QString color = won ? "#4ade80" : "#f87171";
                m_combatResultOverlay->setStyleSheet(QString(
                    "background: rgba(0,0,0,200); color: %1; font-size: 32px; font-weight: bold; "
                    "border: 3px solid %1; border-radius: 15px; padding: 10px;").arg(color));
                m_combatResultOverlay->setText(title);
                m_combatResultOverlay->show();
            }

            m_combatResultEndTime = QDateTime::currentMSecsSinceEpoch() + 3000;  // 显示 3 秒
        }
    }
    prevPhase = phase;

    // 操作提示（根据阶段和选中状态变化）
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
    {
        // 游戏已结束或通关，不覆盖结果提示
    }
    else if (phase == synera::GamePhase::Preparation)
    {
        if (m_combatResultEndTime > 0)
        {
            // 战斗结果优先显示
        }
        else if (m_dragInfo.active)
        {
            m_tipLabel->setText("💡 拖拽到棋盘空格部署 | 拖到备战区撤回 | 右键出售");
        }
        else if (m_shopPreviewUnit)
        {
            m_tipLabel->setText("💡 右侧面板查看单位详情，点击「确认购买」入手");
        }
        else
        {
            bool canMerge = m_gameManager.HasMergeableUnits();
            if (canMerge)
                m_tipLabel->setText("💡 检测到可合星单位！集齐 3 个同名同星自动合成升星 ⭐");
            else if (m_gameManager.GetCurrentWave() == 1 && m_gameManager.GetGold() <= 10)
                m_tipLabel->setText("💡 从备战区拖拽单位到棋盘部署 | 商店购买单位增强阵容");
            else
                m_tipLabel->setText("💡 拖拽备战区单位到棋盘部署 | 右键备战区出售 | 商店购买增强阵容");
        }

        // 备战区合星高亮
        m_benchWidget->update();
    }
    else
    {
        m_tipLabel->setText("⚔ 战斗中，请等待战斗结束...");
    }

    // 重绘 UI
    m_boardWidget->update();
    m_benchWidget->update();
    m_shopWidget->update();
    m_unitInfoWidget->update();
}

// ========== 布阵操作 ==========

void AutoChessDemo::OnBoardCellClicked(int row, int col)
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    // 拖拽中点击棋盘 → 尝试放置/移动
    if (m_dragInfo.active)
    {
        TryDeployFromDrag(row, col);
        return;
    }

    auto& board = m_gameManager.GetBoard();
    auto occupant = board.GetOccupant(synera::Position(row, col));

    if (occupant && occupant->GetOwner() == Owner::PlayerCtrl)
    {
        // 点击己方单位 → 从棋盘拖拽
        if (m_shopPreviewUnit)
        {
            m_shopPreviewUnit.reset();
            if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
        }

        m_dragInfo.active = true;
        m_dragInfo.source = DragInfo::Source::Board;
        m_dragInfo.boardRow = row;
        m_dragInfo.boardCol = col;
        m_dragInfo.unit = occupant;
        m_unitInfoWidget->ShowUnit(occupant);
        UpdateDragPreview(QCursor::pos());
        setCursor(Qt::ClosedHandCursor);
        m_boardWidget->SetDragZoneHighlight(true);
        m_boardWidget->SetDraggedUnitPosition(row, col);
    }
    else if (occupant)
    {
        // 点击敌方单位 → 显示详情
        m_unitInfoWidget->ShowUnit(occupant);
        if (m_shopPreviewUnit)
        {
            m_shopPreviewUnit.reset();
            if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
        }
    }
}

void AutoChessDemo::OnBenchSlotClicked(int index)
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    // 处理拖拽中点击备战区（拖拽判断必须放在 unit 判空前）
    if (m_dragInfo.active)
    {
        // 棋盘拖拽中点击备战区 → 撤回单位到备战区
        if (m_dragInfo.source == DragInfo::Source::Board)
        {
            if (m_gameManager.RecallUnit(m_dragInfo.boardRow, m_dragInfo.boardCol))
            {
                m_tipLabel->setText("↩ 单位已撤回备战区");
            }
        }
        CancelDrag();
        m_boardWidget->repaint();
        m_benchWidget->repaint();
        return;  // 不继续执行（不启动新拖拽）
    }

    auto& bench = m_gameManager.GetBench();
    auto unit = bench.GetUnit(static_cast<size_t>(index));
    if (!unit)
        return;
    if (m_shopPreviewUnit)
    {
        m_shopPreviewUnit.reset();
        if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
    }

    m_dragInfo.active = true;
    m_dragInfo.source = DragInfo::Source::Bench;
    m_dragInfo.slotIndex = index;
    m_dragInfo.unit = unit;
    m_benchWidget->SetSelectedSlot(index);
    m_unitInfoWidget->ShowUnit(unit);

    // 立即进入拖拽状态，无需等待鼠标移动
    UpdateDragPreview(QCursor::pos());
    setCursor(Qt::ClosedHandCursor);
    m_boardWidget->SetDragZoneHighlight(true);
}

void AutoChessDemo::OnBenchSellRequested(int index)
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    auto& bench = m_gameManager.GetBench();
    auto unit = bench.GetUnit(static_cast<size_t>(index));
    if (!unit)
        return;

    // 如果正在拖拽或预览该单位，取消
    if (m_dragInfo.active && m_dragInfo.unit == unit)
        CancelDrag();
    if (m_shopPreviewUnit == unit)
    {
        m_shopPreviewUnit.reset();
        if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
    }

    // 执行出售
    int goldBefore = m_gameManager.GetGold();
    m_gameManager.SellUnitFromBench(index);
    int goldEarned = m_gameManager.GetGold() - goldBefore;

    // 显示反馈提示
    m_tipLabel->setText(QString("💰 出售成功！获得 %1 金币").arg(goldEarned));
    m_unitInfoWidget->Clear();
}

// ========== 商店操作 ==========

void AutoChessDemo::OnShopUnitClicked(int index)
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    auto& shop = m_gameManager.GetShop();
    const auto& units = shop.GetShopUnits();
    if (index < 0 || index >= static_cast<int>(units.size()) || !units[index])
        return;

    // 显示商店单位预览（标记为商店预览，不显示出售按钮）
    m_shopPreviewUnit = units[index];
    m_unitInfoWidget->ShowUnit(m_shopPreviewUnit, true);

    // 显示确认购买按钮
    if (m_confirmPurchaseBtn)
        m_confirmPurchaseBtn->show();
}

void AutoChessDemo::OnConfirmPurchase()
{
    if (!m_shopPreviewUnit)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    // 从商店购买该单位
    auto& shop = m_gameManager.GetShop();
    const auto& units = shop.GetShopUnits();
    int foundIndex = -1;
    for (int i = 0; i < static_cast<int>(units.size()); ++i)
    {
        if (units[i] == m_shopPreviewUnit)
        {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex < 0)
    {
        // 单位已不在商店中（可能已刷新），重置预览
        m_shopPreviewUnit.reset();
        m_unitInfoWidget->Clear();
        if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
        return;
    }

    // 防御性检查：确认该槽位仍有单位（不为空）
    if (!units[foundIndex])
    {
        m_shopPreviewUnit.reset();
        m_unitInfoWidget->Clear();
        if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
        return;
    }

    // 检查金币是否充足
    const std::string& unitName = units[foundIndex]->GetName();
    int cost = 1;
    if (unitName == "法师" || unitName == "骑士" || unitName == "治疗师") cost = 2;
    else if (unitName == "刺客" || unitName == "狂战士" || unitName == "狙击手") cost = 3;

    if (m_gameManager.GetGold() < cost)
    {
        // 自定义美化弹窗
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("金币不足");
        msgBox.setText(QString("购买 %1 需要 %2 金币\n当前仅有 %3 金币")
            .arg(QString::fromStdString(unitName)).arg(cost).arg(m_gameManager.GetGold()));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #1e1c2e; color: #d0d0e0; }"
            "QMessageBox QLabel { color: #d0d0e0; font-size: 14px; padding: 12px; }"
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 #5a50a8, stop:1 #4a4080); color: white; border: none;"
            "    border-radius: 6px; padding: 8px 32px; font-size: 13px; font-weight: bold;"
            "    min-width: 80px; min-height: 28px; }"
            "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 #6a60c0, stop:1 #5a50a0); }");
        msgBox.exec();
        return;
    }

    m_gameManager.BuyUnitFromShop(foundIndex);
    m_shopWidget->RefreshDisplay();
    m_shopPreviewUnit.reset();
    m_unitInfoWidget->Clear();
    if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
}

// ========== 按钮事件 ==========

void AutoChessDemo::OnStartCombatClicked()
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;
    // 清除选中、高亮和商店预览
    if (m_dragInfo.active) CancelDrag();
    if (m_shopPreviewUnit)
    {
        m_shopPreviewUnit.reset();
        if (m_confirmPurchaseBtn) m_confirmPurchaseBtn->hide();
    }
    m_gameManager.StartCombatPhase();
}

void AutoChessDemo::OnRefreshShopClicked()
{
    if (m_gameManager.GetCurrentPhase() != synera::GamePhase::Preparation)
        return;
    if (m_gameManager.IsGameOver() || m_gameManager.IsGameWon())
        return;

    auto& shop = m_gameManager.GetShop();
    int cost = shop.GetRefreshCost();
    if (cost > 0)
    {
        if (m_gameManager.GetGold() < cost)
        {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("金币不足");
            int refreshNum = shop.GetRefreshCount() + 1;  // 本次是第几次刷新
            msgBox.setText(QString("第 %1 次刷新需要 %2 金币\n当前仅有 %3 金币")
                .arg(refreshNum).arg(cost).arg(m_gameManager.GetGold()));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { background-color: #1e1c2e; color: #d0d0e0; }"
                "QMessageBox QLabel { color: #d0d0e0; font-size: 14px; padding: 12px; }"
                "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                "    stop:0 #5a50a8, stop:1 #4a4080); color: white; border: none;"
                "    border-radius: 6px; padding: 8px 32px; font-size: 13px; font-weight: bold;"
                "    min-width: 80px; min-height: 28px; }"
                "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                "    stop:0 #6a60c0, stop:1 #5a50a0); }");
            msgBox.exec();
            return;
        }
        m_gameManager.SpendGold(cost);
    }
    shop.RecordRefresh();

    // 根据波次计算商店等级
    int shopLevel = std::min(9, 1 + m_gameManager.GetCurrentWave() / 2);
    shop.Refresh(shopLevel);
    m_shopWidget->RefreshDisplay();
}

// ========== 拖拽预览 ==========

void AutoChessDemo::UpdateDragPreview(const QPoint& globalPos)
{
    if (m_dragInfo.slotIndex < 0 || !m_dragInfo.unit)
    {
        HideDragPreview();
        return;
    }

    if (!m_dragPreview)
    {
        m_dragPreview = new QLabel(this);
        m_dragPreview->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_dragPreview->setStyleSheet("background: transparent;");
        m_dragPreview->resize(96, 96);
        m_dragPreview->show();
    }

    // 获取单位贴图
    auto it = [&]() -> QString {
        const auto& name = m_dragInfo.unit->GetName();
        QMap<std::string, QString> texMap = {
            {"步兵", "warrior"}, {"弓箭手", "archer"}, {"法师", "mage"},
            {"治疗师", "healer"}, {"骑士", "knight"}, {"刺客", "assassin"},
            {"狂战士", "tank"}, {"狙击手", "archer"}, {"侍从", "knight"},
            {"Boss", "boss"}
        };
        auto iter = texMap.find(name);
        if (iter != texMap.end())
            return QString(":/AutoChessDemo/assets/units/%1.png").arg(iter.value());
        return ":/AutoChessDemo/assets/units/warrior.png";
    }();

    QPixmap px(it);
    const int previewSize = 96;
    QPixmap bg(previewSize, previewSize);
    bg.fill(Qt::transparent);
    {
        QPainter p(&bg);
        p.setRenderHint(QPainter::Antialiasing);

        // 判断当前悬停位置是否有效
        QPoint boardLocal = m_boardWidget->mapFromGlobal(globalPos);
        bool overBoard = m_boardWidget->rect().contains(boardLocal);
        bool validSpot = false;
        if (overBoard)
        {
            int cellW = m_boardWidget->width() / m_gameManager.GetBoard().GetCols();
            int cellH = m_boardWidget->height() / m_gameManager.GetBoard().GetRows();
            int col = boardLocal.x() / cellW;
            int row = boardLocal.y() / cellH;
            Position pos(row, col);
            validSpot = m_gameManager.GetBoard().IsInBounds(pos) &&
                        !m_gameManager.GetBoard().IsOccupied(pos) &&
                        m_gameManager.GetBoard().IsPlayerHalf(pos);
        }

        // 外发光边框
        QColor borderColor = validSpot ? QColor(100, 255, 100) : QColor(255, 80, 80);
        p.setBrush(QColor(255, 255, 255, 30));
        p.setPen(QPen(borderColor, overBoard ? 3 : 2));
        p.drawRoundedRect(6, 6, previewSize - 12, previewSize - 12, 10, 10);

        // 放置有效性图标
        if (overBoard)
        {
            QFont iconFont = p.font();
            iconFont.setPointSize(16);
            iconFont.setBold(true);
            p.setFont(iconFont);
            p.setPen(validSpot ? QColor(100, 255, 100, 200) : QColor(255, 80, 80, 200));
            p.drawText(6, 6, previewSize - 12, previewSize - 12,
                       Qt::AlignBottom | Qt::AlignRight,
                       validSpot ? "✓" : "✗");
        }

        // 绘制单位贴图
        if (!px.isNull())
        {
            int s = 64;
            QPixmap scaled = px.scaled(s, s, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int sx = (previewSize - scaled.width()) / 2;
            int sy = (previewSize - scaled.height()) / 2 - 2;
            p.drawPixmap(sx, sy, scaled);
        }

        // 星级标签
        int starLevel = static_cast<int>(m_dragInfo.unit->GetStarLevel());
        if (starLevel > 1)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 215, 0, 200));
            QFont sf = p.font();
            sf.setPointSize(8);
            sf.setBold(true);
            p.setFont(sf);
            QRect badge(previewSize - 28, 6, 22, 14);
            p.drawRoundedRect(badge, 3, 3);
            p.setPen(QColor(40, 30, 0));
            p.drawText(badge, Qt::AlignCenter, QString("★%1").arg(starLevel));
        }
    }

    m_dragPreview->setPixmap(bg);
    m_dragPreview->resize(bg.size());

    // 预览跟随鼠标，光标居中
    QPoint localPos = mapFromGlobal(globalPos);
    m_dragPreview->move(localPos.x() - previewSize / 2, localPos.y() - previewSize / 2);
}

void AutoChessDemo::UpdateBoardHover(const QPoint& globalPos)
{
    if (!m_dragInfo.active) return;

    QPoint boardLocal = m_boardWidget->mapFromGlobal(globalPos);
    if (!m_boardWidget->rect().contains(boardLocal))
    {
        m_boardWidget->ClearHighlights();
        return;
    }

    int cellW = m_boardWidget->width() / m_gameManager.GetBoard().GetCols();
    int cellH = m_boardWidget->height() / m_gameManager.GetBoard().GetRows();
    int col = boardLocal.x() / cellW;
    int row = boardLocal.y() / cellH;

    Position hovered(row, col);
    if (m_gameManager.GetBoard().IsInBounds(hovered) &&
        !m_gameManager.GetBoard().IsOccupied(hovered) &&
        m_gameManager.GetBoard().IsPlayerHalf(hovered))
    {
        // 有效放置位置：绿色高亮
        m_boardWidget->ClearHighlights();
        m_boardWidget->SetHighlight({ hovered }, QColor(100, 200, 100, 60));
    }
    else if (m_gameManager.GetBoard().IsInBounds(hovered))
    {
        // 无效位置（被占/敌方半场）：红色高亮
        m_boardWidget->ClearHighlights();
        m_boardWidget->SetHighlight({ hovered }, QColor(255, 80, 80, 60));
    }
    else
    {
        m_boardWidget->ClearHighlights();
    }
}

bool AutoChessDemo::TryDeployFromDrag(int row, int col)
{
    if (!m_dragInfo.active || !m_dragInfo.unit)
    {
        CancelDrag();
        return false;
    }

    switch (m_dragInfo.source)
    {
    case DragInfo::Source::Bench:
    {
        auto& bench = m_gameManager.GetBench();
        auto unit = bench.GetUnit(static_cast<size_t>(m_dragInfo.slotIndex));
        if (!unit || unit != m_dragInfo.unit)
        {
            CancelDrag();
            return false;
        }
        if (m_gameManager.DeployUnit(unit, row, col))
        {
            CancelDrag();
            return true;
        }
        return false;
    }
    case DragInfo::Source::Board:
    {
        if (m_gameManager.MoveUnitOnBoard(m_dragInfo.boardRow, m_dragInfo.boardCol, row, col))
        {
            CancelDrag();
            return true;
        }
        return false;
    }
    default:
        CancelDrag();
        return false;
    }
}

void AutoChessDemo::CancelDrag()
{
    m_boardWidget->ClearDraggedUnitPosition();
    m_boardWidget->SetDragZoneHighlight(false);
    m_boardWidget->ClearHighlights();
    m_benchWidget->ClearSelectedSlot();
    HideDragPreview();
    m_dragInfo = DragInfo{};
    setCursor(Qt::ArrowCursor);
}

void AutoChessDemo::HideDragPreview()
{
    if (m_dragPreview)
    {
        m_boardWidget->ClearHighlights();
        m_dragPreview->hide();
        m_dragPreview->deleteLater();
        m_dragPreview = nullptr;
    }
}

// ========== 事件过滤器（拖拽处理） ==========

bool AutoChessDemo::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        QPoint globalPos = me->globalPosition().toPoint();

        if (m_dragInfo.active)
        {
            UpdateDragPreview(globalPos);
            UpdateBoardHover(globalPos);
            m_boardWidget->SetDragZoneHighlight(true);
            setCursor(Qt::ClosedHandCursor);
        }
        // 不消耗事件，让子控件继续处理（如 bench 悬停高亮）
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton) return false;

        // 备战区按下由 BenchWidget 自行处理（触发拖拽启动）
        if (obj == m_benchWidget)
            return false;

        // 拖拽中点击棋盘 → 允许 OnBoardCellClicked 处理放置/移动
        if (m_dragInfo.active && obj == m_boardWidget)
            return false;

        // 拖拽中点击其他地方 → 取消
        if (m_dragInfo.active)
        {
            CancelDrag();
            return true;
        }
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !m_dragInfo.active)
            return false;

        QPoint globalPos = me->globalPosition().toPoint();

        // 拖拽释放：检查是否在棋盘上方
        QPoint boardLocal = m_boardWidget->mapFromGlobal(globalPos);
        if (m_boardWidget->rect().contains(boardLocal))
        {
            int cellW = m_boardWidget->width() / m_gameManager.GetBoard().GetCols();
            int cellH = m_boardWidget->height() / m_gameManager.GetBoard().GetRows();
            int col = boardLocal.x() / cellW;
            int row = boardLocal.y() / cellH;
            if (!TryDeployFromDrag(row, col))
                CancelDrag();  // 部署失败（敌方半场/被占）→ 立即取消
        }
        else
        {
            // 棋盘拖拽释放在备战区 → 撤回单位
            if (m_dragInfo.source == DragInfo::Source::Board)
            {
                QPoint benchLocal = m_benchWidget->mapFromGlobal(globalPos);
                if (m_benchWidget->rect().contains(benchLocal))
                {
                    if (m_gameManager.RecallUnit(m_dragInfo.boardRow, m_dragInfo.boardCol))
                    {
                        CancelDrag();
                        m_boardWidget->repaint();
                        m_benchWidget->repaint();
                        m_tipLabel->setText("↩ 单位已撤回备战区");
                        return true;
                    }
                }
            }
            CancelDrag();
        }

        setCursor(Qt::ArrowCursor);
        return false;
    }

    return QMainWindow::eventFilter(obj, event);
}
