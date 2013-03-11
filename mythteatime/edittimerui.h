#ifndef EDIT_TIMER_UI_H
#define EDIT_TIMER_UI_H

#include <teatimeui.h>
#include <timerdata.h>

// MythTV headers
#include <mythcontext.h>
#include <mythscreentype.h>
#include <mythuibutton.h>
#include <mythuispinbox.h>
#include <mythuitext.h>
#include <mythuitextedit.h>
#include <mythuicheckbox.h>
#include <mythdialogbox.h>

/**
 * @brief Editor screen to alter a timer or create a new one
 *
 **/
class EditTimer : public MythScreenType 
{
    Q_OBJECT
    
    public:
        EditTimer(MythScreenStack *parent, TimerData *timer);
        bool Create(void);
        void customEvent(QEvent *event);
        
    protected slots:
        void onDeleteClicked(void);
        void onSaveClicked(void);
        void onSaveAndStartClicked(void);
        void onAddActionClicked(void);
        void onActionItemClicked(MythUIButtonListItem *);
        void onEditCompleted(bool close);
        void fixedTimeCbToggled(bool);
        void jumppointMenu(void);
        void syseventMenu(void);
        void onRemoveAction(void);
        void moveActionUp(void) { moveAction(true); };
        void moveActionDown(void) { moveAction(false); };
        void newCustomCmd(void);

    signals:
        void editComplete(bool close);


    private:
        bool updateLocalDataFromUi(QString &err);
        void buildActionButtonList(void);
        void moveAction(bool up);
        void editCustomCmd(QString); 
        MythDialogBox* createDialog(const QString title);

        MythUIButton     *m_SaveButton;
        MythUIButton     *m_SaveAndStartButton;
        MythUIButton     *m_DeleteButton;
        MythUIButton     *m_CancelButton;
        MythUIButton     *m_AddActionButton;
        MythUISpinBox    *m_TimeSpinbox;
        MythUIText       *m_InfoText;
        MythUIText       *m_TitleText;
        MythUITextEdit   *m_TimeEdit;
        MythUITextEdit   *m_MessageTextEdit;
        MythUICheckBox   *m_FixedTimeCb;
        MythUICheckBox   *m_PauseCb;
        MythUIButtonList *m_Actions;

        TimerData        m_Data;
};



#endif
