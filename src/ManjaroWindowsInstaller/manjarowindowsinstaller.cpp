#include "manjarowindowsinstaller.h"
#include "ui_manjarowindowsinstaller.h"
#include "iostream"
#include "fstream"
#include "QString"
#include "QStringList"
#include "QDir"
#include "QList"

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
            getDriveList();
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
        ui->label_15->setText(tr("Exporting your configurations.."));
        ui->progressBar->setValue(0);

        //get system country
        QLocale systemLocale;
        int country = systemLocale.country();

        //Q-styled string
        QString Qlanguage = ui->comboBox->currentText();
        QString Qdrive = ui->comboBox_2->currentText();
        QString QinstallationSize = ui->plainTextEdit_3->toPlainText();
        QString Qusername = ui->plainTextEdit->toPlainText();
        QString Qpassword = ui->plainTextEdit_2->toPlainText();
        ui->progressBar->setValue(20);

        //Convert to standard string
        std::string language = Qlanguage.toUtf8().constData();
        std::string drive = Qdrive.toUtf8().constData();
        std::string installationSize = QinstallationSize.toUtf8().constData();
        std::string username = Qusername.toUtf8().constData();
        std::string password = Qpassword.toUtf8().constData();
        ui->progressBar->setValue(40);

        //Write configurations into file.
        std::ofstream configFile;
        configFile.open("manjaro.config");
        configFile << country << std::endl;
        configFile << language << std::endl;
        configFile << drive << std::endl;
        configFile << installationSize << std::endl;
        configFile << username << std::endl;
        configFile << password << std::endl;
        configFile.close();
        system("mkdir %SYSTEMDRIVE%\\manjaro");
        system("copy manjaro.config %SYSTEMDRIVE%\\manjaro\\manjaro.config");
        ui->progressBar->setValue(50);

        //Implant the installation flag.(To identify the target partition)
        ui->label_15->setText(tr("Creating the installation flag......"));
        configFile.open(drive + ":\\flag");
        configFile << "This is the Manjaro Linux's installation flag." << std::endl;
        configFile.close();
        ui->progressBar->setValue(60);

    //Make Manjaro ISO bootable by Windows's boot loader.
        //copy grub4dos folder and manjaro ISO to C:\manjaro
        ui->label_15->setText(tr("Installing grub4dos.."));
        system("move manjaro.iso %SYSTEMDRIVE%\\manjaro\\manjaro.iso");
        system("xcopy grub4dos %SYSTEMDRIVE%\\grub4dos /s /e");
        ui->progressBar->setValue(75);

        //edit boot.ini to boot grldr file: add C:\manjaro\grub4dos\grldr="Manjaro Linux"
        ui->label_15->setText(tr("Editing boot.ini options.."));
        std::ofstream changeBootINI("%SYSTEMDRIVE%\\boot.ini", std::ios_base::app | std::ios_base::out);
        changeBootINI << "\n" << "%SYSTEMDRIVE%\\manjaro\\grub4dos\\grldr=\"ManjaroLinux\""<< std::endl;
        changeBootINI.close();
        ui->progressBar->setValue(100);
}


void ManjaroWindowsInstaller::getDriveList()
{
    QDir my_Dir;
    QList<QFileInfo> m_Drives = my_Dir.drives();
    QStringList driveList;
    foreach( QFileInfo item, m_Drives )
        driveList << item.path();

    ui->comboBox_2->addItems(driveList);
}
