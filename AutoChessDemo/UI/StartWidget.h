#pragma once

#include <QWidget>
#include <QPushButton>
#include <QPixmap>

namespace synera
{
    class StartWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit StartWidget(QWidget* parent = nullptr);

        // 设置背景图片（外部传入 PNG 路径）
        void SetBackgroundImage(const QString& imagePath);

    signals:
        void StartGame();           // 点击「开始游戏」
        void ShowInstructions();    // 点击「玩法介绍」

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void ShowInstructionsDialog();  // 弹出玩法介绍窗口

        QPushButton* m_startBtn = nullptr;
        QPushButton* m_instructionsBtn = nullptr;
        QPixmap m_background;           // 背景图片
    };
}
