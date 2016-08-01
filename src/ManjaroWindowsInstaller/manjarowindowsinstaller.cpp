#include "manjarowindowsinstaller.h"
#include "ui_manjarowindowsinstaller.h"
#include "iostream"
#include "fstream"
#include "qstring.h"

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
    switch(ui->tabWidget->currentIndex())
    {
        case 0:
            ui->tabWidget->setCurrentIndex(1);
            break;
        case 1:
            ui->tabWidget->setCurrentIndex(2);
            proceedInstallation();
            ui->tabWidget->setCurrentIndex(3);
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
void ManjaroWindowsInstaller::proceedInstallation()
{
    //Export config
        //Q-styled string
        QString Qlanguage = ui->comboBox->currentText();
        QString Qdrive = ui->comboBox_2->currentText();
        QString QinstallationSize = ui->plainTextEdit_3->toPlainText();
        QString Qusername = ui->plainTextEdit->toPlainText();
        QString Qpassword = ui->plainTextEdit_2->toPlainText();
        //Convert to standard string
        std::string language = Qlanguage.toUtf8().constData();
        std::string drive = Qdrive.toUtf8().constData();
        std::string installationSize = QinstallationSize.toUtf8().constData();
        std::string username = Qusername.toUtf8().constData();
        std::string password = Qpassword.toUtf8().constData();

        std::ofstream configFile;
        configFile.open("manjaro.config");
        configFile << language << std::endl;
        configFile << drive << std::endl;      //it need to be converted to UUID or something like that plus a partition number.
        configFile << installationSize << std::endl;
        configFile << username << std::endl;
        configFile << password << std::endl;
        configFile.close();
        system("mkdir %SYSTEMDRIVE%\\manjaro");
        system("copy manjaro.config %SYSTEMDRIVE%\\manjaro\\manjaro.config");

    //Make Manjaro ISO bootable.
        //copy grub4dos folder and manjaro ISO to C:\manjaro
        system("copy manjaro.iso %SYSTEMDRIVE%\\manjaro\\manjaro.iso");
        system("xcopy grub4dos %SYSTEMDRIVE%\\grub4dos /s /e");

        //edit boot.ini to boot grldr file (TBD)

}
