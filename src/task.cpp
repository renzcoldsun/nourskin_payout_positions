#include "task.h"
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QSettings>

Task::Task(QObject *parent) :
    QObject(parent)
{
}

void Task::run(void)
{
    QFile Fout1(startMonthFile);
    QFile Fout2(endMonthFile);
    QProcess shellProcess;
    QSettings settings("/etc/nourskin_positions.ini", QSettings::IniFormat);
    QString csv_file01 = "/tmp/positions_csv_out_01.txt";
    QString csv_file02 = "/tmp/positions_csv_out_02.txt";
    // initialize QMaps;
    positions1 = QMap<int, int>();
    positions2 = QMap<int, int>();
    changedPositions = QMap<int, int>();
    userInfo = QMap<int, QString>();

    settings.beginGroup("positions");
    QString hostname = settings.value("hostname").toString();
    QString username = settings.value("username").toString();
    QString password = settings.value("password").toString();

    // check out the files supplied
    if(!Fout1.exists())
    {
        std::cout << "I am not able to see this file: " << startMonthFile.toStdString() << std::endl;
        exit(1);
    } else {
        std::cout << "file exists : " << startMonthFile.toStdString() << std::endl;
    }
    if(!Fout2.exists())
    {
        std::cout << "I am not able to see this file: " << endMonthFile.toStdString() << std::endl;
        exit(1);
    } else {
        std::cout << "file exists : " << endMonthFile.toStdString() << std::endl;
    }
    // flush the first file;
    QString shellCommand = QString("/usr/bin/mysql -S /var/lib/mysql/mysql.sock -u %1 -p%2 nourskin_playpen < %1").arg(username, password, startMonthFile);
    std::cout << "Running: " << shellCommand.toStdString() << std::endl;
    shellProcess.start("/usr/bin/bash");
    shellProcess.write(shellCommand.toStdString().c_str());
    shellProcess.waitForFinished();
    shellProcess.close();

    // connect
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(hostname);
    db.setUserName(username);
    db.setPassword(password);
    db.setDatabaseName("nourskin_playpen");
    db.setPort(3306);
    if(!db.open())
    {
        std::cout << "Cannot connect to the database" << std::endl;
        exit(1);
    }
    // get the positions
    QSqlQuery q1("SELECT user_ptr_id,position_id FROM userprofile");
    while(q1.next())
    {
        positions1[q1.value(0).toInt()] = q1.value(1).toInt();
        QString toWrite = q1.value(0).toString() + QString(":") + q1.value(1).toString();
        writeToFile(csv_file01, toWrite);
    }
    std::cout << "This positions collection has this number of record: " << positions1.count() << std::endl;
    db.close();
    
    // flush the second file;
    shellCommand = QString("/usr/bin/mysql -S /var/lib/mysql/mysql.sock -u %1 -p%2 nourskin_playpen < %1").arg(username, password, endMonthFile);
    std::cout << "Running: " << shellCommand.toStdString() << std::endl;
    shellProcess.start("/usr/bin/bash");
    shellProcess.write(shellCommand.toStdString().c_str());
    shellProcess.waitForFinished();
    shellProcess.close();
    // reopen
    if(!db.open())
    {
        std::cout << "Cannot connect again to the database" << std::endl;
        exit(1);
    }
    // get the positions again
    QSqlQuery q2("SELECT user_ptr_id,position_id FROM userprofile");
    while(q2.next())
    {
        positions2[q2.value(0).toInt()] = q2.value(1).toInt();
        QString toWrite = q2.value(0).toString() + QString(":") + q2.value(1).toString();
        writeToFile(csv_file02, toWrite);
    }
    std::cout << "This positions collection has this number of record: " << positions2.count() << std::endl;
    // compare the positions
    QMap<int, int>::iterator iter;
    for(iter = positions2.begin();iter != positions2.end(); ++iter)
    {
        int id = iter.key();
        int position = iter.value();
        if(positions1.contains(id))
        {
            // this user already exists. check if this user
            // has a changed position
            int old_position = positions1[id];
            if(position != old_position)
                changedPositions[id] = position;
        } else {
            // this user does not exist previously.
            // just add to the changedPositions variable
            changedPositions[id] = position;
        }
    }

    // we got a list of positions. let's get the info to save to a CSV
    std::cout << "Number of changed positions: " << changedPositions.count() << std::endl;

    emit finished();
}

void Task::writeToFile(QString filename, QString toWrite)
{
    QFile outputFile(filename);
    outputFile.open(QIODevice::WriteOnly | QIODevice::Append);
    if(!outputFile.isOpen())
    {
        std::cout << "Unable to write to file" << filename.toStdString() << std::endl;
        exit(1);
    }
    QTextStream outStream(&outputFile);
    outStream << toWrite << QString("\n");
    outputFile.close();
}
