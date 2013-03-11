#include "data.h"
#include "timerdata.h"
#include "teatimeui.h"

// myth
#include <mythlogging.h>
#include <dbutil.h>


TeaTimeData::TeaTimeData()
    : m_Timer(NULL)
{

}

/**
 * @brief initialize with data from DB
 *
 * reads all the timers strored in the database and starts the timer
 * if its execution time is in the future
 *
 **/
bool TeaTimeData::initialize(void)
{
    LOG_Tea(LOG_INFO, "Loading timers from DB");
    MSqlQuery query(MSqlQuery::InitCon());

    query.prepare("SELECT id from `teatime` WHERE `hostname` = :HOST");
    query.bindValue(":HOST", gCoreContext->GetHostName());
    if (!query.exec())
    {
        LOG_Tea(LOG_WARNING, "Could not read timers from DB.");
        return false;
    }

    while(query.next())
    {
        int tId = query.value(0).toInt();
        TimerData* d = new TimerData(tId);
        d->init();
        m_Timers.insert(d->Id, d);
        if (d->isActive())
        {
            LOG_Tea(LOG_INFO, QString("Active: %1").arg(d->toString()));
            m_ActiveTimers << d;
        }
    }

    if (!m_ActiveTimers.isEmpty())
        startTimer();

    return true;
}

/**
 * @brief removes the old timer data and reloads everything from DB
 *
 * has to be called everytime the teatimetables are changed.
 *
 **/
void TeaTimeData::reInit(void)
{
    shutdown(); 
    initialize();
}

void TeaTimeData::shutdown(void)
{
    // todo: check wether a timer is currently executing 
    // abort execution before the timer is deleted..
    stopTimer();
    m_ActiveTimers.clear();
    for (int i = 0; i < m_Timers.count(); i++)
    {
        delete m_Timers[i];
    }
    m_Timers.clear();
}

/**
 * @brief starts a timer to check once a seconde wether anything has do be done
 *
 **/
void TeaTimeData::startTimer(void)
{
    stopTimer();

    m_Timer = new QTimer(this);
    if (!connect(m_Timer, SIGNAL(timeout()), this, SLOT(checkTimers())))
    {
        LOG_Tea(LOG_WARNING, "timout signal not connected.");
    }
    else
    {    
        m_Timer->start(1000);
        LOG_Tea(LOG_INFO, "Timer started.");
    }
}

/**
 * @brief Checks wether a timer needs to be executed
 *
 **/
void TeaTimeData::checkTimers()
{

    QMutableListIterator<TimerData*> it(m_ActiveTimers);
    while(it.hasNext())
    {
        TimerData *val = it.next();
        if (!val->isActive())
        {
            LOG_Tea(LOG_INFO, QString("Executing timer action of: ")
                    .append(val->toString()));
            val->execAsync();
            it.remove();
        }
    }

    if (m_ActiveTimers.isEmpty())
    {
        stopTimer();
    }
}

void TeaTimeData::stopTimer()
{
    if (m_Timer != NULL)
    {
        if (m_Timer->isActive())
        {
            m_Timer->stop();
        }
        delete m_Timer;        
        m_Timer = NULL;
        LOG_Tea(LOG_INFO, "Timer stopped.");
    }
}
