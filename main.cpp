#include "mainwindow.h"

#include <QApplication>
#include <QtWidgets/qstylefactory.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    MainWindow w;
    w.show();
    return a.exec();
}
