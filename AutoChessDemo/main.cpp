#include "AutoChessDemo.h"
#include <QtWidgets/QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 全局字号微调：提高可读性
    QFont appFont = app.font();
    appFont.setPointSize(10);
    app.setFont(appFont);

    AutoChessDemo window;
    window.show();
    return app.exec();
}
