#include "StartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QScreen>

namespace synera
{

    StartWidget::StartWidget(QWidget* parent)
        : QWidget(parent)
    {
        // 透明背景 — 让 CentralWrapper 的背景图统一显示
        setAttribute(Qt::WA_TranslucentBackground);

        // 按钮样式 — 暗紫渐变，与游戏主题一致
        const QString btnStyle =
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "    stop:0 #5a50a8, stop:1 #4a4080);"
            "  color: white; border: none; border-radius: 8px;"
            "  padding: 14px 48px; font-size: 16px; font-weight: bold;"
            "  min-width: 220px;"
            "}"
            "QPushButton:hover {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "    stop:0 #7a70c8, stop:1 #6a60b0);"
            "}"
            "QPushButton:pressed {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "    stop:0 #3a3088, stop:1 #2a2070);"
            "}";

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);

        // 占位弹性空间（将按钮推到中下方，不遮挡标题）
        layout->addStretch(50);

        // 「开始游戏」按钮
        m_startBtn = new QPushButton("开始游戏", this);
        m_startBtn->setStyleSheet(btnStyle);
        m_startBtn->setCursor(Qt::PointingHandCursor);
        connect(m_startBtn, &QPushButton::clicked, this, &StartWidget::StartGame);

        // 「玩法介绍」按钮
        m_instructionsBtn = new QPushButton("玩法介绍", this);
        m_instructionsBtn->setStyleSheet(btnStyle);
        m_instructionsBtn->setCursor(Qt::PointingHandCursor);
        connect(m_instructionsBtn, &QPushButton::clicked, this, [this]() {
            ShowInstructionsDialog();
        });
        connect(this, &StartWidget::ShowInstructions, this, [this]() {
            ShowInstructionsDialog();
        });

        layout->addWidget(m_startBtn, 0, Qt::AlignCenter);
        layout->addSpacing(8);
        layout->addWidget(m_instructionsBtn, 0, Qt::AlignCenter);

        // 底部弹性空间
        layout->addStretch(1);

        // 版本信息
        QLabel* versionLabel = new QLabel("v4.3", this);
        versionLabel->setStyleSheet("color: #605878; font-size: 11px; padding: 8px;");
        versionLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(versionLabel, 0, Qt::AlignCenter);
    }

    void StartWidget::SetBackgroundImage(const QString& imagePath)
    {
        m_background = QPixmap(imagePath);
        if (!m_background.isNull())
            update();
    }

    void StartWidget::paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        // ===== 背景 — 不绘制，由 CentralWrapper 统一渲染背景图+遮罩 =====
        // 透明背景让 CentralWrapper 的背景图一致显示，避免两层叠加产生色块

        // ===== 顶部装饰光晕 =====
        {
            QRadialGradient glow(QPointF(width() / 2.0, height() * 0.18),
                                 qMin(width(), height()) * 0.5);
            glow.setColorAt(0, QColor(130, 90, 220, 40));
            glow.setColorAt(0.5, QColor(80, 50, 160, 15));
            glow.setColorAt(1, QColor(0, 0, 0, 0));
            painter.fillRect(rect(), glow);
        }

        // ===== 标题「SYNERA」 =====
        {
            QFont titleFont;
            titleFont.setPixelSize(qMin(width(), height()) / 5);
            titleFont.setBold(true);
            titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 6);
            painter.setFont(titleFont);

            int titleY = height() * 2 / 7;
            float textX = width() / 2.0f;
            float textBaseline = titleY + titleFont.pixelSize() * 0.35f;
            float halfW = painter.fontMetrics().horizontalAdvance("SYNERA") / 2.0f;

            // 宽范围柔光（背景辉光）
            QPainterPath glowPath;
            glowPath.addText(textX - halfW, textBaseline, titleFont, "SYNERA");
            QPen softGlow(QColor(180, 140, 255, 25), 40);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(softGlow);
            painter.drawPath(glowPath);

            // 中层紫色发光
            softGlow.setColor(QColor(140, 100, 230, 45));
            softGlow.setWidth(22);
            painter.setPen(softGlow);
            painter.drawPath(glowPath);

            // 暗色描边（增强对比度与可读性）
            QPen strokePen(QColor(10, 8, 20, 180), 5);
            painter.setPen(strokePen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(glowPath);

            // 主文字 — 亮白→金紫渐变（清晰且有层次）
            QLinearGradient textGrad(textX - halfW * 0.8f, 0, textX + halfW * 0.8f, 0);
            textGrad.setColorAt(0, QColor(210, 190, 255));
            textGrad.setColorAt(0.3, QColor(250, 240, 255));
            textGrad.setColorAt(0.6, QColor(230, 210, 255));
            textGrad.setColorAt(1, QColor(190, 165, 245));
            painter.setPen(QPen(textGrad, 1));
            painter.setBrush(textGrad);
            painter.drawText(QRect(0, titleY - 20, width(), titleFont.pixelSize() + 40),
                             Qt::AlignCenter, "SYNERA");
        }

        // ===== 副标题 =====
        {
            QFont titleFont;
            titleFont.setPixelSize(qMin(width(), height()) / 5);
            titleFont.setBold(true);

            int titleY = height() * 2 / 7;

            QFont subFont;
            subFont.setPixelSize(qMin(width(), height()) / 22);
            subFont.setLetterSpacing(QFont::AbsoluteSpacing, 8);
            painter.setFont(subFont);

            float subTextX = width() / 2.0f;
            float subHalfW = painter.fontMetrics().horizontalAdvance("SYNERGY AUTO-ARENA") / 2.0f;
            float subBaseline = titleY + titleFont.pixelSize() / 2 + 120 + subFont.pixelSize() * 0.35f;

            // 副标题暗色描边
            QPainterPath subGlow;
            subGlow.addText(subTextX - subHalfW, subBaseline, subFont, "SYNERGY AUTO-ARENA");
            QPen subStroke(QColor(10, 8, 20, 150), 4);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(subStroke);
            painter.drawPath(subGlow);

            // 副标题发光
            QPen subGlowPen(QColor(140, 100, 220, 40), 16);
            painter.setPen(subGlowPen);
            painter.drawPath(subGlow);

            // 副标题文字 — 亮紫渐变
            QLinearGradient subGrad(subTextX - 140, 0, subTextX + 140, 0);
            subGrad.setColorAt(0, QColor(190, 170, 235));
            subGrad.setColorAt(0.5, QColor(230, 220, 255));
            subGrad.setColorAt(1, QColor(180, 160, 225));
            painter.setPen(QPen(subGrad, 1));
            painter.setBrush(subGrad);
            painter.drawText(QRect(0, titleY + titleFont.pixelSize() / 2 + 120,
                                   width(), subFont.pixelSize() + 10),
                             Qt::AlignCenter, "SYNERGY AUTO-ARENA");
        }

        // ===== 底部装饰线 =====
        {
            int lineY = height() - 40;
            int lineW = qMin(width() * 2 / 3, 400);
            int lineX = (width() - lineW) / 2;
            QLinearGradient lineGrad(lineX, 0, lineX + lineW, 0);
            lineGrad.setColorAt(0, QColor(80, 65, 140, 0));
            lineGrad.setColorAt(0.5, QColor(100, 80, 180, 100));
            lineGrad.setColorAt(1, QColor(80, 65, 140, 0));
            painter.fillRect(lineX, lineY, lineW, 1, lineGrad);
        }
    }

    void StartWidget::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        int fs = qMin(width(), height()) / 28;
        fs = qBound(12, fs, 22);
        if (m_startBtn)
        {
            QFont btnFont = m_startBtn->font();
            btnFont.setPointSize(fs);
            btnFont.setBold(true);
            m_startBtn->setFont(btnFont);
            m_instructionsBtn->setFont(btnFont);
        }
    }

    void StartWidget::ShowInstructionsDialog()
    {
        QDialog* dialog = new QDialog(this);
        dialog->setWindowTitle("玩法介绍");
        dialog->setFixedSize(540, 520);
        dialog->setStyleSheet(
            "QDialog {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 #1e1c2e, stop:1 #161428);"
            "  color: #d0d0e0;"
            "}");

        QVBoxLayout* layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(20, 16, 20, 16);

        QTextEdit* text = new QTextEdit(dialog);
        text->setReadOnly(true);
        text->setFrameShape(QFrame::NoFrame);
        text->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        text->setStyleSheet(
            "QTextEdit {"
            "  background: transparent;"
            "  color: #c8c0e0;"
            "  font-size: 13px;"
            "  line-height: 1.6;"
            "  selection-background-color: #5a50a8;"
            "}");

        text->setHtml(R"(
<style>
  body { color: #c8c0e0; font-size: 13px; line-height: 1.6; }
  h2 { color: #b0a0e8; font-size: 15px; margin-top: 12px; }
  b { color: #d0c8f0; }
</style>
<h2>游戏目标</h2>
<p>共 <b>15 波</b> PvE 回合，击败全部波次即为通关。第 15 波为 Boss 战。</p>

<h2>准备阶段</h2>
<p>商店刷出 5 个随机单位 → 点击购买 → 拖拽上棋盘 → 点击「开始战斗」。</p>

<h2>战斗阶段</h2>
<p>单位自动寻路、攻击、施法。胜利得金币，失败扣血（HP=0 则游戏结束）。</p>

<h2>经济系统</h2>
<p>胜利金币：前 10 轮 <b>波数 × 2</b>，第 11 轮起 <b>波数 × 2 + 5</b><br>
失败补偿：<b>波数 × 2 + 5</b> 金币<br>
扣血：<b>波数 × 4</b> | 胜利回血：<b>波数 × 2</b>（HP≠100 时）</p>

<h2>合星</h2>
<p><b>3 个同名 1★ → 2★</b>（属性 ×1.8）<br>
<b>3 个同名 2★ → 3★</b>（属性 ×3.6）</p>

<h2>羁绊</h2>
<p>同羁绊 ≥2 激活一档、≥4 激活二档，战斗前为全队加属性。<br>
共 10 种羁绊：人类、战士、兽人、精灵、远程、法师、骑士、治疗、刺客、侍从。</p>

<h2>棋盘席位</h2>
<p>前 3 波最多部署 <b>8 人</b>。第 4 波起可购买额外席位：<br>
第 4-9 波：<b>5 金币/席位</b> | 第 10 波起：<b>10 金币/席位</b><br>
购买后永久提升部署上限（右上角显示当前席位）。</p>

<h2>操作</h2>
<p>点击商店卡片 → 确认购买 | 拖拽部署/撤回<br>
右键备战区快速出售 | 第 1 次刷新免费，之后递增 1 金币</p>
<h2>全屏模式</h2>
	<p>启动即全屏。按 <b>F11</b> 切换全屏/窗口，全屏时按 <b>ESC</b> 退回窗口。<br>
	右上角最大化 = 全屏，最小化可缩到任务栏，恢复后回到缩小前状态。<br>
	全屏时 16:9 背景无裁剪，窗口模式自动适配。</p>
)");

        layout->addWidget(text);

        QPushButton* closeBtn = new QPushButton("知道了", dialog);
        closeBtn->setStyleSheet(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "    stop:0 #5a50a8, stop:1 #4a4080);"
            "  color: white; border: none; border-radius: 6px;"
            "  padding: 10px 36px; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
            "    stop:0 #7a70c8, stop:1 #6a60b0); }");
        closeBtn->setCursor(Qt::PointingHandCursor);
        connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
        layout->addWidget(closeBtn, 0, Qt::AlignCenter);

        dialog->exec();
        dialog->deleteLater();
    }

} // namespace synera
