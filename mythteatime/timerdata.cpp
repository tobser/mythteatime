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
#include <QCoreApplication>

TimerData::TimerData(int id)
    :Id(id),
    m_st(NULL),
    m_pd(NULL)

{
    initJumpDest();
}

TimerData::TimerData(TimerData * td )
    :m_st(NULL),
    m_pd(NULL)
{
    if (!td)
    {
        Id = -3;
    }
    else
    {
        Id=td->Id;
        FixedTime =td->FixedTime;
        Message_Text = td->Message_Text;
        Exec_Date_Time = td->Exec_Date_Time;
        Date_Time = td->Date_Time;
        Time_Span = td->Time_Span;
        Exec_Actions = td->Exec_Actions;
        Pause_Playback = td->Pause_Playback;
    }
    initJumpDest();
}

QString TimerData::toString(void)
{
    return QString ("TimerData[id=%1,t=\"%2\",e=%3,ts=%4,dt=%5,a=\"%6\"]")
        .arg(Id)
        .arg(Message_Text)
        .arg(Exec_Date_Time.toString())
        .arg(Time_Span.toString())
        .arg(Date_Time.toString())
        .arg(Exec_Actions.count());
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
    map["time_span"] = Time_Span.toString();
    map["date_time"] = Date_Time.toString();
    QString actions;
    for (int i =0; i<Exec_Actions.count(); i++){
        if (i > 0)
            actions.append(", ");

        TeaAction a = Exec_Actions[i];
        actions += QString("%1.) ").arg(i+1);
        actions += a.Action_Type +" "+ a.Action_Data;
    }
    map["actions"] = actions;
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
    QDateTime now = QDateTime::currentDateTime();
    return (now <= Exec_Date_Time);
}

bool TimerData::init(void)
{
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT "
              " message_text,"      // 0
              " exec_date_time, "   // 1 
              " date_time, "        // 2
              " time_span, "        // 3
              " pause_playback "   // 4
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

    Message_Text= query.value(0).toString();

    Exec_Date_Time = query.value(1).toDateTime();
    Date_Time = query.value(2).toDateTime();
    Time_Span = QTime::fromString(query.value(3).toString(), "hh:mm:ss");

    FixedTime = Date_Time.isValid();

    Pause_Playback = (query.value(4).toString() == "yes");

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

void TimerData::exec(void)
{
    MythMainWindow *mainWin = GetMythMainWindow();
    if (!mainWin)
    {
        LOG_Tea(LOG_WARNING, "Could not get main window.");
        return;
    }

    QStringList sl ;
    if (Pause_Playback)
        sl << "pauseplayback";

    LOG_Tea(LOG_INFO, QString("Popup: %1").arg(Message_Text));
    MythEvent* me = new MythEvent(MythEvent::MythUserMessage, Message_Text, sl);
    QCoreApplication::instance()->postEvent(mainWin, me);
    sleep(2);

    int cnt = Exec_Actions.count();
    if (cnt == 0)
        return;

    LOG_Tea(LOG_INFO, "Executing actions.");

    m_pd->SetTotal(cnt);
    m_st->AddScreen(m_pd);

    for (int i=0; i < cnt; i++)
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

void TimerData::runCommand(const QString cmd)
{
     LOG_Tea(LOG_INFO, QString("running cmd '%1'").arg(cmd));
     MythSystem s (cmd, (kMSStdIn|kMSStdOut|kMSStdErr));
     s.Run();
     s.Wait();
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

    int cnt = Exec_Actions.count();
    QString msg = QObject::tr("Executing %1 timer acions").arg(cnt);
    m_pd = new MythUIProgressDialog(msg, m_st, "exec_actions");
    m_pd->Create();

    QFuture<void> f = QtConcurrent::run(this, &TimerData::exec);
}

bool TimerData::saveToDb(void)
{
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
                    ", `exec_date_time` = :EXECT " 
                    ", `date_time` = :DATETIME " 
                    ", `time_span` = :TIMESPAN " 
                    ", `pause_playback` = :PAUSE " 
                    "WHERE `id` =:ID ");

    if (Message_Text.isEmpty())
        query.bindValue(":TEXT", Id);
    else
        query.bindValue(":TEXT", Message_Text); 

    query.bindValue(":EXECT", QVariant());

    if (FixedTime)
    {
        query.bindValue(":TIMESPAN", QVariant());
        // todo check date format + popup + abort..
        query.bindValue(":DATETIME", Date_Time);
    }
    else
    {
        query.bindValue(":TIMESPAN", Time_Span);
        // todo check date format + popup + abort..
        query.bindValue(":DATETIME", QVariant());
    }

    query.bindValue(":PAUSE", (Pause_Playback? "yes" : "no"));
    query.bindValue(":ID", Id);

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

void TimerData::calcAndSaveExecTime(void)
{
    QDateTime exec_time;
    if (FixedTime)
    {
        exec_time = Date_Time;
    }
    else
    {
        exec_time = QDateTime::currentDateTime()
                    .addSecs(Time_Span.hour() * 60 * 60)
                    .addSecs(Time_Span.minute() * 60)
                    .addSecs(Time_Span.second());
    }

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare( "UPDATE `teatime` SET "
                    "`exec_date_time` =  :EXEC_TIME "
                    "WHERE `id` =:ID ");
    query.bindValue(":EXEC_TIME", exec_time);
    query.bindValue(":ID", Id);
    if(!query.exec())
    {
        QString msg = QString("Could not store execution time to DB. %1")
            .arg(query.lastError().text());
        LOG_Tea(LOG_WARNING, msg);
    }
}
