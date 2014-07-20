#ifndef MYTHTEATIME_UI_H
#define MYTHTEATIME_UI_H

#include <timerdata.h>

// MythTV headers
#include <mythcontext.h>
#include <mythscreentype.h>
#include <mythuibutton.h>
#include <mythuispinbox.h>
#include <mythuitext.h>


#ifndef LOG_Tea
#define LOG_Tea(level, message) LOG(VB_GENERAL, level, QString("TT: %1") \
                                    .arg(message))
#endif

/**
 * @brief Screen to display all timers defined in the database
 *
 **/
class TeaTime : public MythScreenType
{
    Q_OBJECT

    public:
        TeaTime(MythScreenStack *parent);
        bool create(void);

    public slots:
        void newClicked(void);
        void itemClicked(MythUIButtonListItem *);
        void onEditCompleted(bool started);

    private slots:
        void refreshCountdown(void);

    private:
        void openEditScreen(TimerData *td);
        void fillTimerList(void);
        void updateBtnText(MythUIButtonListItem* item, TimerData* timerData);

        MythUIButton     *m_CancelButton;
        MythUIButton     *m_NewButton;
        MythUIText       *m_InfoText;
        MythUIText       *m_TitleText;
        MythUIButtonList *m_ButtonList;
        QTimer           *m_Timer;
};

#endif /* MYTHTEATIME_UI_H*/
