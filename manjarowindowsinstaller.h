#ifndef MANJAROWINDOWSINSTALLER_H
#define MANJAROWINDOWSINSTALLER_H

#include <QMainWindow>

namespace Ui {
class ManjaroWindowsInstaller;
}

class ManjaroWindowsInstaller : public QMainWindow
{
    Q_OBJECT

public:
    explicit ManjaroWindowsInstaller(QWidget *parent = 0);
    ~ManjaroWindowsInstaller();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_clicked(bool checked);

private:
    Ui::ManjaroWindowsInstaller *ui;
};

#endif // MANJAROWINDOWSINSTALLER_H
