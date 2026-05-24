#include "UnitInfoWidget.h"

#include <QPainter>
#include <QMap>
#include <QVBoxLayout>

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
        setMinimumSize(200, 360);

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
            // 计算出售价格
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

            // 定位在底部居中，在所有文字下方
            int btnW = 110;
            int btnH = 30;
            m_sellButton->setFixedSize(btnW, btnH);
            m_sellButton->move((width() - btnW) / 2, height() - 38);
            m_sellButton->show();
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

        // 背景
        painter.fillRect(rect(), QColor(28, 26, 38));

        // 装饰性边框
        painter.setPen(QPen(QColor(60, 55, 80), 1));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));

        if (!m_selectedUnit)
        {
            painter.setPen(QColor(100, 95, 120));
            QFont f = painter.font();
            f.setPointSize(11);
            painter.setFont(f);
            painter.drawText(rect(), Qt::AlignCenter, "未选中单位");
            return;
        }

        const auto& unit = m_selectedUnit;
        int y = 16;

        auto drawLine = [&](const QString& text, const QColor& color = Qt::white, int size = 11) {
            painter.setPen(color);
            QFont f = painter.font();
            f.setPointSize(size);
            painter.setFont(f);
            painter.drawText(12, y, text);
            y += 24;
        };

        // ===== 单位大图 =====
        QString texPath = GetUnitTexture(unit->GetName());
        QPixmap unitTex(texPath);
        if (!unitTex.isNull())
        {
            int imgSize = qMin(width() - 40, 80);
            QPixmap scaled = unitTex.scaled(imgSize, imgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int ix = (width() - scaled.width()) / 2;
            int iy = y;
            painter.drawPixmap(ix, iy, scaled);
            y += scaled.height() + 12;
        }

        // ===== 名称 + 星级 =====
        QString starStr;
        for (int i = 0; i < static_cast<int>(unit->GetStarLevel()); ++i)
            starStr += "★";
        drawLine(QString::fromStdString(unit->GetName()) + "  " + starStr,
                 unit->GetStarLevel() > StarLevel::One ? QColor(255, 215, 0) : Qt::white, 13);

        // ===== 归属标签 =====
        {
            QColor ownerColor = (unit->GetOwner() == Owner::PlayerCtrl)
                                ? QColor(80, 160, 255) : QColor(255, 80, 80);
            QString ownerStr = (unit->GetOwner() == Owner::PlayerCtrl) ? "[我方]" : "[敌方]";

            painter.setBrush(ownerColor.darker(140));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(12, y - 16, 50, 20, 4, 4);
            painter.setPen(Qt::white);
            QFont of = painter.font();
            of.setPointSize(9);
            painter.setFont(of);
            painter.drawText(12, y - 16, 50, 20, Qt::AlignCenter, ownerStr);
            y += 12;
        }

        y += 8;

        // ===== 属性条 =====
        auto drawStatBar = [&](const QString& label, int value, int maxValue,
                               const QColor& color, int barWidth) {
            painter.setPen(QColor(180, 180, 190));
            QFont sf = painter.font();
            sf.setPointSize(10);
            painter.setFont(sf);

            int labelX = 12;
            int barX = 80;
            int valX = barX + barWidth + 4;

            painter.drawText(labelX, y - 14, 65, 20, Qt::AlignLeft | Qt::AlignVCenter, label);

            // 背景条
            painter.setBrush(QColor(50, 48, 65));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(barX, y - 10, barWidth, 10, 3, 3);

            // 前景条
            float ratio = (maxValue > 0) ? qMin(1.0f, static_cast<float>(value) / maxValue) : 0.0f;
            if (ratio > 0.0f)
            {
                painter.setBrush(color);
                painter.drawRoundedRect(barX, y - 10,
                                        static_cast<int>(barWidth * ratio), 10, 3, 3);
            }

            // 数值
            painter.setPen(Qt::white);
            painter.drawText(valX, y - 14, 80, 20, Qt::AlignLeft | Qt::AlignVCenter,
                             QString::number(value) + (maxValue > 0 ? "/" + QString::number(maxValue) : ""));
            y += 22;
        };

        int barW = qMin(width() - 170, 80);
        barW = qMax(barW, 40);

        drawStatBar("生命 HP", unit->GetHp(), unit->GetMaxHp(), QColor(50, 200, 50), barW);
        drawStatBar("攻击 ATK", unit->GetAtk(), 0, QColor(255, 150, 50), barW);
        drawStatBar("射程 RNG", unit->GetRange(), 0, QColor(100, 180, 255), barW);
        drawStatBar("法力 Mana", unit->GetMana(), unit->GetMaxMana(), QColor(80, 120, 255), barW);

        // ===== 状态 =====
        QString stateStr;
        switch (unit->GetState()) {
        case UnitState::Idle:      stateStr = "空闲"; break;
        case UnitState::Moving:    stateStr = "移动中"; break;
        case UnitState::Attacking: stateStr = "攻击中"; break;
        case UnitState::Casting:   stateStr = "施法中"; break;
        case UnitState::Dead:      stateStr = "已阵亡"; break;
        }
        drawLine(QString("状态: %1").arg(stateStr), QColor(180, 180, 200), 10);

        // ===== 分隔线 =====
        y += 4;
        painter.fillRect(12, y, width() - 24, 1, QColor(60, 55, 80));
        y += 12;

        // ===== 羁绊 =====
        QString traitsStr;
        for (const auto& t : unit->GetTraits())
        {
            if (!traitsStr.isEmpty()) traitsStr += "  ";
            traitsStr += QString::fromStdString(t);
        }
        if (!traitsStr.isEmpty())
        {
            // 羁绊标签
            painter.setPen(QColor(180, 170, 220));
            QFont tf = painter.font();
            tf.setPointSize(9);
            painter.setFont(tf);
            painter.drawText(12, y, QString("羁绊: %1").arg(traitsStr));
            y += 20;
        }

        // ===== 装备区（预留） =====
        painter.fillRect(12, y, width() - 24, 1, QColor(60, 55, 80));
        y += 10;
        painter.setPen(QColor(100, 95, 120));
        painter.drawText(12, y, "装备: (无)");

        // 刷新出售按钮位置（窗口大小变化时）
        UpdateSellButton();
    }
}
