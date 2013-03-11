#ifndef MYTH_TIMER_DATA_H
#define MYTH_TIMER_DATA_H

#include <QString>
#include <QVariant>
#include <QTime>
#include <mythuitype.h>
#include <mythmainwindow.h>
#include <mythprogressdialog.h>

class TeaAction{
    public:
        QString Action_Type;
        QString Action_Data;
};
Q_DECLARE_METATYPE(TeaAction)

class TimerData {
    public:
        TimerData(int id);
        TimerData(TimerData * td);
        TimerData(): m_st(NULL), m_pd(NULL) { Id = -2; };

        bool init(void);
        bool isActive(void);
        void execAsync(void);
        QString toString(void);
        void toMap(InfoMap& map);
        void removeFromDb(void);
        bool saveToDb(void);
        void calcAndSaveExecTime(void);

        int                     Id;
        bool                    FixedTime;
        bool                    Pause_Playback;
        QString                 Message_Text;
        QDateTime               Exec_Date_Time;
        QDateTime               Date_Time;
        QTime                   Time_Span;
        QList<TeaAction>        Exec_Actions;
	    QMap <QString, QString> jumpDest;

    private:
        void exec(void);
        void jumpToAndWaitArrival(const QString & target);
        void runSysEvent(const QString & sysEventKey);
        void runCommand(const QString cmd);
	    void initJumpDest();

        MythScreenStack         *m_st ;
        MythUIProgressDialog    *m_pd;
};

 Q_DECLARE_METATYPE(TimerData)

#endif
