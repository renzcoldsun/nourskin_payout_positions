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
    QSettings settings("/etc/nourskin_positions.ini", QSettings::IniFormat);
    QList<int> qListInt = QList<int>();
    QMap<int, int>::iterator iter;
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
    std::cout << "Possible outcome will be stored in this file: " << reportFile.toStdString() << std::endl << std::endl;
    if(!this->verifyDate(this->startDate))
        this->exitUsage(2);
    if(!this->verifyDate(this->endDate))
        this->exitUsage(3);

    this->startDate = QString("%1 23:59:59").arg(this->startDate);
    this->endDate = QString("%1 23:59:59").arg(this->endDate);

    // connect
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(hostname);
    db.setUserName(username);
    db.setPassword(password);
    db.setDatabaseName("nourskin");
    db.setPort(3306);
    if(!db.open())
    {
        std::cout << "Cannot connect to the database" << std::endl;
        exit(4);
    }
    // get the positions
    std::cout << "Getting data..." << std::endl;
    QString sqlQuery1 = QString("SELECT user_ptr_id,position_id,upline_id_id FROM userprofile WHERE date_joined <= '%1'").arg(this->startDate);
    QSqlQuery q1(sqlQuery1);
    while(q1.next())
    {
        user_id = q1.value(0).toInt();
        upline_id = q1.value(2).toInt();
        positions1[user_id] = 1;
        qListInt = QList<int>();
        if(!downline_ids1.keys().contains(upline_id))
        {
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
    
    // reopen
    if(!db.open())
    {
        std::cout << "Cannot connect again to the database" << std::endl;
        exit(5);
    }
    // get the positions again
    std::cout << "Getting data..." << std::endl;
    QString sqlQuery2 = QString("SELECT user_ptr_id,position_id,upline_id_id FROM userprofile WHERE date_joined <= '%1'").arg(this->endDate);
    QSqlQuery q2(sqlQuery2);
    while(q2.next())
    {
        user_id = q2.value(0).toInt();
        upline_id = q2.value(2).toInt();
        positions2[user_id] = 1;
        qListInt = QList<int>();
        if(!downline_ids2.keys().contains(upline_id))
        {
            qListInt.append(user_id);
        } else {
            qListInt = downline_ids2.value(upline_id);
            qListInt.append(user_id);
        }
        downline_ids2.insert(upline_id, qListInt);
    }
    std::cout << "This positions collection has this number of record: " << positions2.count() << std::endl;
    // update the positions in this one.
    this->updatePositions(1);
    // update the positions in this one.
    this->updatePositions(2);
    // compare the positions

    for(iter = positions2.begin();iter != positions2.end(); ++iter)
    {
        int id = iter.key();
        if(!positions1.keys().contains(id))
        {
            changedPositions.insert(id, positions2.value(id));
            continue;
        }
        if(positions1.value(id) != positions2.value(id))
        {
            changedPositions.insert(id, positions2.value(id));
            continue;
        }
    }

    // we got a list of positions. let's get the info to save to a CSV

    if(changedPositions.count() > 0)
    {
        std::cout << std::endl << "Number of changed positions: " << changedPositions.count() << std::endl << std::endl;
        for(QMap<int, int>::iterator iiter = changedPositions.begin();iiter != changedPositions.end(); ++iter)
        {
            int id = iiter.key();
            int position = iiter.value();
            QString writeThis = QString("%1:%2").arg(id, position);
            writeToFile(reportFile, writeThis);
        }
    } else {
        std::cout << "There are no changed positions." << std::endl;
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
        exit(10);
    }
    QTextStream outStream(&outputFile);
    outStream << toWrite << QString("\n");
    outputFile.close();
}

void Task::updatePositions(int where)
{
    int user_id = 0;
    int user_position_id = 0;
    int downline_qualified = 0;
    if(where == 1)
    {
        for(int pos=1;pos<final_position;pos++)
        {
            // std::cout << "Working on Position :: " << pos << std::endl;
            for(QMap<int, int>::iterator userIter=positions1.begin();userIter != positions1.end();++userIter)
            {
                downline_qualified = 0;
                user_id = userIter.key();
                user_position_id = userIter.value();
                if(user_position_id != pos) continue;
                //std::cout << std::endl << "Working on user id: " << user_id << " :: " ;
                foreach(int downline_id, downline_ids1.value(user_id))
                {
                    int downline_position = positions1.value(downline_id);
                    if(downline_position == pos) ++downline_qualified;
                    // std::cout << downline_id << " ";
                    if(downline_qualified >= promote_qualifier)
                    {
                        positions1.insert(user_id, user_position_id + 1);
                        break;
                    }
                }

            }
        }
    } else if(where == 2)
    {
        for(int pos=1;pos<final_position;pos++)
        {
            // std::cout << "Working on Position :: " << pos << std::endl;
            for(QMap<int, int>::iterator userIter=positions2.begin();userIter != positions2.end();++userIter)
            {
                downline_qualified = 0;
                user_id = userIter.key();
                user_position_id = userIter.value();
                if(user_position_id != pos) continue;
                // std::cout << std::endl << "Working on user id: " << user_id << " :: " ;
                foreach(int downline_id, downline_ids2.value(user_id))
                {
                    int downline_position = positions2.value(downline_id);
                    if(downline_position == pos) ++downline_qualified;
                    //std::cout << downline_id << " ";
                    if(downline_qualified >= promote_qualifier)
                    {
                        positions2.insert(user_id, user_position_id + 1);
                        break;
                    }
                }

            }
        }
    } else {
        return;
    }
}

void Task::exitUsage(int exitCode)
{
    std::cout << "Not enough arguments" << std::endl;
    std::cout << "./positions <start date yyyy-MM-dd> <end date yyyy-MM-dd>" << std::endl;
    exit(exitCode);
}

bool Task::verifyDate(QString date)
{
    QStringList dateSeparated;
    dateSeparated = date.split('-');
    if(dateSeparated.count() != 3)
    {
        qDebug() << dateSeparated;
        return false;
    }
    return false;
}
