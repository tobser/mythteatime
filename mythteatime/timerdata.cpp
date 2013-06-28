#include "teatimeui.h"
#include "timerdata.h"

// myth
#include <dbutil.h>
#include <mythscreentype.h>
#include <mythcontext.h>
#include <mythevent.h>
#include <mythsystem.h>

// qt
#include <QtCore>
#include <QString>
#include <QCoreApplication>

#define tr(a) QObject::tr(a)

TimerData::TimerData(int id):
    Id(id),
    m_st(NULL),
    m_pd(NULL)
{
    initDefaults();
    initJumpDest();
}

TimerData::TimerData():
    Id(-2),
    m_st(NULL),
    m_pd(NULL)
{
}

TimerData::TimerData(TimerData * td ):
    Id(-3),
    m_st(NULL),
    m_pd(NULL)
{
    initDefaults();
    initJumpDest();
    if (td)
    {
        Id             = td->Id;
        Message_Text   = td->Message_Text;
        Exec_Date_Time = td->Exec_Date_Time;
        Date_Time      = td->Date_Time;
        Time_Span      = td->Time_Span;
        Exec_Actions   = td->Exec_Actions;
        Reoccurrence   = td->Reoccurrence;
        ShowMessage    = td->ShowMessage;
    }
}

void TimerData::initDefaults()
{
    Date_Time    = QDateTime::currentDateTime();
    Time_Span    = QTime(0,5,0);
    Reoccurrence = "time_span";
    ShowMessage  = true;
}

QString TimerData::toString(void)
{
    return QString ("TimerData[id=%1,reo=\"%7\",t=\"%2\",e=%3,ts=%4,dt=%5,a=\"%6\"]")
        .arg(Id)
        .arg(Message_Text)
        .arg(Exec_Date_Time.toString())
        .arg(Time_Span.toString())
        .arg(Date_Time.toString())
        .arg(Exec_Actions.count())
        .arg(Reoccurrence);
}

/**
 * @brief Initialize list of jumppoint and their assosiated screennames
 *
 * format:
 *  jumpDest["Point"] = "screenname";
 *
 * where "Point" ist the jumppoint name as it has to be provided to
 * GetMythMainWindow()->JumpTo(..) and the screenname hast to be
 * what you get if you are at the jumppoint and call
 * GetMythUI()->GetCurrentLocation()
 *
 **/
void TimerData::initJumpDest()
{
    jumpDest["Main Menu"]    = "mainmenu";

    // if option "Setup"->"Video"->"Program Guide"->"Show te program guide
    // when starting Live TV" is active, then the target window is always
    // guidegrid
    jumpDest["Live TV"] = "Playback";
    jumpDest["Live TV In Guide"] = "guidegrid";

    jumpDest["Video Browser"]  =
        jumpDest["Video Gallery"]  =
        jumpDest["Video Listings"] =
        jumpDest["Video Default"]  =
        jumpDest["Video Manager"]  = "mythvideo";

    jumpDest["Play music"]             = "playmusic";
    jumpDest["Select music playlists"] = "musicplaylists";
    jumpDest["Play radio stream"]      = "streamview";

    jumpDest["MythGallery"] = "mythgallery";
    jumpDest["MythWeather"] = "mythweather";
    jumpDest["MythNews"]    = "mythnews";

    jumpDest["Standby Mode"]  = "standbymode";
    jumpDest["Status Screen"] = "StatusBox";
}

void TimerData::toMap(InfoMap& map)
{
    map["text"] = Message_Text;

    QString actions;
    for (int i =0; i<Exec_Actions.count(); i++){
        if (i > 0)
            actions.append(", ");

        TeaAction a = Exec_Actions[i];
        actions += QString("%1.) ").arg(i+1);
        actions += a.Action_Type +" "+ a.Action_Data;
    }
    map["actions"] = actions;

    if (Reoccurrence.startsWith("time_span"))
    {
        map["timer_type"] = tr("Duration");
        map["time_span"] = Time_Span.toString();
        map["reoccurrence"] = "";
    }
    else
    {
        map["date_time"] = Date_Time.toString();
        if (Reoccurrence == "daily")
        {
            map["timer_type"] = tr("Daily");
        }
        else if (Reoccurrence == "one_shot")
        {
            map["timer_type"] = tr("One Shot");
        }
        else
        {
            map["timer_type"] = tr("Some Days");

            QString reocc;
            QStringList days = Reoccurrence.split(",", QString::SkipEmptyParts);
            for (int i = 0; i < days.count(); i++)
            {
                if (i > 0 )
                    reocc += ", ";
                reocc += QDate::longDayName(days[i].toInt());
            }
            map["reoccurrence"]= reocc;
        }
    }

    if (isActive())
    {
        QDateTime now = QDateTime::currentDateTime();
        int secs = now.secsTo(Exec_Date_Time);
        if (secs > 0)
        {
            int h=0 , m =0 , s =0;
            secsTo(&h, &m, &s, secs);
            QLatin1Char fill = QLatin1Char('0');
            map["time_left"] = QString("%1:%2:%3").arg(h,2,10,fill)
                                                  .arg(m,2,10,fill)
                                                  .arg(s,2,10,fill);
        }
    }
}

void TimerData::removeFromDb(void)
{
    if (Id < 0)
        return;

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("DELETE FROM `teatime_rundata` WHERE `timer_id` = :TID");
    query.bindValue(":TID",Id);

    if (!query.exec())
    {
        QString msg = QString("Could not delete actions of timer %1. %2")
            .arg(Id).arg(query.lastError().text());

        LOG_Tea(LOG_WARNING, msg);
        return;
    }
    LOG_Tea(LOG_INFO, QString("Actions for %1 deleted.").arg(Id));

    query.prepare("DELETE FROM `teatime` WHERE `Id` = :TID");
    query.bindValue(":TID", Id);
    if (!query.exec())
    {
        QString msg = QString("Could not delete timer %1. %2")
            .arg(Id).arg(query.lastError().text());

        LOG_Tea(LOG_WARNING, msg);
        return;
    }
    LOG_Tea(LOG_INFO, QString("Timer %1 deleted.").arg(Id));
}

bool TimerData::isActive(void)
{
    if (!Exec_Date_Time.isValid())
        return false;

    return (Exec_Date_Time >= QDateTime::currentDateTime());
}

bool TimerData::init(void)
{
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT "
            " message_text,"    // 0
            " exec_date_time, " // 1
            " date_time, "      // 2
            " time_span, "      // 3
            " reoccurrence, "   // 4
            " show_message "    // 5
            "from `teatime` WHERE id = :TID");
    query.bindValue(":TID", Id);

    if (!query.exec())
    {
        QString msg = QString("Read timer data for timer %1 failed. %2")
            .arg(Id).arg(query.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
        return false;
    }

    if(!query.next())
    {
        return false;
    }

    Message_Text   = query.value(0).toString();
    Exec_Date_Time = query.value(1).toDateTime();
    Date_Time      = query.value(2).toDateTime();
    Time_Span      = QTime::fromString(query.value(3).toString(), "hh:mm:ss");
    Reoccurrence   = query.value(4).toString();
    ShowMessage    = query.value(5).toBool();

    setNextExecutionTime();

    query.prepare("SELECT type,data FROM teatime_rundata "
            "WHERE timer_id = :TID ORDER BY run_order ASC");
    query.bindValue(":TID", Id);
    if (!query.exec())
    {
        QString msg = QString("Read rundata for timer %1 failed. %2")
            .arg(Id).arg(query.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
        return false;
    }

    while(query.next())
    {
        TeaAction a;
        a.Action_Type = query.value(0).toString();
        a.Action_Data = query.value(1).toString();
        Exec_Actions << a;
    }


    LOG_Tea(LOG_INFO, QString("%1 active: %2").arg(Id).arg(isActive()));

    return true;
}

void TimerData::setNextExecutionTime(bool startTimespanTimer )
{
    LOG_Tea(LOG_INFO, QString("setNextExecutionTime(startTs=%1,%2)")
            .arg(startTimespanTimer)
            .arg(Reoccurrence));
    if (Reoccurrence.isEmpty())
    {
        LOG_Tea(LOG_WARNING, QString("Reoccurrence not set!"));
        return;
    }

    QDateTime nextExec  = QDateTime::currentDateTime();
    QDateTime now       = QDateTime::currentDateTime();

    if (Reoccurrence == "daily")
    {
        if (!Exec_Date_Time.isValid() || now.secsTo(Exec_Date_Time) < 0)
        {
            nextExec.setTime(Date_Time.time());
            if (now.secsTo(nextExec) <0 )
                nextExec = nextExec.addDays(1);

            saveExecTimeToDb(nextExec);
            return;
        }
    }
    else if (Reoccurrence == "one_shot")
    {
        saveExecTimeToDb(Date_Time);
        return;
    }
    else if (Reoccurrence.startsWith("time_span"))
    {
        if (startTimespanTimer)
        {
            Reoccurrence = "time_span_active";
            nextExec = now.addSecs(Time_Span.hour() * 60 * 60)
                .addSecs(Time_Span.minute() * 60)
                .addSecs(Time_Span.second());
            saveExecTimeToDb(nextExec);
            return;
        }
        else if (Reoccurrence == "time_span_active")
        {
            // do nothing, use Exectime from DB
            return;   
        }
        else
        {
            Reoccurrence = "time_span";
            saveExecTimeToDb(QDateTime());
            return;
        }
    }
    else
    {
        int todayDoW = now.date().dayOfWeek();
        QStringList days = Reoccurrence.split(",", QString::SkipEmptyParts);
        for (int i = 0; i < days.count(); i++)
        {
            if (days[i].toInt() >= todayDoW)
            {
                nextExec = nextExec.addDays(days[i].toInt() - todayDoW);
                nextExec.setTime(Date_Time.time());
                if (QDateTime::currentDateTime() < nextExec)
                {
                    saveExecTimeToDb(nextExec);
                    return;
                }
            }
        }

        // this week is complete, => find first runtime next week
        int firstExecDoW = days[0].toInt();
        int daysInFuture =  (7-todayDoW) + firstExecDoW;
        nextExec = QDateTime::currentDateTime().addDays(daysInFuture);
        nextExec.setTime(Date_Time.time());

        saveExecTimeToDb(nextExec);
        return;
    }
}

void TimerData::saveExecTimeToDb(QDateTime nextExecTime)
{
    Exec_Date_Time = nextExecTime;

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare( "UPDATE `teatime` SET `exec_date_time` = :EXECT ,"
            "`reoccurrence` = :REOC "
            "WHERE `id` =:ID ");
    query.bindValue(":ID", Id);
    query.bindValue(":EXECT", Exec_Date_Time);
    query.bindValue(":REOC", Reoccurrence);

    if(!query.exec())
    {
        QString msg = QString("Could not store new execution time. %1")
            .arg(query.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
        return;
    }

    LOG_Tea(LOG_INFO, QString("Sheduled next execution of timer(id=%1,reoccurrence=%2)  to '%3'")
            .arg(Id).arg(Reoccurrence).arg(Exec_Date_Time.toString()));
}

void TimerData::exec(void)
{
    MythMainWindow *mainWin = GetMythMainWindow();
    if (!mainWin)
    {
        LOG_Tea(LOG_WARNING, "Could not get main window.");
        return;
    }

    int actionCnt = Exec_Actions.count();

    if (ShowMessage)
    {
        int wait = 2;
        QString popup_message = Message_Text;
        if (actionCnt > 0)
        {
            QString info = QString("\n\n%1 actions will be run in %2s")
                                    .arg(actionCnt)
                                    .arg(wait);
            popup_message.append(info);
        }

        LOG_Tea(LOG_INFO, QString("Popup: %1").arg(popup_message));

        // Pause playback (Will only work if your frontend contains the patches from ticket #10894).
        QStringList sl ;
        sl << "pauseplayback"; 

        MythEvent* me = new MythEvent(MythEvent::MythUserMessage, popup_message, sl);
        QCoreApplication::instance()->postEvent(mainWin, me);
        sleep(wait);
    }

    if (actionCnt == 0)
        return;

    LOG_Tea(LOG_INFO, "Executing actions.");

    m_pd->SetTotal(actionCnt);
    m_st->AddScreen(m_pd);

    for (int i=0; i < actionCnt; i++)
    {
        TeaAction a = Exec_Actions[i];
        m_pd->SetMessage(a.Action_Type +": " +a.Action_Data);
        m_pd->SetProgress(i);

        if (a.Action_Type == "JumpPoint")
        {
            jumpToAndWaitArrival(a.Action_Data);
        }
        else if (a.Action_Type == "SysEvent")
        {
            runSysEvent(a.Action_Data);
        }
        else if (a.Action_Type == "Command")
        {
            runCommand(a.Action_Data);
        }
    }

    m_pd->Close();
    m_pd = NULL;
    m_st = NULL;
}

void TimerData::runSysEvent(const QString & sysEventKey)
{
    if (sysEventKey.isEmpty())
        return;

    MSqlQuery q(MSqlQuery::InitCon());
    q.prepare("SELECT data FROM `settings` WHERE `hostname` = :HOST"
            " AND `value` = :EV_KEY ");
    q.bindValue(":HOST", gCoreContext->GetHostName());
    q.bindValue(":EV_KEY", sysEventKey);

    if (!q.exec())
    {
        QString msg("Could not load data for %1: %2");
        msg.arg(sysEventKey).arg(q.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
    }
    q.next();
    QString cmd = q.value(0).toString();
    LOG_Tea(LOG_INFO, QString("running sysevent '%1'").arg(sysEventKey));
    runCommand(cmd);
    LOG_Tea(LOG_INFO, QString("done."));
}

void TimerData::runCommand(const QString cmdStr)
{
    LOG_Tea(LOG_INFO, QString("running cmd '%1'").arg(cmdStr));
    QScopedPointer<MythSystem>  cmd(MythSystem::Create(cmdStr, (kMSStdIn|kMSStdOut|kMSStdErr)));
    cmd->Wait(0);
}

void TimerData::jumpToAndWaitArrival(const QString & target)
{
    LOG_Tea(LOG_INFO, QString("jumping to %1.").arg (target));

    GetMythMainWindow()->JumpTo(target);

    QString expected = jumpDest[target];
    QTime timer;
    timer.start();

    while (GetMythUI()->GetCurrentLocation() != expected)
    {
        if (timer.elapsed() > 10000)
        {
            LOG_Tea(LOG_WARNING, QString("%1 != %2")
                    .arg(GetMythUI()->GetCurrentLocation()).arg(expected));
            LOG_Tea(LOG_WARNING, QString("Jump to %1 not completed in time.")
                    .arg (target));
            return;
        }
        usleep(10000);
    }

    LOG_Tea(LOG_INFO, QString("arrived at %1.").arg (target));
}

void TimerData::execAsync(void)
{
    m_st = GetMythMainWindow()->GetStack("popup stack");
    if (!m_st)
    {
        LOG_Tea(LOG_WARNING, "Could not get \"popup stack\" to display "
                "progressdialog.");
        return;
    }

    int actionCnt = Exec_Actions.count();
    QString msg = tr("Executing %1 timer acions").arg(actionCnt);
    m_pd = new MythUIProgressDialog(msg, m_st, "exec_actions");
    m_pd->Create();

    QFuture<void> f = QtConcurrent::run(this, &TimerData::exec);

    if (Reoccurrence == "time_span_active")
        Reoccurrence = "time_span";

    setNextExecutionTime();
}

bool TimerData::saveToDb(bool startTimespanTimer)
{
    setNextExecutionTime(startTimespanTimer);

    MSqlQuery query(MSqlQuery::InitCon());

    if (Id < 0)
    {
        query.prepare("INSERT INTO `teatime` (`hostname`) VALUES (:HOST)");
        query.bindValue(":HOST", gCoreContext->GetHostName());
        if (!query.exec())
        {
            QString msg = QString("Could not create new timer row. %1")
                .arg(query.lastError().text());
            LOG_Tea(LOG_WARNING, msg);
            return false;
        }

        Id = query.lastInsertId().toInt();
    }

    query.prepare( "UPDATE `teatime` SET "
            "`message_text` = :TEXT "
            ", `exec_date_time` = :EXEC "
            ", `date_time` = :DATETIME "
            ", `time_span` = :TIMESPAN "
            ", `reoccurrence` = :REOCC "
            ", `show_message` = :SHOWMESS "
            "WHERE `id` =:ID ");

    if (Message_Text.isEmpty())
        query.bindValue(":TEXT", tr("Nameless timer"));
    else
        query.bindValue(":TEXT", Message_Text);

    query.bindValue(":REOCC", Reoccurrence);
    query.bindValue(":EXEC", Exec_Date_Time);
    query.bindValue(":DATETIME", Date_Time);
    query.bindValue(":TIMESPAN", Time_Span);
    query.bindValue(":ID", Id);
    query.bindValue(":SHOWMESS", (ShowMessage ? "1" : "0"));

    if(!query.exec())
    {
        QString msg = QString("Could not store new timer data. %1")
            .arg(query.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
        return false;
    }

    query.prepare("DELETE FROM `teatime_rundata` WHERE `timer_id` = :TID");
    query.bindValue(":TID", Id);
    query.exec();

    query.prepare("INSERT INTO `teatime_rundata` "
            "(timer_id, run_order, type, data) "
            " VALUES "
            "(:ID, :ORDER, :TYPE, :DATA)");

    for (int i = 0; i < Exec_Actions.count(); i ++)
    {
        query.bindValue(":ID", Id );
        query.bindValue(":ORDER", i );
        query.bindValue(":TYPE", Exec_Actions[i].Action_Type);
        query.bindValue(":DATA", Exec_Actions[i].Action_Data);
        if (!query.exec())
        {
            QString msg = QString("Could not action data. %1")
                .arg(query.lastError().text());
            LOG_Tea(LOG_WARNING, msg);
            return false;
        }
    }

    return true;
}

void TimerData::setTimeSpanFromSecs(int seconds)
{
    int h=0 , m =0 , s =0;
    secsTo(&h, &m, &s, seconds);
    Time_Span = QTime(h%24, m, s);
}

int TimerData::getTimespanMinutes(void)
{
    return Time_Span.hour() * 60 + Time_Span.minute();
}

bool TimerData::isReoccurring()
{
    return (Reoccurrence != "one_shot"
            && !Reoccurrence.startsWith("time_span"));
}
