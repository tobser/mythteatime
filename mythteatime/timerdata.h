#ifndef MYTH_TIMER_DATA_H
#define MYTH_TIMER_DATA_H
#include <stdint.h>

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
            TimerData();
            TimerData(int id);
            TimerData(TimerData * td);

            bool init(void);
            bool isActive(void);
            void execAsync(void);
            QString toString(void);
            void toMap(InfoMap& map);
            void removeFromDb(void);
            bool saveToDb(bool startTimespanTimer = false);
            void calcAndSaveExecTime(void);
            void setTimeSpanFromSecs(int seconds);
            int getTimespanMinutes(void);
            bool isReoccurring();
            void setNotificationId(uint32_t id);

            int                     Id;
            bool                    ShowMessage; 
            QString                 Message_Text;
            QDateTime               Exec_Date_Time;
            QDateTime               Date_Time;
            QTime                   Time_Span;
            QList<TeaAction>        Exec_Actions;
            QMap <QString, QString> jumpDest;
            QString                 Reoccurrence;
            uint32_t                m_NotificationId;

        private:
            void exec(void);
            void jumpToAndWaitArrival(const QString & target);
            void runSysEvent(const QString & sysEventKey);
            void runCommand(const QString cmd);
            void initJumpDest();
            void initDefaults();
            void setNextExecutionTime(bool startTimespanTimer = false);
            void saveExecTimeToDb(QDateTime nextExecTime);
            void secsTo(int *h, int *m, int *s, int inSecs)
            {
                *h = inSecs / (60 * 60);
                *m = (inSecs % (60 * 60)) / 60;
                *s = inSecs % 60;
            };
            void postNotification(QString progressText, int  step, int  totalSteps);
    };

Q_DECLARE_METATYPE(TimerData)

#endif
