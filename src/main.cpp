#include "mainwindow.h"

#include <QApplication>

#include <time.h>

#ifdef WIN32
#include <Windows.h>
#endif

void win32_hacks();

int main(int argc, char *argv[])
{
    win32_hacks();
    qsrand(time(0));

    QApplication::setOrganizationName("AimFans");
    QApplication::setOrganizationDomain("aim-fans.ru");
    QApplication::setApplicationName("Polygon 4 DB Tool");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}

void win32_hacks()
{
#ifdef WIN32
    //SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}