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

void ManjaroWindowsInstaller::on_pushButton_clicked(bool checked)
{
    if (checked==true)
    {
        switch(ui->tabWidget->currentIndex())  //Don't know how to call the currentIndex()-a property of the QTabWidget.
        {
            case 0:
                ui->tabWidget->setCurrentIndex(1);
                break;
            case 1:
                ui->tabWidget->setCurrentIndex(2);
                proceedInstallation();
                break;
            case 2:
                break;
            case 3:
                system("shutdown -r -t 0");
                break;
            default:
                break;
        }
    }
}
void proceedInstallation()
{

}
