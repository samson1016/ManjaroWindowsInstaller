#include "manjarowindowsinstaller.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ManjaroWindowsInstaller w;
    w.show();

    return a.exec();
}
