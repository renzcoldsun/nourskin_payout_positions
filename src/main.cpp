#include <QtCore>
#include "task.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Task *task = new Task(&a);
    if (argc < 3)
    {
        std::cout << "Not enough arguments" << std::endl;
        std::cout << "positions <sql_start_of_the_month> <sql_end_of_the_month>" << std::endl;
        exit(1);
    }
    task->startMonthFile = QString(argv[1]);
    task->endMonthFile = QString(argv[2]);
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, task, SLOT(run()));
    return a.exec();
}

