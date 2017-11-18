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
    QList<int> qListInt = QList<int>();
    // initialize QMaps;
    positions1 = QMap<int, int>();
    positions2 = QMap<int, int>();
    changedPositions = QMap<int, int>();
    userInfo = QMap<int, QString>();
    downline_ids1 = QMap<int, QList<int> >();
    downline_ids2 = QMap<int, QList<int> >();
    int user_id = 0;
    int upline_id = 0;

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
    QSqlQuery q1("SELECT user_ptr_id,position_id,upline_id_id FROM payout_userprofile");
    while(q1.next())
    {
        user_id = q1.value(0).toInt();
        upline_id = q1.value(2).toInt();
        positions1[user_id] = 1;
        if(!downline_ids1.keys().contains(upline_id))
        {
            qListInt = QList<int>();
            qListInt.append(user_id);
        } else {
            qListInt = downline_ids1.value(upline_id);
            qListInt.append(user_id);
        }
        downline_ids1.insert(upline_id, qListInt);
        // upline_ids1[q1.value(0).toInt()] = q1.value(2).toInt();
        // QString toWrite = q1.value(0).toString() + QString(":") + q1.value(1).toString();
        // writeToFile(csv_file01, toWrite);
    }
    std::cout << "This positions collection has this number of record: " << positions1.count() << std::endl;
    db.close();
    // update the positions in this one.
    this->updatePositions(&positions1, &downline_ids1);
    
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
    QSqlQuery q2("SELECT user_ptr_id,position_id,upline_id_id FROM payout_userprofile");
    while(q2.next())
    {
        user_id = q2.value(0).toInt();
        upline_id = q2.value(2).toInt();
        positions2[user_id] = 1;
        if(!downline_ids2.keys().contains(upline_id))
        {
            qListInt = QList<int>();
            qListInt.append(user_id);
        } else {
            qListInt = downline_ids2.value(upline_id);
            qListInt.append(user_id);
        }
        downline_ids2.insert(upline_id, qListInt);
    }
    std::cout << "This positions collection has this number of record: " << positions2.count() << std::endl;
    // update the positions in this one.
    this->updatePositions(&positions2, &downline_ids2);
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

void Task::updatePositions(intIntMap *positions, intIntListMap *downlineCollection)
{
    int user_id = 0;
    int downline_user_id=0;
    int downline_position_id=0;
    int qualified_downlines = 0;
    int current_position = 0;
    int position_matching = 0;
    int match_position = 1;
    intIntMap positions_temp = *positions;
    QMap<int, int>::iterator iiter; // loop iterator (i)
    QList<int> downlines = QList<int>();
    // loop for all positions check if there are positions that have
    while(true)
    {
        // positions matching. if this counter is still zero after the for loop
        // below, we can break from the loop expecting there are no more
        // users with higher positions
        position_matching = 0;
        // loop through all the users
        for(iiter=positions->end(); iiter != positions->begin(); iiter--)
        {
            // get the user id
            user_id = iiter.key();
            // get the current position of this user
            current_position = iiter.value();
            // qualified_downlines to 0
            qualified_downlines = 0;
            // if the current position of this user is not equal to the
            // matching position, go to the next user
            if(current_position != match_position) continue;
            // since the current position matches the match position, we increment position_matching
            position_matching++;
            downlines = downlineCollection->value(user_id);
            // loop through the downlines. check if there are users with matching positions
            for(int i=0;i < downlines.size(); ++i)
            {
                downline_user_id = downlines.at(i);
                downline_position_id = positions->value(downline_user_id);
                if(downline_position_id == position_matching)
                {
                    qualified_downlines++;
                }
            }
            if(qualified_downlines >= promote_qualifier)
            {
                current_position++;
                positions_temp.insert(user_id, current_position);
            }
        }
        std::cout << "Total Matching This Position : " << position_matching << std::endl;
        std::cout << "Gathering Next Position : " << match_position << std::endl;
        match_position++;
        if(position_matching <= 0) break;   // break if there are no more matching positions
        // we are almost always sure that if match_position reached 20
        // there will be chaos after 21 due to complexity of the MLM structure
        if(match_position > 20) break;
    }
    *positions = positions_temp;
}

