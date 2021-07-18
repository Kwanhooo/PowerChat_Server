#include "powerserver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PowerServer w;
    w.show();

    return a.exec();
}
