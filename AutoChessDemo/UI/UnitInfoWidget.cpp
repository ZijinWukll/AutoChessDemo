#include "UnitInfoWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMap>
#include <QVBoxLayout>
#include <cmath>

namespace synera
{
    static QString GetUnitTexture(const std::string& unitName)
    {
        QMap<std::string, QString> texMap = {
            {"步兵", "warrior"}, {"弓箭手", "archer"}, {"法师", "mage"},
            {"治疗师", "healer"}, {"骑士", "knight"}, {"刺客", "assassin"},
            {"狂战士", "tank"}, {"狙击手", "sniper"}, {"侍从", "squire"},
            {"Boss", "boss"}
        };
        auto it = texMap.find(unitName);
        if (it != texMap.end())
            return QString(":/AutoChessDemo/assets/units/%1.png").arg(it.value());
        return ":/AutoChessDemo/assets/units/warrior.png";
    }

    static int GetUnitCostForSell(const std::string& name)
    {
        if (name == "步兵" || name == "弓箭手" || name == "侍从") return 1;
        if (name == "法师" || name == "骑士" || name == "治疗师") return 2;
        if (name == "刺客" || name == "狂战士" || name == "狙击手") return 3;
        return 1;
    }

    UnitInfoWidget::UnitInfoWidget(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumSize(200, 280);

        // 创建出售按钮（默认隐藏）
        m_sellButton = new QPushButton("出售", this);
        m_sellButton->setStyleSheet(
            "QPushButton { background-color: #c0392b; color: white; border: none; "
            "border-radius: 5px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background-color: #e74c3c; }"
            "QPushButton:pressed { background-color: #96281b; }");
        m_sellButton->hide();
        connect(m_sellButton, &QPushButton::clicked, this, [this]() {
            if (m_selectedUnit)
                emit SellRequested(m_selectedUnit);
        });
    }

    void UnitInfoWidget::ShowUnit(std::shared_ptr<Unit> unit, bool isShopPreview)
    {
        m_selectedUnit = unit;
        m_isShopPreview = isShopPreview;
        UpdateSellButton();
        update();
    }

    void UnitInfoWidget::Clear()
    {
        m_selectedUnit.reset();
        m_isShopPreview = false;
        if (m_sellButton)
            m_sellButton->hide();
        update();
    }

    void UnitInfoWidget::UpdateSellButton()
    {
        if (!m_sellButton || !m_selectedUnit)
        {
            if (m_sellButton) m_sellButton->hide();
            return;
        }

        // 只有在选中己方单位且不是商店预览时，才显示出售按钮
        bool showSell = (m_selectedUnit->GetOwner() == Owner::PlayerCtrl && !m_isShopPreview);
        if (showSell)
        {
            int cost = GetUnitCostForSell(m_selectedUnit->GetName());
            int starMul = 1;
            switch (m_selectedUnit->GetStarLevel())
            {
            case StarLevel::Two:   starMul = 3;  break;
            case StarLevel::Three: starMul = 9;  break;
            default:               starMul = 1;  break;
            }
            int sellPrice = cost * starMul;

            m_sellButton->setText(QString("出售 %1金").arg(sellPrice));

            int btnW = 110;
            int btnH = 30;
            m_sellButton->setFixedSize(btnW, btnH);
            m_sellButton->move((width() - btnW) / 2, height() - btnH - 10);
            m_sellButton->show();
            m_sellButton->raise();
        }
        else
        {
            m_sellButton->hide();
        }
    }

    void UnitInfoWidget::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        // === 背景 ===
        QLinearGradient bgGrad(0, 0, 0, height());
        bgGrad.setColorAt(0, QColor(26, 24, 38));
        bgGrad.setColorAt(1, QColor(20, 18, 32));
        painter.fillRect(rect(), bgGrad);

        // 装饰边框
        painter.setPen(QPen(QColor(60, 55, 85, 160), 1));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));

        // 顶部细高光线
        painter.fillRect(rect().adjusted(0, 0, 0, -height() + 1), QColor(80, 70, 120, 60));

        if (!m_selectedUnit)
        {
            // === 空状态：装饰性占位 ===
            painter.setPen(QPen(QColor(60, 55, 85, 100), 1, Qt::DashLine));
            painter.drawRoundedRect(rect().adjusted(10, 10, -10, -10), 8, 8);

            // 居中图标（六边形轮廓示意）
            int cx = width() / 2;
            int cy = height() / 2 - 20;
            int hw = 36;

            QPainterPath hexPath;
            for (int i = 0; i < 6; ++i) {
                double angle = i * 60 - 30;
                double px = cx + hw * cos(angle * 3.14159 / 180.0);
                double py = cy + hw * sin(angle * 3.14159 / 180.0);
                if (i == 0) hexPath.moveTo(px, py);
                else hexPath.lineTo(px, py);
            }
            hexPath.closeSubpath();
            painter.setPen(QPen(QColor(80, 70, 120, 120), 2));
            painter.setBrush(QColor(50, 45, 75, 40));
            painter.drawPath(hexPath);

            // 占位文字
            painter.setPen(QColor(100, 95, 130));
            QFont pf = painter.font();
            pf.setPointSize(11);
            pf.setBold(false);
            painter.setFont(pf);
            painter.drawText(QRect(10, cy + hw + 16, width() - 20, 24),
                             Qt::AlignCenter, "点击单位查看详情");

            QFont sf = painter.font();
            sf.setPointSize(9);
            painter.setFont(sf);
            painter.setPen(QColor(80, 75, 105));
            painter.drawText(QRect(10, cy + hw + 36, width() - 20, 20),
                             Qt::AlignCenter, "点击商店预览可购买");
            return;
        }

        const auto& unit = m_selectedUnit;
        int y = 14;

        auto drawLine = [&](const QString& text, const QColor& color = Qt::white, int size = 11) {
            painter.setPen(color);
            QFont f = painter.font();
            f.setPointSize(size);
            painter.setFont(f);
            painter.drawText(12, y, text);
            y += 22;
        };

        // ===== 单位大图 =====
        QString texPath = GetUnitTexture(unit->GetName());
        QPixmap unitTex(texPath);
        if (!unitTex.isNull())
        {
            int imgSize = qMin(width() - 40, 156);  // 130 × 1.2 = 156
            QPixmap scaled = unitTex.scaled(imgSize, imgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // 图片背景柔光
            int cx = width() / 2;
            int iy = y;
            QRadialGradient glow(QPointF(cx, iy + scaled.height() / 2.0f), scaled.width() * 0.7f);
            QColor glowColor = (unit->GetStarLevel() > StarLevel::One)
                ? QColor(255, 215, 0, 25) : QColor(130, 90, 220, 25);
            glow.setColorAt(0, glowColor);
            glow.setColorAt(1, QColor(0, 0, 0, 0));
            painter.fillRect(cx - scaled.width() / 2 - 10, iy - 4,
                            scaled.width() + 20, scaled.height() + 8, glow);

            int ix = (width() - scaled.width()) / 2;
            painter.drawPixmap(ix, iy, scaled);

            // 星级角标（放在图片右下角）
            if (unit->GetStarLevel() > StarLevel::One)
            {
                int starLv = static_cast<int>(unit->GetStarLevel());
                QString starLabel = QString("★%1").arg(starLv);
                QFont sf = painter.font();
                sf.setPointSize(9);
                sf.setBold(true);
                painter.setFont(sf);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(255, 215, 0, 200));
                QRectF badge(ix + scaled.width() - 38, iy + scaled.height() - 18, 36, 16);
                painter.drawRoundedRect(badge, 4, 4);
                painter.setPen(QColor(60, 40, 0));
                painter.drawText(badge, Qt::AlignCenter, starLabel);
            }

            y += scaled.height() + 28;
        }

        // ===== 名称 =====
        {
            QString nameStr = QString::fromStdString(unit->GetName());
            QString starStr;
            for (int i = 0; i < static_cast<int>(unit->GetStarLevel()); ++i)
                starStr += "★";

            QFont nf = painter.font();
            nf.setPointSize(15);
            nf.setBold(true);
            painter.setFont(nf);

            QColor nameColor = (unit->GetStarLevel() > StarLevel::One)
                ? QColor(255, 215, 0) : Qt::white;

            // 名字居中显示
            QFontMetrics fm(nf);
            int textW = fm.horizontalAdvance(nameStr + "  " + starStr);
            int nx = (width() - textW) / 2;
            painter.setPen(nameColor);
            painter.drawText(nx, y, nameStr + "  " + starStr);
            y += 28;
        }

        // ===== 归属标签 =====
        {
            QColor ownerColor = (unit->GetOwner() == Owner::PlayerCtrl)
                                ? QColor(70, 140, 255) : QColor(255, 70, 70);
            QString ownerStr = (unit->GetOwner() == Owner::PlayerCtrl) ? "我方" : "敌方";

            QFont of = painter.font();
            of.setPointSize(9);
            of.setBold(true);
            painter.setFont(of);

            QRectF badgeRect(width() / 2 - 30, y - 14, 60, 20);
            painter.setBrush(ownerColor.darker(150));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(badgeRect, 4, 4);
            painter.setPen(Qt::white);
            painter.drawText(badgeRect, Qt::AlignCenter, ownerStr);
            y += 16;
        }

        y += 6;

        // ===== 属性条 =====
        int labelX = 14;
        int labelW = 50;
        int barX = labelX + labelW + 2;
        int valW = 52;                      // 数值文字宽度
        int valX = width() - 8 - valW;      // 靠右 8px 边距
        int barW = qMax(30, valX - barX - 4);

        auto drawStatBar = [&](const QString& label, int value, int maxValue,
                               const QColor& color) {
            QFont sf = painter.font();
            sf.setPointSize(10);
            sf.setBold(false);
            painter.setFont(sf);
            painter.setPen(QColor(180, 180, 195));

            // 标签
            painter.drawText(labelX, y - 14, labelW, 20, Qt::AlignLeft | Qt::AlignVCenter, label);

            // 背景条
            painter.setBrush(QColor(45, 43, 62));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(barX, y - 8, barW, 8, 3, 3);

            // 前景条（渐变效果）
            float ratio = (maxValue > 0) ? qMin(1.0f, static_cast<float>(value) / maxValue) : 0.0f;
            if (ratio > 0.0f)
            {
                int fillW = static_cast<int>(barW * ratio);
                QLinearGradient barGrad(barX, 0, barX + fillW, 0);
                barGrad.setColorAt(0, color.lighter(130));
                barGrad.setColorAt(1, color.darker(110));
                painter.setBrush(barGrad);
                painter.drawRoundedRect(barX, y - 8, fillW, 8, 3, 3);
            }

            // 数值（右对齐）
            painter.setPen(Qt::white);
            QString valStr = (maxValue > 0)
                ? QString("%1/%2").arg(value).arg(maxValue)
                : QString::number(value);
            painter.drawText(valX, y - 14, valW, 20, Qt::AlignRight | Qt::AlignVCenter, valStr);
            y += 20;
        };

        drawStatBar("生命", unit->GetHp(), unit->GetMaxHp(), QColor(60, 210, 60));
        drawStatBar("攻击", unit->GetAtk(), 0, QColor(255, 150, 50));
        drawStatBar("射程", unit->GetRange(), 0, QColor(100, 180, 255));
        drawStatBar("法力", unit->GetMana(), unit->GetMaxMana(), QColor(80, 120, 255));

        y += 2;

        // ===== 状态 =====
        {
            QString stateStr;
            switch (unit->GetState()) {
            case UnitState::Idle:      stateStr = "空闲"; break;
            case UnitState::Moving:    stateStr = "移动中"; break;
            case UnitState::Attacking: stateStr = "攻击中"; break;
            case UnitState::Casting:   stateStr = "施法中"; break;
            case UnitState::Dead:      stateStr = "已阵亡"; break;
            }
            painter.setPen(QColor(150, 145, 170));
            QFont sf = painter.font();
            sf.setPointSize(9);
            painter.setFont(sf);
            painter.drawText(14, y, QString("状态: %1").arg(stateStr));
            y += 18;
        }

        // ===== 分隔线 =====
        {
            int lineY = y;
            QLinearGradient lineGrad(14, 0, width() - 14, 0);
            lineGrad.setColorAt(0, QColor(60, 55, 80, 0));
            lineGrad.setColorAt(0.5, QColor(60, 55, 80, 180));
            lineGrad.setColorAt(1, QColor(60, 55, 80, 0));
            painter.fillRect(14, lineY, width() - 28, 1, lineGrad);
            y += 10;
        }

        // ===== 羁绊 =====
        {
            QString traitsStr;
            for (const auto& t : unit->GetTraits())
            {
                if (!traitsStr.isEmpty()) traitsStr += "  ";
                traitsStr += QString::fromStdString(t);
            }
            if (!traitsStr.isEmpty())
            {
                painter.setPen(QColor(180, 170, 220));
                QFont tf = painter.font();
                tf.setPointSize(9);
                painter.setFont(tf);

                // 羁绊标签
                QFontMetrics tfm(tf);
                int tw = tfm.horizontalAdvance("羁绊: " + traitsStr);
                if (tw > width() - 28)
                {
                    // 截断显示
                    while (!traitsStr.isEmpty() && tfm.horizontalAdvance("羁绊: " + traitsStr + "...") > width() - 28)
                        traitsStr.chop(1);
                    traitsStr += "...";
                }
                painter.drawText(14, y, QString("羁绊: %1").arg(traitsStr));
                y += tfm.height() + 6;
            }
        }

        // ===== 装备区 =====
        {
            QFont ef = painter.font();
            ef.setPointSize(9);
            painter.setFont(ef);
            painter.setPen(QColor(100, 95, 120));
            painter.drawText(14, y, "装备: (无)");
        }

        // 刷新出售按钮位置
        UpdateSellButton();
    }
} // namespace synera
