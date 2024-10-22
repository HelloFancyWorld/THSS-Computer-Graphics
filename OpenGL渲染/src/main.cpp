#include "QtOpenGLDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtOpenGLDemo w;
    w.show();
    return a.exec();
}
