#include "manjarowindowsinstaller.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ManjaroWindowsInstaller window;
    window.show();

    return app.exec();
}
