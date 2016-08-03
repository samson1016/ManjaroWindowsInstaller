#include "manjarowindowsinstaller.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QString>

QString detectDefaultLanguage();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;

    translator.load("lang/" + detectDefaultLanguage());

    app.installTranslator(&translator);

    ManjaroWindowsInstaller window;
    window.show();

    return app.exec();
}

QString detectDefaultLanguage()
{
    QLocale systemLocale;
    int language = systemLocale.language();
    QString Q_language = QString::number(language);
    return Q_language;
}
