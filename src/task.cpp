#include "task.h"
#include <QDebug>
#include <QProcess>
#include <QString>
#include <QSettings>
#include <QDateTime>

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
    upline_ids1 = QMap<int, int>();
    upline_ids2 = QMap<int, int>();

    // initialize report output
    QDateTime dTime = QDateTime::currentDateTime();
    QString reportFile = QString("/root/reports-positions-%1-%2-%3-%4-%5-%6.csv").arg(QString::number(dTime.date().year()),
                                                                                      QString::number(dTime.date().month()),
                                                                                      QString::number(dTime.date().day()),
                                                                                      QString::number(dTime.time().hour()),
                                                                                      QString::number(dTime.time().minute()),
                                                                                      QString::number(dTime.time().second()));
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
    QString shellCommand = QString("/usr/bin/mysql -S /var/lib/mysql/mysql.sock -u %1 -p%2 nourskin_playpen < %3").arg(username, password, startMonthFile);
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
    std::cout << "Getting data..." << std::endl;
    QSqlQuery q1("SELECT user_ptr_id,position_id,upline_id_id FROM userprofile");
    while(q1.next())
    {
        positions1[q1.value(0).toInt()] = q1.value(1).toInt();
        upline_ids1[q1.value(0).toInt()] = q1.value(2).toInt();
        QString toWrite = q1.value(0).toString() + QString(":") + q1.value(1).toString();
        writeToFile(csv_file01, toWrite);
    }
    std::cout << "This positions collection has this number of record: " << positions1.count() << std::endl;
    db.close();
    // update the positions in this one.
    this->updatePositions(&positions1, &upline_ids1);
    
    // flush the second file;
    shellCommand = QString("/usr/bin/mysql -S /var/lib/mysql/mysql.sock -u %1 -p%2 nourskin_playpen < %3").arg(username, password, endMonthFile);
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
    std::cout << "Getting data..." << std::endl;
    QSqlQuery q2("SELECT user_ptr_id,position_id FROM userprofile");
    while(q2.next())
    {
        positions2[q2.value(0).toInt()] = q2.value(1).toInt();
        upline_ids2[q1.value(0).toInt()] = q1.value(2).toInt();
        QString toWrite = q2.value(0).toString() + QString(":") + q2.value(1).toString();
        writeToFile(csv_file02, toWrite);
    }
    std::cout << "This positions collection has this number of record: " << positions2.count() << std::endl;
    // update the positions in this one.
    this->updatePositions(&positions2, &upline_ids2);
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
    std::cout << std::endl << "Number of changed positions: " << changedPositions.count() << std::endl << std::endl;
    if(changedPositions.count() > 0)
    {
        for(QMap<int, int>::iterator iiter = changedPositions.begin();iiter != changedPositions.end(); ++iter)
        {
            int id = iiter.key();
            int position = iiter.value();
            QString writeThis = QString("%1:%2").arg(id, position);
            writeToFile(reportFile, writeThis);
        }
    }

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

void Task::updatePositions(intIntMap *positions, intIntMap *uplines)
{
    int user_id = 0;
    int tmp_user_id = 0;
    int upline_id = 0;
    int uplines_count = 0;
    int current_position = 0;
    int position_matching = 0;
    int match_position = 1;
    bool promote = false;
    QMap<int, int>::iterator iiter; // loop iterator (i)
    QMap<int, int>::iterator jiter; // loop iterator (j)
    // loop 1 set all positions to 1
    for(iiter =positions->begin(); iiter != positions->end(); ++iiter)
    {
        // std::cout << "ID:" << iiter.key() << "VALUE:" << iiter.value() << std::endl;
        positions->value(iiter.key(), 1);
    }
    // loop for all positions check if there are positions that have
    while(true)
    {
        position_matching = 0;
        for(iiter=positions->begin(); iiter != positions->end(); ++iiter)
        {
            std::cout << "ID:" << iiter.key() << "VALUE:" << iiter.value() << std::endl;
            user_id = iiter.key();
            current_position = iiter.value();
            if(current_position < match_position) continue;
            position_matching++;
            uplines_count = 0;
            promote = false;
            for(jiter=uplines->begin();jiter != uplines->end();++jiter)
            {
                upline_id = jiter.key();
                tmp_user_id = jiter.value();
                // this user does not have a position
                if(!positions->keys().contains(tmp_user_id)) continue;
                // this user's position is not match_position!
                if(positions->value(tmp_user_id) < match_position)
                if(user_id == upline_id)
                    uplines_count++;
                if(uplines_count >= promote_qualifier)
                {
                    promote = true;
                    break;
                }
            }
            if(promote)
            {
                current_position++;
                positions->value(user_id, current_position);
            }
        }
        std::cout << "Gathering Next Position : " << match_position << std::endl;
        match_position++;
        if(position_matching <= 0) break;   // break if there are no more matching positions
        if(match_position > 20) break;      // if this is reached!
    }

}

