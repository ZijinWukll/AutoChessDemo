#include "BoardWidget.h"
#include "GameCore/GameConfig.h"

#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QMap>
#include <QFont>

namespace synera
{
    static QString GetUnitTexture(const std::string& unitName, const QString& suffix = "")
    {
        QMap<std::string, QString> texMap = {
            {"步兵", "warrior"}, {"弓箭手", "archer"}, {"法师", "mage"},
            {"治疗师", "healer"}, {"骑士", "knight"}, {"刺客", "assassin"},
            {"狂战士", "tank"}, {"狙击手", "sniper"}, {"侍从", "squire"},
            {"Boss", "boss"}
        };
        if (unitName == "Soldier" || unitName == "步兵") texMap[unitName] = "warrior";
        if (unitName == "Archer" || unitName == "弓箭手") texMap[unitName] = "archer";
        if (unitName == "Mage" || unitName == "法师") texMap[unitName] = "mage";
        if (unitName == "Healer" || unitName == "治疗师") texMap[unitName] = "healer";
        if (unitName == "Knight" || unitName == "骑士") texMap[unitName] = "knight";
        if (unitName == "Squire" || unitName == "侍从") texMap[unitName] = "squire";
        if (unitName == "Assassin" || unitName == "刺客") texMap[unitName] = "assassin";
        if (unitName == "Berserker" || unitName == "狂战士") texMap[unitName] = "tank";
        if (unitName == "Sniper" || unitName == "狙击手") texMap[unitName] = "sniper";

        auto it = texMap.find(unitName);
        if (it != texMap.end())
            return QString(":/AutoChessDemo/assets/units/%1%2.png").arg(it.value()).arg(suffix);
        return QString(":/AutoChessDemo/assets/units/warrior%1.png").arg(suffix);
    }

    BoardWidget::BoardWidget(GameManager& gameManager, QWidget* parent)
        : QWidget(parent)
        , m_gameManager(gameManager)
    {
        setMinimumSize(400, 400);
        setMouseTracking(false);
    }

    void BoardWidget::SetHighlight(const std::vector<Position>& positions, const QColor& color)
    {
        for (const auto& pos : positions)
            m_highlights.emplace_back(pos, color);
        update();
    }

    void BoardWidget::ClearHighlights()
    {
        m_highlights.clear();
        m_dragZoneVisible = false;
        update();
    }

    void BoardWidget::SetDragZoneHighlight(bool visible)
    {
        m_dragZoneVisible = visible;
        update();
    }

    void BoardWidget::SetDraggedUnitPosition(int row, int col)
    {
        m_draggedUnitRow = row;
        m_draggedUnitCol = col;
        update();
    }

    void BoardWidget::ClearDraggedUnitPosition()
    {
        m_draggedUnitRow = -1;
        m_draggedUnitCol = -1;
        update();
    }

    QRect BoardWidget::GetCellRect(int row, int col) const
    {
        const int rows = m_gameManager.GetBoard().GetRows();
        const int cols = m_gameManager.GetBoard().GetCols();
        const int cellW = width() / cols;
        const int cellH = height() / rows;
        return QRect(col * cellW, row * cellH, cellW, cellH);
    }

    QPoint BoardWidget::GetCellScreenPos(int row, int col) const
    {
        QRect r = GetCellRect(row, col);
        return r.center();
    }

    // ========== 动画 ==========

    void BoardWidget::StartDeployAnimation(std::shared_ptr<Unit> unit, QPointF fromScreenPos)
    {
        int r = unit->GetGridRow();
        int c = unit->GetGridCol();
        if (r < 0 || c < 0) return;

        QPointF toPos = QPointF(GetCellScreenPos(r, c));
        // 将 fromScreenPos 从全局坐标转为当前控件坐标
        fromScreenPos = mapFromGlobal(QPoint(static_cast<int>(fromScreenPos.x()),
                                              static_cast<int>(fromScreenPos.y())));
        m_deployAnim = {true, fromScreenPos, toPos, 0.0f, unit};
    }

    void BoardWidget::UpdateAnimations()
    {
        const float animSpeed = 0.05f;    // 部署动画速度（100fps ≈ 0.05/frame ≈ 200ms）
        bool needUpdate = false;

        if (m_deployAnim.active)
        {
            m_deployAnim.progress += animSpeed;
            if (m_deployAnim.progress >= 1.0f)
            {
                m_deployAnim.progress = 1.0f;
                m_deployAnim.active = false;
            }
            needUpdate = true;
        }

        // 移动动画
        for (auto it = m_moveAnims.begin(); it != m_moveAnims.end(); )
        {
            it->progress += it->speed;
            if (it->progress >= 1.0f)
            {
                it = m_moveAnims.erase(it);
            }
            else
            {
                ++it;
            }
            needUpdate = true;
        }

        // ---- 战斗攻击特效 ----
        // 从 GameManager 读取本帧攻击事件 → 生成闪光效果
        const auto& events = m_gameManager.GetRecentAttackEvents();
        if (!events.empty())
        {
            int cellW = width() / m_gameManager.GetBoard().GetCols();
            int cellH = height() / m_gameManager.GetBoard().GetRows();
            float maxRadius = qMin(cellW, cellH) * 0.7f;

            for (auto& evt : events)
            {
                // 攻击方闪光（位置：攻击者格子中心）
                QPointF atkPos = QPointF(
                    evt.attackerCol * cellW + cellW / 2.0f,
                    evt.attackerRow * cellH + cellH / 2.0f
                );
                m_attackFlashes.push_back({
                    atkPos, 0.0f,
                    evt.isPlayerAttacker ? QColor(80, 160, 255) : QColor(255, 100, 100),
                    false, maxRadius
                });

                // 受击方红色闪光（位置：目标格子中心）
                QPointF hitPos = QPointF(
                    evt.targetCol * cellW + cellW / 2.0f,
                    evt.targetRow * cellH + cellH / 2.0f
                );
                m_attackFlashes.push_back({
                    hitPos, 0.0f, QColor(255, 60, 60), true, maxRadius * 0.6f
                });
            }
            needUpdate = true;
        }

        // 闪光特效更新（快速淡出）
        const float flashFadeSpeed = 0.12f;  // ≈ 8 帧 (80ms) 全部淡出
        for (auto it = m_attackFlashes.begin(); it != m_attackFlashes.end(); )
        {
            it->progress += flashFadeSpeed;
            if (it->progress >= 1.0f)
                it = m_attackFlashes.erase(it);
            else
                ++it;
        }

        if (needUpdate)
            update();
    }

    // ========== 绘制 ==========

    void BoardWidget::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        const Board& board = m_gameManager.GetBoard();
        const int rows = board.GetRows();
        const int cols = board.GetCols();
        const int cellW = width() / cols;
        const int cellH = height() / rows;

        // 缓存格子贴图 — 暗黑魔法石板风格
        static QPixmap cellLight, cellDark;
        if (cellLight.isNull())
        {
            auto makeCell = [](const QColor& c1, const QColor& c2, const QColor& glow) {
                QPixmap px(64, 64);
                px.fill(Qt::transparent);
                QPainter cp(&px);
                cp.setRenderHint(QPainter::Antialiasing);
                QLinearGradient grad(0, 0, 64, 64);
                grad.setColorAt(0, c1);
                grad.setColorAt(1, c2);
                cp.setBrush(grad);
                cp.setPen(Qt::NoPen);
                cp.drawRoundedRect(1, 1, 62, 62, 4, 4);
                // 内发光
                cp.setBrush(glow);
                cp.drawRoundedRect(2, 2, 60, 60, 3, 3);
                return px;
            };
            cellLight = makeCell(QColor(44, 39, 60), QColor(38, 33, 54), QColor(160, 140, 255, 8));
            cellDark  = makeCell(QColor(37, 32, 52), QColor(31, 26, 46), QColor(100, 80, 200, 5));
        }

        // 缓存高亮贴图 — 暗黑魔法主题
        static QPixmap hlSelect, hlMove, hlAttack;
        if (hlSelect.isNull())
        {
            auto makeHL = [](const QColor& fill, const QColor& border, const QColor& glow) {
                QPixmap px(64, 64);
                px.fill(Qt::transparent);
                QPainter p(&px);
                p.setRenderHint(QPainter::Antialiasing);
                // 外发光层
                p.setPen(QPen(glow, 5));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(2, 2, 60, 60, 6, 6);
                // 主边框
                p.setPen(QPen(border, 2));
                p.setBrush(fill);
                p.drawRoundedRect(4, 4, 56, 56, 5, 5);
                return px;
            };
            hlSelect = makeHL(QColor(80, 160, 255, 45),  QColor(80, 160, 255, 200),    QColor(80, 160, 255, 35));
            hlMove   = makeHL(QColor(60, 200, 255, 35), QColor(60, 200, 255, 160),   QColor(60, 200, 255, 25));
            hlAttack = makeHL(QColor(220, 60, 120, 35),  QColor(220, 60, 120, 160),   QColor(220, 60, 120, 25));
        }

        int halfRow = rows / 2; // 上半=敌方，下半=我方

        // ===== 第1层：格子背景 =====
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
            {
                QRect cellRect(c * cellW, r * cellH, cellW, cellH);
                QPixmap cellTex = ((r + c) % 2 == 0) ? cellLight : cellDark;
                painter.drawPixmap(cellRect, cellTex);
            }

        // ===== 第2层：半场色调（渐变覆盖） =====
        QRect enemyHalf(0, 0, width(), halfRow * cellH);
        QRect playerHalf(0, halfRow * cellH, width(), height() - halfRow * cellH);
        // 敌方半场（上半）— 暗紫魔法能量
        {
            QLinearGradient enemyGrad(0, 0, 0, halfRow * cellH);
            enemyGrad.setColorAt(0, QColor(180, 60, 220, 30));
            enemyGrad.setColorAt(0.4, QColor(140, 50, 200, 18));
            enemyGrad.setColorAt(1, QColor(100, 40, 180, 6));
            painter.fillRect(enemyHalf, enemyGrad);
        }
        // 我方半场（下半）— 深蓝奥术能量
        {
            QLinearGradient playerGrad(0, halfRow * cellH, 0, height());
            playerGrad.setColorAt(0, QColor(40, 80, 220, 6));
            playerGrad.setColorAt(0.6, QColor(50, 100, 230, 18));
            playerGrad.setColorAt(1, QColor(60, 110, 240, 30));
            painter.fillRect(playerHalf, playerGrad);
        }

        // ===== 第3层：中线分隔 — 能量裂隙效果 =====
        {
            int midY = halfRow * cellH;
            // 外层宽光晕（紫）
            painter.setPen(QPen(QColor(140, 60, 200, 22), 10));
            painter.drawLine(4, midY, width() - 4, midY);
            // 中层光晕（蓝紫）
            painter.setPen(QPen(QColor(80, 80, 220, 40), 4));
            painter.drawLine(6, midY, width() - 6, midY);
            // 核心线（亮蓝白）
            painter.setPen(QPen(QColor(140, 140, 255, 100), 1, Qt::DashLine));
            painter.drawLine(10, midY, width() - 10, midY);
            // 端点符文 — 发光圆点
            painter.setBrush(QColor(150, 120, 255, 150));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(6, midY), 4, 4);
            painter.drawEllipse(QPointF(width() - 6, midY), 4, 4);
            // 内核光点
            painter.setBrush(QColor(210, 200, 255, 200));
            painter.drawEllipse(QPointF(6, midY), 2, 2);
            painter.drawEllipse(QPointF(width() - 6, midY), 2, 2);
        }

        // ===== 第4层：拖拽放置区指示（已精简 — 交由第7层高亮系统处理单个格子） =====
        // 拖拽时不再覆盖整个半场，只由 UpdateBoardHover 设置单个格子的高亮
        // 保留 m_dragZoneVisible 标志控制是否渲染其他拖拽相关元素
        if (m_dragZoneVisible)
        {
            // 空操作：单个格子的蓝光由第7层高亮绘制
        }

        // ===== 第5层：半场标签 =====
        {
            QFont labelFont = painter.font();
            labelFont.setPointSize(9);
            labelFont.setBold(true);
            painter.setFont(labelFont);

            // 敌方标签 (左上) — 暗紫徽章
            {
                QRect tagRect(6, 6, 72, 20);
                painter.setBrush(QColor(140, 50, 200, 70));
                painter.setPen(QPen(QColor(180, 100, 240, 100), 1));
                painter.drawRoundedRect(tagRect, 5, 5);
                painter.setPen(QColor(200, 160, 255, 220));
                painter.drawText(tagRect, Qt::AlignCenter, "⚔ 敌方");
            }

            // 我方标签 (左下) — 蓝晶徽章
            {
                QRect tagRect(6, height() - 26, 72, 20);
                painter.setBrush(QColor(40, 80, 200, 70));
                painter.setPen(QPen(QColor(80, 140, 255, 100), 1));
                painter.drawRoundedRect(tagRect, 5, 5);
                painter.setPen(QColor(160, 200, 255, 220));
                painter.drawText(tagRect, Qt::AlignCenter, "🛡 我方");
            }
        }

        // ===== 第6层：网格线（极暗，衬托魔法感） =====
        {
            painter.setPen(QPen(QColor(60, 55, 80, 25), 1));
            for (int r = 0; r <= rows; ++r)
            {
                int y = r * cellH;
                painter.drawLine(0, y, width(), y);
            }
            for (int c = 0; c <= cols; ++c)
            {
                int x = c * cellW;
                painter.drawLine(x, 0, x, height());
            }
        }

        // ===== 第7层：高亮 =====
        for (const auto& [hlPos, hlColor] : m_highlights)
        {
            QRect cellRect(hlPos.y * cellW, hlPos.x * cellH, cellW, cellH);
            QPixmap* hlTex = &hlMove;
            if (hlColor == QColor(100, 200, 100, 60)) hlTex = &hlMove;
            else if (hlColor == QColor(255, 80, 80, 60)) hlTex = &hlAttack;
            painter.drawPixmap(cellRect, *hlTex);
        }

        // ===== 第8层：绘制已部署的单位 =====
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
            {
                Position pos(r, c);
                auto unit = board.GetOccupant(pos);
                if (!unit || !unit->IsAlive())
                    continue;

                // 如果该单位正在播放部署动画，跳过（由动画层绘制）
                if (m_deployAnim.active && m_deployAnim.unit == unit)
                    continue;

                // 跳过移动动画中的单位
                bool inMoveAnim = false;
                for (auto& ma : m_moveAnims)
                    if (ma.unit == unit) { inMoveAnim = true; break; }
                if (inMoveAnim) continue;

                QRect cellRect(c * cellW, r * cellH, cellW, cellH);
                DrawUnitAt(painter, unit, cellRect, cellW, cellH);
            }

        // ===== 第9层：动画中的单位 =====
        // 部署动画
        if (m_deployAnim.active && m_deployAnim.unit)
        {
            float t = m_deployAnim.progress;
            // ease-out 缓动
            float easeT = 1.0f - (1.0f - t) * (1.0f - t);
            QPointF pos = m_deployAnim.fromPos + (m_deployAnim.toPos - m_deployAnim.fromPos) * easeT;
            int s = qMin(cellW, cellH) - 6;
            QRectF drawRect(pos.x() - s/2.0f, pos.y() - s/2.0f, s, s);
            QRect cellRect(static_cast<int>(drawRect.x()), static_cast<int>(drawRect.y()),
                           static_cast<int>(drawRect.width()), static_cast<int>(drawRect.height()));
            DrawUnitAt(painter, m_deployAnim.unit, cellRect, s, s);
        }

        // 移动动画
        for (auto& ma : m_moveAnims)
        {
            if (!ma.unit) continue;
            float t = ma.progress;
            float easeT = t; // linear
            QPointF pos = ma.fromPos + (ma.toPos - ma.fromPos) * easeT;
            int s = qMin(cellW, cellH) - 6;
            QRectF drawRect(pos.x() - s/2.0f, pos.y() - s/2.0f, s, s);
            QRect cellRect(static_cast<int>(drawRect.x()), static_cast<int>(drawRect.y()),
                           static_cast<int>(drawRect.width()), static_cast<int>(drawRect.height()));
            DrawUnitAt(painter, ma.unit, cellRect, s, s);
        }

        // ===== 第10层：攻击闪光特效 =====
        for (auto& flash : m_attackFlashes)
        {
            float alpha = 1.0f - flash.progress;
            float radius = flash.maxRadius * (0.3f + flash.progress * 0.7f);
            QColor col = flash.color;
            col.setAlpha(static_cast<int>(80 * alpha));

            if (flash.isHit)
            {
                // 受击：红色十字闪光
                int crossLen = static_cast<int>(radius * 0.6f);
                QPointF c = flash.pos;
                painter.setPen(QPen(QColor(255, 60, 60, static_cast<int>(120 * alpha)), 3));
                painter.drawLine(QPointF(c.x() - crossLen, c.y()), QPointF(c.x() + crossLen, c.y()));
                painter.drawLine(QPointF(c.x(), c.y() - crossLen), QPointF(c.x(), c.y() + crossLen));
                // 外圈光环
                painter.setPen(QPen(QColor(255, 80, 80, static_cast<int>(60 * alpha)), 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(c, radius * 0.5f, radius * 0.5f);
            }
            else
            {
                // 攻击方：辐射光爆
                QRadialGradient grad(flash.pos, radius);
                QColor inner = col;
                inner.setAlpha(static_cast<int>(60 * alpha));
                grad.setColorAt(0, inner);
                QColor outer = col;
                outer.setAlpha(static_cast<int>(15 * alpha));
                grad.setColorAt(0.6f, outer);
                grad.setColorAt(1, QColor(0, 0, 0, 0));
                painter.setBrush(grad);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(flash.pos, radius, radius);

                // 中心光点
                painter.setBrush(QColor(255, 255, 255, static_cast<int>(100 * alpha)));
                painter.drawEllipse(flash.pos, radius * 0.12f, radius * 0.12f);
            }
        }

        // ===== 第11层：部署空位提示（备战阶段） =====
        if (m_gameManager.GetCurrentPhase() == GamePhase::Preparation)
        {
            painter.setPen(QPen(QColor(60, 160, 255, 100), 2));
            for (int r = halfRow; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                {
                    Position pos(r, c);
                    if (!board.IsOccupied(pos))
                    {
                        QRect cr(c * cellW, r * cellH, cellW, cellH);
                        painter.setBrush(QColor(50, 140, 255, 30));
                        painter.drawRoundedRect(cr.adjusted(3, 3, -3, -3), 6, 6);
                        // 角落装饰十字
                        painter.setPen(QPen(QColor(60, 160, 255, 60), 1));
                        int m = 8;
                        int cx = cr.center().x(), cy = cr.center().y();
                        painter.drawLine(cx - m, cy, cx + m, cy);
                        painter.drawLine(cx, cy - m, cx, cy + m);
                    }
                }
        }
    }

    void BoardWidget::DrawUnitAt(QPainter& painter, std::shared_ptr<Unit> unit,
                                  const QRect& cellRect, int cellW, int cellH)
    {
        // 棋盘拖拽：被拖的单位半透明
        bool isDragged = (unit->GetGridRow() == m_draggedUnitRow && unit->GetGridCol() == m_draggedUnitCol);
        if (isDragged)
            painter.setOpacity(0.35f);

        int margin = 1;
        QRect uRect = cellRect.adjusted(margin, margin, -margin, -margin);
        int cellSize = qMin(uRect.width(), uRect.height());

        // 加载 sprite
        QString texPath = GetUnitTexture(unit->GetName());
        QPixmap unitTex(texPath);
        if (!unitTex.isNull())
        {
            // 棋盘区域：2x 放大取上半部分（头部特写），裁剪到格子内
            QPixmap big = unitTex.scaled(cellSize * 2, cellSize * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap scaled = big.copy(0, 0, big.width(), big.height() / 2);

            if (unit->GetOwner() == Owner::EnemyCtrl)
            {
                QTransform tf;
                tf.scale(1, -1);
                scaled = scaled.transformed(tf);
            }

            int x = uRect.center().x() - scaled.width() / 2;
            int y = uRect.center().y() - scaled.height() / 2;

            // 裁剪到格子范围，防止放大溢出到相邻格
            painter.save();
            painter.setClipRect(cellRect);
            painter.drawPixmap(x, y, scaled);
            painter.restore();
        }
        else
        {
            QColor unitColor = (unit->GetOwner() == Owner::PlayerCtrl)
                               ? QColor(60, 140, 255, 200)
                               : QColor(180, 60, 200, 200);
            painter.setBrush(unitColor);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(uRect);
        }

        // 星级装饰：QPainter 绘制边框+星标，避免贴图遮挡角色头像
        int starLevel = static_cast<int>(unit->GetStarLevel());
        if (starLevel > 1)
        {
            QColor ringColor = (starLevel == 2)
                ? QColor(255, 215, 0, 150)    // 2★ = 金色
                : QColor(200, 130, 255, 180); // 3★ = 紫金
            painter.setPen(QPen(ringColor, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(uRect.adjusted(2, 2, -2, -2), 8, 8);

            QString starStr;
            for (int i = 0; i < starLevel; ++i) starStr += "★";
            painter.setPen(ringColor.lighter(130));
            QFont sf = painter.font();
            sf.setPointSize(10);
            sf.setBold(true);
            painter.setFont(sf);
            painter.drawText(uRect.adjusted(2, 2, -4, -2), Qt::AlignTop | Qt::AlignRight, starStr);
        }

        // 星级小标签（放在单位右下角）
        if (starLevel > 1)
        {
            QString starLabel = QString("★%1").arg(starLevel);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 215, 0, 200));
            QFont sf = painter.font();
            sf.setPointSize(7);
            sf.setBold(true);
            painter.setFont(sf);
            QRect badgeRect(uRect.right() - 22, uRect.bottom() - 14, 20, 12);
            painter.drawRoundedRect(badgeRect, 3, 3);
            painter.setPen(QColor(40, 30, 0));
            painter.drawText(badgeRect, Qt::AlignCenter, starLabel);
        }

        // 血条
        if (unit->GetMaxHp() > 0)
        {
            float hpRatio = static_cast<float>(unit->GetHp()) / unit->GetMaxHp();
            int barW = uRect.width() - 6;
            int barH = 4;
            int barX = uRect.center().x() - barW / 2;
            int barY = uRect.bottom() - barH - 1;

            painter.setBrush(QColor(35, 30, 40, 200));
            painter.setPen(Qt::NoPen);
            painter.drawRect(barX, barY, barW, barH);

            QColor hpColor = (hpRatio > 0.5f) ? QColor(50, 220, 50) :
                             (hpRatio > 0.25f) ? QColor(230, 200, 30) : QColor(230, 40, 40);
            painter.setBrush(hpColor);
            painter.drawRect(barX, barY, static_cast<int>(barW * hpRatio), barH);
        }

        // 法力条
        if (unit->GetMaxMana() > 0)
        {
            float manaRatio = static_cast<float>(unit->GetMana()) / unit->GetMaxMana();
            int barW = uRect.width() - 6;
            int barH = 3;
            int barX = uRect.center().x() - barW / 2;
            int barY = uRect.bottom() - barH - 6;

            painter.setBrush(QColor(25, 20, 60, 180));
            painter.setPen(Qt::NoPen);
            painter.drawRect(barX, barY, barW, barH);

            QLinearGradient manaGrad(barX, 0, barX + barW, 0);
            manaGrad.setColorAt(0, QColor(60, 140, 255));
            manaGrad.setColorAt(1, QColor(100, 180, 255));
            painter.setBrush(manaGrad);
            painter.drawRect(barX, barY, static_cast<int>(barW * manaRatio), barH);
        }

        // 名称
        if (cellSize > 30)
        {
            painter.setPen(QColor(255, 255, 255, 200));
            QFont nf = painter.font();
            nf.setPointSize(7);
            nf.setBold(true);
            painter.setFont(nf);
            QString nameAbbr = QString::fromStdString(unit->GetName());
            if (nameAbbr.length() > 4) nameAbbr = nameAbbr.left(4);
            painter.drawText(uRect.adjusted(2, 2, -2, -2),
                             Qt::AlignTop | Qt::AlignLeft, nameAbbr);
        }

        if (isDragged)
            painter.setOpacity(1.0f);
    }

    void BoardWidget::mousePressEvent(QMouseEvent* event)
    {
        const int cols = m_gameManager.GetBoard().GetCols();
        const int rows = m_gameManager.GetBoard().GetRows();
        const int cellW = width() / cols;
        const int cellH = height() / rows;

        int col = event->pos().x() / cellW;
        int row = event->pos().y() / cellH;

        if (row >= 0 && row < rows && col >= 0 && col < cols)
        {
            emit CellClicked(row, col);
        }
    }

    void BoardWidget::mouseMoveEvent(QMouseEvent* event)
    {
        const int cols = m_gameManager.GetBoard().GetCols();
        const int rows = m_gameManager.GetBoard().GetRows();
        const int cellW = width() / cols;
        const int cellH = height() / rows;

        int col = event->pos().x() / cellW;
        int row = event->pos().y() / cellH;

        // 仅在悬停格子变化时更新 tooltip，避免闪烁
        if (row != m_lastHoverRow || col != m_lastHoverCol)
        {
            m_lastHoverRow = row;
            m_lastHoverCol = col;

            if (row >= 0 && row < rows && col >= 0 && col < cols)
            {
                auto unit = m_gameManager.GetBoard().GetOccupant(Position(row, col));
                if (unit && unit->IsAlive())
                {
                    QString tip = QString("%1 [★%2]\nHP: %3/%4  ATK: %5  Range: %6\n法力: %7/%8")
                        .arg(QString::fromStdString(unit->GetName()))
                        .arg(static_cast<int>(unit->GetStarLevel()))
                        .arg(unit->GetHp()).arg(unit->GetMaxHp())
                        .arg(unit->GetAtk()).arg(unit->GetRange())
                        .arg(unit->GetMana()).arg(unit->GetMaxMana());
                    const auto& traits = unit->GetTraits();
                    if (!traits.empty())
                    {
                        QString traitStr;
                        for (const auto& t : traits)
                            traitStr += QString::fromStdString(t) + " ";
                        tip += QString("\n羁绊: %1").arg(traitStr.trimmed());
                    }
                    setToolTip(tip);
                }
                else
                {
                    setToolTip("");
                }
            }
        }
    }
}
