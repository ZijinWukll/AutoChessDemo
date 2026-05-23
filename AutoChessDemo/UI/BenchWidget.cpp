#include "BenchWidget.h"
#include "GameCore/GameConfig.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMap>
#include <QDateTime>

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
        auto it = texMap.find(unitName);
        if (it != texMap.end())
            return QString(":/AutoChessDemo/assets/units/%1%2.png").arg(it.value()).arg(suffix);
        return QString(":/AutoChessDemo/assets/units/warrior%1.png").arg(suffix);
    }

    BenchWidget::BenchWidget(GameManager& gameManager, QWidget* parent)
        : QWidget(parent)
        , m_gameManager(gameManager)
        , m_slotCount(static_cast<int>(BENCH_CAPACITY))
    {
        setMinimumHeight(90);
        setMouseTracking(true);
    }

    QRect BenchWidget::GetSlotRect(int index) const
    {
        const int slotW = width() / m_slotCount;
        return QRect(index * slotW, 4, slotW - 4, height() - 8);
    }

    void BenchWidget::SetSelectedSlot(int index)
    {
        m_selectedSlot = index;
        update();
    }

    void BenchWidget::ClearSelectedSlot()
    {
        m_selectedSlot = -1;
        update();
    }

    // 统计同名同星级的单位数量（用于合星提示）
    int BenchWidget::CountSameNameStar(const std::string& name, StarLevel star) const
    {
        int count = 0;
        const auto& bench = m_gameManager.GetBench();
        for (size_t i = 0; i < bench.GetCapacity(); ++i)
        {
            auto u = bench.GetUnit(i);
            if (u && u->GetName() == name && u->GetStarLevel() == star)
                count++;
        }
        // 也检查棋盘上的
        const auto& board = m_gameManager.GetBoard();
        for (int r = 0; r < board.GetRows(); ++r)
            for (int c = 0; c < board.GetCols(); ++c)
            {
                auto u = board.GetOccupant(Position(r, c));
                if (u && u->GetOwner() == Owner::PlayerCtrl &&
                    u->GetName() == name && u->GetStarLevel() == star)
                    count++;
            }
        return count;
    }

    void BenchWidget::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        // 备战区背景 — 渐变
        {
            QLinearGradient bg(0, 0, 0, height());
            bg.setColorAt(0, QColor(34, 32, 48));
            bg.setColorAt(1, QColor(26, 24, 38));
            painter.fillRect(rect(), bg);
        }

        // 顶部分隔线 — 发光效果
        painter.setPen(QPen(QColor(80, 70, 100, 60), 1));
        painter.drawLine(0, 0, width(), 0);
        painter.setPen(QPen(QColor(60, 50, 80, 40), 2));
        painter.drawLine(0, 1, width(), 1);

        // (选中提示已移至主窗口的提示栏，避免遮挡槽位)

        const Bench& bench = m_gameManager.GetBench();
        const int slotW = width() / m_slotCount;
        const int slotH = height();

        for (int i = 0; i < m_slotCount; ++i)
        {
            QRect slotRect(i * slotW + 2, 6, slotW - 4, slotH - 12);

            // 空槽位 - 深色渐变背景
            {
                QLinearGradient slotGrad(0, slotRect.top(), 0, slotRect.bottom());
                slotGrad.setColorAt(0, QColor(44, 42, 58));
                slotGrad.setColorAt(1, QColor(36, 34, 48));
                painter.setBrush(slotGrad);
                painter.setPen(QPen(QColor(55, 50, 70, 80), 1));
                painter.drawRoundedRect(slotRect, 5, 5);
            }

            // 槽位编号
            painter.setPen(QColor(60, 55, 75));
            QFont nf = painter.font();
            nf.setPointSize(7);
            painter.setFont(nf);
            painter.drawText(slotRect.adjusted(2, 2, -2, -2), Qt::AlignBottom | Qt::AlignRight,
                             QString("S%1").arg(i));

            // 绘制单位
            auto unit = bench.GetUnit(static_cast<size_t>(i));
            if (unit)
            {
                QRect uRect = slotRect.adjusted(4, 4, -4, -4);
                int count = CountSameNameStar(unit->GetName(), unit->GetStarLevel());

                // 合星提示框：如果 >= 2 个同名同星，加金色边框带外发光
                if (count >= 2)
                {
                    int flash = (static_cast<int>(QDateTime::currentMSecsSinceEpoch() / 500) % 2);
                    // 外发光
                    painter.setPen(QPen(QColor(255, 215, 0, flash ? 30 : 10), 6));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(slotRect.adjusted(0, 0, 0, 0), 6, 6);
                    // 主边框
                    QPen mergePen(QColor(255, 215, 0), flash ? 2 : 1);
                    painter.setPen(mergePen);
                    painter.drawRoundedRect(slotRect.adjusted(2, 2, -2, -2), 6, 6);

                    // 合星进度文字
                    painter.setPen(QColor(255, 215, 0));
                    QFont mf = painter.font();
                    mf.setPointSize(8);
                    mf.setBold(true);
                    painter.setFont(mf);
                    painter.drawText(slotRect.adjusted(6, 4, -6, -4),
                                     Qt::AlignTop | Qt::AlignLeft,
                                     QString("合星 %1/3").arg(count));
                }

                // 选中的槽位高亮边框 — 冰蓝光效
                if (i == m_selectedSlot)
                {
                    painter.setPen(QPen(QColor(100, 200, 255, 40), 6));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(slotRect.adjusted(0, 0, 0, 0), 6, 6);
                    QPen selPen(QColor(100, 200, 255), 2);
                    painter.setPen(selPen);
                    painter.drawRoundedRect(slotRect.adjusted(2, 2, -2, -2), 6, 6);
                }

                // 单位圆形背景
                {
                    int bgSize = qMin(uRect.width(), uRect.height()) - 4;
                    QRect bgRect(uRect.center().x() - bgSize / 2, uRect.top() + 6, bgSize, bgSize);
                    QRadialGradient radGrad(bgRect.center(), bgSize / 2);
                    QColor ownerColor = (unit->GetOwner() == Owner::PlayerCtrl)
                        ? QColor(50, 100, 200, 60) : QColor(200, 60, 60, 60);
                    radGrad.setColorAt(0, ownerColor);
                    radGrad.setColorAt(1, QColor(0, 0, 0, 0));
                    painter.setBrush(radGrad);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(bgRect);
                }

                // 加载单位贴图
                QString texPath = GetUnitTexture(unit->GetName());
                QPixmap unitTex(texPath);
                if (!unitTex.isNull())
                {
                    int size = qMin(uRect.width(), uRect.height()) - 8;
                    QPixmap scaled = unitTex.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = uRect.center().x() - scaled.width() / 2;
                    int y = uRect.top() + 8;
                    painter.drawPixmap(x, y, scaled);
                }
                else
                {
                    painter.setBrush(QColor(50, 130, 230, 180));
                    painter.setPen(Qt::NoPen);
                    painter.drawRoundedRect(uRect, 8, 8);
                }

                // 名称
                painter.setPen(Qt::white);
                QFont nf2 = painter.font();
                nf2.setPointSize(8);
                nf2.setBold(false);
                painter.setFont(nf2);
                QString label = QString::fromStdString(unit->GetName());
                if (label.length() > 6) label = label.left(6);
                painter.drawText(uRect.adjusted(0, 0, 0, -2), Qt::AlignBottom | Qt::AlignHCenter, label);

                // 星级徽章
                int starLevel = static_cast<int>(unit->GetStarLevel());
                if (starLevel > 1)
                {
                    QString starLabel = QString("★%1").arg(starLevel);
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 215, 0, 200));
                    QFont sf = painter.font();
                    sf.setPointSize(7);
                    sf.setBold(true);
                    painter.setFont(sf);
                    QRect badge(uRect.right() - 20, uRect.top() + 2, 18, 12);
                    painter.drawRoundedRect(badge, 3, 3);
                    painter.setPen(QColor(40, 30, 0));
                    painter.drawText(badge, Qt::AlignCenter, starLabel);
                }

                // 血条
                if (unit->GetMaxHp() > 0)
                {
                    float hpRatio = static_cast<float>(unit->GetHp()) / unit->GetMaxHp();
                    int barW = uRect.width() - 10;
                    int barH = 3;
                    int barX = uRect.center().x() - barW / 2;
                    int barY = uRect.bottom() - 18;
                    painter.setBrush(QColor(60, 60, 60, 150));
                    painter.setPen(Qt::NoPen);
                    painter.drawRect(barX, barY, barW, barH);
                    QColor hpC = (hpRatio > 0.5f) ? QColor(50, 200, 50) : QColor(220, 200, 30);
                    painter.setBrush(hpC);
                    painter.drawRect(barX, barY, static_cast<int>(barW * hpRatio), barH);
                }
            }
        }
    }

    void BenchWidget::mousePressEvent(QMouseEvent* event)
    {
        const int slotW = width() / m_slotCount;
        int index = event->pos().x() / slotW;

        if (index < 0 || index >= m_slotCount)
            return;

        if (event->button() == Qt::RightButton)
        {
            auto unit = m_gameManager.GetBench().GetUnit(static_cast<size_t>(index));
            if (unit)
                emit SellRequested(index);
            return;
        }

        if (event->button() == Qt::LeftButton)
            emit SlotClicked(index);
    }

    void BenchWidget::mouseMoveEvent(QMouseEvent* event)
    {
        // 鼠标不在备战区范围内 → 清除高亮（拖动时鼠标可能移到棋盘上）
        if (!rect().contains(event->pos()))
        {
            ClearSelectedSlot();
            return;
        }

        const int slotW = width() / m_slotCount;
        int index = event->pos().x() / slotW;

        int prev = m_selectedSlot;
        if (index >= 0 && index < m_slotCount)
        {
            auto unit = m_gameManager.GetBench().GetUnit(static_cast<size_t>(index));
            if (unit)
            {
                if (index != prev)
                    SetSelectedSlot(index);
                return;
            }
        }
        if (prev >= 0)
            ClearSelectedSlot();
    }

    void BenchWidget::leaveEvent(QEvent* event)
    {
        ClearSelectedSlot();
        QWidget::leaveEvent(event);
    }
}
