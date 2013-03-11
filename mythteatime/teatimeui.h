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
        bool Create(void);
        
    public slots:
        void newClicked(void);
        void itemClicked(MythUIButtonListItem *);
        void onEditCompleted(bool close);

    private:
        void openEditScreen(TimerData *td);
        void fillTimerList(void);

        MythUIButton     *m_CancelButton;
        MythUIButton     *m_NewButton;
        MythUIText       *m_InfoText;
        MythUIText       *m_TitleText;
        MythUIButtonList *m_ButtonList;
};

#endif /* MYTHTEATIME_UI_H*/
