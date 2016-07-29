#include "manjarowindowsinstaller.h"
#include "ui_manjarowindowsinstaller.h"

ManjaroWindowsInstaller::ManjaroWindowsInstaller(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ManjaroWindowsInstaller)
{
    ui->setupUi(this);
}

ManjaroWindowsInstaller::~ManjaroWindowsInstaller()
{
    delete ui;
}

void ManjaroWindowsInstaller::on_pushButton_clicked()
{

}
