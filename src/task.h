#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QtSql>
#include <QMap>
#include <QList>
#include <QString>
#include <QDateTime>
#include <iostream>

typedef QMap<int, float> floatMap;

class Task : public QObject
{
    Q_OBJECT
public:
    explicit Task(QObject *parent = 0);
    QString startMonthFile;
    QString endMonthFile;
signals:
    void finished(void);
public slots:
    void run(void);
private:
    QSqlDatabase db;

    QMap<int, int> positions1;
    QMap<int, int> positions2;
    QMap<int, int> changedPositions;
    QMap<int, QString> userInfo;

    void writeToFile(QString, QString);
};

#endif // TASK_H

