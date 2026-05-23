#include "ShopWidget.h"
#include "GameCore/GameConfig.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QMap>

namespace synera
{
    static QString GetUnitTexture(const std::string& unitName)
    {
        QMap<std::string, QString> texMap = {
            {"步兵", "warrior"}, {"弓箭手", "archer"}, {"法师", "mage"},
            {"治疗师", "healer"}, {"骑士", "knight"}, {"刺客", "assassin"},
            {"狂战士", "tank"}, {"狙击手", "archer"}, {"侍从", "knight"},
            {"Boss", "boss"}
        };
        auto it = texMap.find(unitName);
        if (it != texMap.end())
            return QString(":/AutoChessDemo/assets/units/%1.png").arg(it.value());
        return ":/AutoChessDemo/assets/units/warrior.png";
    }

    // 根据单位名称获取费用颜色
    static QColor GetCostColor(int cost)
    {
        switch (cost)
        {
        case 1: return QColor(180, 180, 190);     // 白
        case 2: return QColor(50, 200, 80);        // 绿
        case 3: return QColor(80, 140, 255);       // 蓝
        case 4: return QColor(200, 120, 255);      // 紫
        case 5: return QColor(255, 200, 50);       // 金
        default: return QColor(180, 180, 190);
        }
    }

    // 获取单位费用
    static int GetUnitCost(const std::string& name)
    {
        if (name == "步兵" || name == "弓箭手" || name == "侍从") return 1;
        if (name == "法师" || name == "骑士" || name == "治疗师") return 2;
        if (name == "刺客" || name == "狂战士" || name == "狙击手") return 3;
        return 1;
    }

    ShopWidget::ShopWidget(GameManager& gameManager, QWidget* parent)
        : QWidget(parent)
        , m_gameManager(gameManager)
    {
        setMinimumHeight(110);
    }

    void ShopWidget::RefreshDisplay()
    {
        update();
    }

    void ShopWidget::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        // 商店背景 — 渐变
        {
            QLinearGradient shopBg(0, 0, 0, height());
            shopBg.setColorAt(0, QColor(34, 30, 46));
            shopBg.setColorAt(1, QColor(28, 24, 38));
            painter.fillRect(rect(), shopBg);
        }

        // 商店标题栏 — 渐变
        {
            QLinearGradient titleGrad(0, 8, 0, 30);
            titleGrad.setColorAt(0, QColor(44, 40, 58));
            titleGrad.setColorAt(1, QColor(36, 32, 50));
            painter.fillRect(QRect(0, 8, width(), 22), titleGrad);
        }
        painter.setPen(QColor(180, 170, 210));
        QFont tf = painter.font();
        tf.setPointSize(9);
        tf.setBold(true);
        painter.setFont(tf);

        const auto& shop = m_gameManager.GetShop();
        painter.drawText(10, 10, QString("💰 金币: %1").arg(m_gameManager.GetGold()));

        // 分隔线 — 渐变
        {
            QLinearGradient sepGrad(0, 0, width(), 0);
            sepGrad.setColorAt(0, QColor(60, 55, 80, 0));
            sepGrad.setColorAt(0.5, QColor(80, 70, 100, 120));
            sepGrad.setColorAt(1, QColor(60, 55, 80, 0));
            painter.fillRect(QRect(0, 32, width(), 1), sepGrad);
        }

        const auto& units = shop.GetShopUnits();
        const int slotW = width() / SHOP_SIZE;
        const int cardY = 36;
        const int cardH = height() - 40;

        for (int i = 0; i < SHOP_SIZE; ++i)
        {
            int cardX = i * slotW + 4;
            int cardW = slotW - 8;
            QRect cardRect(cardX, cardY, cardW, cardH);

            // 卡片背景 — 渐变圆角卡
            {
                QLinearGradient cardBg(cardRect.topLeft(), cardRect.bottomLeft());
                cardBg.setColorAt(0, QColor(48, 44, 62));
                cardBg.setColorAt(1, QColor(40, 36, 52));
                painter.setBrush(cardBg);
                painter.setPen(QPen(QColor(70, 60, 90, 120), 1));
                painter.drawRoundedRect(cardRect, 8, 8);
            }

            if (i < static_cast<int>(units.size()) && units[i])
            {
                auto& unit = units[i];
                int cost = GetUnitCost(unit->GetName());
                QColor costColor = GetCostColor(cost);

                // 卡片顶部费用色条 — 渐变
                {
                    QLinearGradient costBar(cardRect.topLeft(), cardRect.topRight());
                    costBar.setColorAt(0, QColor(costColor.red(), costColor.green(), costColor.blue(), 50));
                    costBar.setColorAt(1, QColor(costColor.red(), costColor.green(), costColor.blue(), 20));
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(cardRect, 8, 8);
                    painter.setClipPath(clipPath);
                    painter.fillRect(cardRect.adjusted(0, 0, 0, -cardH + 24), costBar);
                    painter.setClipping(false);
                }

                // 费用标签（右上角）— 圆形徽章
                {
                    int badgeX = cardRect.right() - 24;
                    int badgeY = cardRect.top() + 6;
                    int badgeR = 14;
                    QRadialGradient badgeGrad(badgeX + badgeR / 2, badgeY + badgeR / 2, badgeR);
                    badgeGrad.setColorAt(0, costColor.lighter(130));
                    badgeGrad.setColorAt(1, costColor);
                    painter.setBrush(badgeGrad);
                    painter.setPen(QPen(costColor.lighter(150), 1));
                    painter.drawEllipse(badgeX, badgeY, badgeR, badgeR);
                    painter.setPen(Qt::white);
                    QFont cf = painter.font();
                    cf.setPointSize(8);
                    cf.setBold(true);
                    painter.setFont(cf);
                    painter.drawText(badgeX, badgeY, badgeR, badgeR,
                                     Qt::AlignCenter, QString::number(cost));
                }

                // 单位圆形光晕背景
                {
                    int glowSize = qMin(cardW - 20, cardH - 60);
                    glowSize = qMin(glowSize, 64);
                    QRect glowRect(cardRect.center().x() - glowSize / 2,
                                   cardRect.top() + 16, glowSize, glowSize);
                    QRadialGradient radGrad(glowRect.center(), glowSize / 2);
                    radGrad.setColorAt(0, QColor(costColor.red(), costColor.green(), costColor.blue(), 40));
                    radGrad.setColorAt(1, QColor(costColor.red(), costColor.green(), costColor.blue(), 0));
                    painter.setBrush(radGrad);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(glowRect);
                }

                // 单位精灵图
                QString texPath = GetUnitTexture(unit->GetName());
                QPixmap unitTex(texPath);
                if (!unitTex.isNull())
                {
                    int spriteSize = qMin(cardW - 20, cardH - 60);
                    spriteSize = qMin(spriteSize, 56);
                    QPixmap scaled = unitTex.scaled(spriteSize, spriteSize,
                                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int sx = cardRect.center().x() - scaled.width() / 2;
                    int sy = cardRect.top() + 18;
                    painter.drawPixmap(sx, sy, scaled);
                }
                else
                {
                    painter.setBrush(costColor.darker(150));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cardRect.center(), 20, 20);
                }

                // 单位名称
                painter.setPen(Qt::white);
                QFont nf = painter.font();
                nf.setPointSize(8);
                nf.setBold(true);
                painter.setFont(nf);
                QString name = QString::fromStdString(unit->GetName());
                painter.drawText(cardRect.adjusted(2, cardH - 28, -2, -8),
                                 Qt::AlignBottom | Qt::AlignHCenter, name);

                // 费用标签（底部）
                {
                    painter.setPen(costColor);
                    QFont cf2 = painter.font();
                    cf2.setPointSize(7);
                    cf2.setBold(true);
                    painter.setFont(cf2);
                    painter.drawText(cardRect.adjusted(4, 2, -4, -2),
                                     Qt::AlignBottom | Qt::AlignLeft,
                                     QString("💰 %1 金").arg(cost));
                }
                // 提示文字
                painter.setPen(QColor(140, 130, 170));
                QFont cf3 = painter.font();
                cf3.setPointSize(6);
                cf3.setBold(false);
                painter.setFont(cf3);
                painter.drawText(cardRect.adjusted(4, 2, -4, -2),
                                 Qt::AlignBottom | Qt::AlignRight, "点击查看");
            }
            else
            {
                // 空槽位 - 显示"已售"
                painter.setPen(QColor(80, 75, 95));
                QFont ef = painter.font();
                ef.setPointSize(8);
                painter.setFont(ef);
                painter.drawText(cardRect, Qt::AlignCenter, "已售");
            }
        }
    }

    void ShopWidget::mousePressEvent(QMouseEvent* event)
    {
        const int slotW = width() / SHOP_SIZE;
        int index = event->pos().x() / slotW;

        if (index >= 0 && index < SHOP_SIZE)
            emit UnitPurchased(index);
    }
}
