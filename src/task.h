#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QtSql>
#include <QMap>
#include <QList>
#include <QString>
#include <QDateTime>
#include <iostream>

#define promote_qualifier 5

typedef QMap<int, float> floatMap;
typedef QMap<int, int> intIntMap;
typedef QMap<int, QList<int> > intIntListMap;

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
    intIntListMap downline_ids1, downline_ids2;

    void updatePositions(intIntMap *, intIntListMap *uplines);
    void writeToFile(QString, QString);
};

#endif // TASK_H

