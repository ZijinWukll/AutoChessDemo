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
            {"狂战士", "tank"}, {"狙击手", "archer"}, {"侍从", "knight"},
            {"Boss", "boss"}
        };
        if (unitName == "Soldier" || unitName == "步兵") texMap[unitName] = "warrior";
        if (unitName == "Archer" || unitName == "弓箭手") texMap[unitName] = "archer";
        if (unitName == "Mage" || unitName == "法师") texMap[unitName] = "mage";
        if (unitName == "Healer" || unitName == "治疗师") texMap[unitName] = "healer";
        if (unitName == "Knight" || unitName == "骑士" || unitName == "侍从") texMap[unitName] = "knight";
        if (unitName == "Assassin" || unitName == "刺客") texMap[unitName] = "assassin";
        if (unitName == "Berserker" || unitName == "狂战士") texMap[unitName] = "tank";

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

        // 缓存格子贴图 — 渐变风格
        static QPixmap cellLight, cellDark;
        if (cellLight.isNull())
        {
            auto makeCell = [](const QColor& c1, const QColor& c2) {
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
                cp.setBrush(QColor(255, 255, 255, 8));
                cp.drawRoundedRect(2, 2, 60, 60, 3, 3);
                return px;
            };
            cellLight = makeCell(QColor(215, 202, 180), QColor(195, 182, 160));
            cellDark  = makeCell(QColor(175, 158, 135), QColor(155, 138, 115));
        }

        // 缓存高亮贴图 — 带外发光效果
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
            hlSelect = makeHL(QColor(80, 200, 200, 50),  QColor(80, 200, 200),     QColor(80, 200, 200, 40));
            hlMove   = makeHL(QColor(100, 200, 100, 40), QColor(100, 200, 100),  QColor(100, 200, 100, 30));
            hlAttack = makeHL(QColor(255, 80, 80, 40),   QColor(255, 80, 80),    QColor(255, 80, 80, 30));
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
        // 敌方半场（上半）— 暗红渐变
        {
            QLinearGradient enemyGrad(0, 0, 0, halfRow * cellH);
            enemyGrad.setColorAt(0, QColor(160, 40, 40, 70));
            enemyGrad.setColorAt(1, QColor(160, 40, 40, 20));
            painter.fillRect(enemyHalf, enemyGrad);
        }
        // 我方半场（下半）— 暗蓝渐变
        {
            QLinearGradient playerGrad(0, halfRow * cellH, 0, height());
            playerGrad.setColorAt(0, QColor(40, 70, 160, 20));
            playerGrad.setColorAt(1, QColor(40, 70, 160, 70));
            painter.fillRect(playerHalf, playerGrad);
        }

        // ===== 第3层：中线分隔 — 微光效果 =====
        {
            int midY = halfRow * cellH;
            // 外发光
            painter.setPen(QPen(QColor(120, 160, 255, 30), 6));
            painter.drawLine(4, midY, width() - 4, midY);
            // 主线
            QPen midPen(QColor(120, 160, 255, 150), 2, Qt::DashLine);
            painter.setPen(midPen);
            painter.drawLine(8, midY, width() - 8, midY);
            // 端点装饰 — 小圆点
            painter.setBrush(QColor(120, 160, 255, 180));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(6, midY), 3, 3);
            painter.drawEllipse(QPointF(width() - 6, midY), 3, 3);
        }

        // ===== 第4层：拖拽放置区指示 =====
        if (m_dragZoneVisible)
        {
            // 己方半场：柔和绿覆盖 + 淡边框
            for (int r = halfRow; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                {
                    Position pos(r, c);
                    if (!board.IsOccupied(pos))
                    {
                        QRect cr(c * cellW, r * cellH, cellW, cellH);
                        painter.fillRect(cr, QColor(80, 200, 120, 20));
                        painter.setPen(QPen(QColor(80, 200, 120, 100), 2));
                        painter.drawRoundedRect(cr.adjusted(3, 3, -3, -3), 6, 6);
                        // 中心十字标记
                        painter.setPen(QPen(QColor(80, 200, 120, 50), 1));
                        int m = 6, cx = cr.center().x(), cy = cr.center().y();
                        painter.drawLine(cx - m, cy, cx + m, cy);
                        painter.drawLine(cx, cy - m, cx, cy + m);
                    }
                }
            // 敌方半场：暗红覆盖（表示不可放置）
            {
                QLinearGradient denGrad(0, 0, 0, halfRow * cellH);
                denGrad.setColorAt(0, QColor(180, 60, 60, 22));
                denGrad.setColorAt(1, QColor(180, 60, 60, 6));
                painter.fillRect(QRect(0, 0, width(), halfRow * cellH), denGrad);
            }
        }

        // ===== 第5层：半场标签 =====
        {
            QFont labelFont = painter.font();
            labelFont.setPointSize(9);
            labelFont.setBold(true);
            painter.setFont(labelFont);

            // 敌方标签 (左上) — 红色标签背景
            {
                QRect tagRect(4, 4, 80, 18);
                painter.setBrush(QColor(180, 50, 50, 100));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(tagRect, 4, 4);
                painter.setPen(QColor(255, 140, 140, 220));
                painter.drawText(tagRect, Qt::AlignCenter, "⚔ 敌方区域");
            }

            // 我方标签 (左下) — 蓝色标签背景
            {
                QRect tagRect(4, height() - 22, 80, 18);
                painter.setBrush(QColor(50, 80, 180, 100));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(tagRect, 4, 4);
                painter.setPen(QColor(140, 200, 255, 220));
                painter.drawText(tagRect, Qt::AlignCenter, "🛡 我方区域");
            }
        }

        // ===== 第6层：网格线（极淡） =====
        {
            painter.setPen(QPen(QColor(100, 90, 75, 40), 1));
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

        // ===== 第6层：高亮 =====
        for (const auto& [hlPos, hlColor] : m_highlights)
        {
            QRect cellRect(hlPos.y * cellW, hlPos.x * cellH, cellW, cellH);
            QPixmap* hlTex = &hlMove;
            if (hlColor == QColor(80, 200, 200, 80)) hlTex = &hlSelect;
            else if (hlColor == QColor(255, 80, 80, 60)) hlTex = &hlAttack;
            painter.drawPixmap(cellRect, *hlTex);
        }

        // ===== 第7层：绘制已部署的单位 =====
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

        // ===== 第8层：动画中的单位 =====
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

        // ===== 第9层：部署空位提示 =====
        if (m_gameManager.GetCurrentPhase() == GamePhase::Preparation)
        {
            // 在空的己方半场格子上画淡绿色边框表示可部署
            painter.setPen(QPen(QColor(100, 200, 100, 120), 2));
            for (int r = halfRow; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                {
                    Position pos(r, c);
                    if (!board.IsOccupied(pos))
                    {
                        QRect cr(c * cellW, r * cellH, cellW, cellH);
                        painter.setBrush(QColor(80, 180, 80, 40));
                        painter.drawRoundedRect(cr.adjusted(3, 3, -3, -3), 6, 6);
                        // 角落装饰线
                        painter.setPen(QPen(QColor(80, 200, 80, 80), 1));
                        int m = 8;
                        int cx = cr.center().x(), cy = cr.center().y();
                        painter.drawLine(cx - m, cy, cx + m, cy);  // 十字
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

        int margin = 3;
        QRect uRect = cellRect.adjusted(margin, margin, -margin, -margin);
        int size = qMin(uRect.width(), uRect.height());

        // 加载 sprite
        QString texPath = GetUnitTexture(unit->GetName());
        QPixmap unitTex(texPath);
        if (!unitTex.isNull())
        {
            QPixmap scaled = unitTex.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            if (unit->GetOwner() == Owner::EnemyCtrl)
            {
                QTransform tf;
                tf.scale(1, -1);
                scaled = scaled.transformed(tf);
            }

            int x = uRect.center().x() - scaled.width() / 2;
            int y = uRect.center().y() - scaled.height() / 2;
            painter.drawPixmap(x, y, scaled);
        }
        else
        {
            QColor unitColor = (unit->GetOwner() == Owner::PlayerCtrl)
                               ? QColor(50, 150, 255, 200)
                               : QColor(255, 80, 80, 200);
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

            painter.setBrush(QColor(40, 40, 40, 180));
            painter.setPen(Qt::NoPen);
            painter.drawRect(barX, barY, barW, barH);

            QColor hpColor = (hpRatio > 0.5f) ? QColor(50, 200, 50) :
                             (hpRatio > 0.25f) ? QColor(220, 200, 30) : QColor(220, 40, 40);
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

            painter.setBrush(QColor(30, 30, 80, 160));
            painter.setPen(Qt::NoPen);
            painter.drawRect(barX, barY, barW, barH);

            painter.setBrush(QColor(80, 120, 255));
            painter.drawRect(barX, barY, static_cast<int>(barW * manaRatio), barH);
        }

        // 名称
        if (size > 30)
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
