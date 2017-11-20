#include <QtCore>
#include "task.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Task *task = new Task(&a);
    if (argc < 3)
    {
        task->exitUsage(1);
    }
    task->startDate = QString(argv[1]);
    task->endDate = QString(argv[2]);
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, task, SLOT(run()));
    return a.exec();
}

