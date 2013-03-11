#ifndef MYTH_DATA_H
#define MYTH_DATA_H

#include <mythmainwindow.h>

#include "teatimeui.h"
#include "timerdata.h"

#include <QThread>
#include <QTimer>
#include <QString>
#include <QEvent>
#include <QMap>


/**
 * @brief holds the list of timers
 *
 * keeps track of active timers and (re)loads them from the database
 *
 **/
class TeaTimeData :public QObject
{
    Q_OBJECT

    public:
        TeaTimeData();
        bool initialize(void);
        void shutdown(void);
        void reInit(void);
        QMap<int, TimerData *> m_Timers;
        QList<TimerData *> m_ActiveTimers;

    public slots:
        void checkTimers();

    private:
        void stopTimer(void);
        void startTimer(void);
        QTimer  * m_Timer;
};

extern MPUBLIC TeaTimeData* gTeaData;

#endif /* MYTH_DATA_H*/

