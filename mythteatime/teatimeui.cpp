// POSIX headers
#include <unistd.h>

// MythTV headers
#include <mythuibutton.h>
#include <mythuitext.h>
#include <mythmainwindow.h>
#include <mythcontext.h>
#include <mythdirs.h>
#include <mythdialogbox.h>
#include <mythuiutils.h>

// MythTeaTime headers
#include "teatimeui.h"
#include "edittimerui.h"
#include "data.h"

TeaTime::TeaTime(MythScreenStack *parent):
    MythScreenType(parent, "teatime"),
    m_CancelButton(NULL),
    m_NewButton(NULL),
    m_InfoText(NULL),
    m_TitleText(NULL),
    m_ButtonList(NULL)
{
}

bool TeaTime::Create(void)
{

    bool foundtheme = false;

    // Load the theme for this screen
    foundtheme = LoadWindowFromXML("teatime-ui.xml", "teatime", this);
    
    if (!foundtheme)
    {
        LOG_Tea(LOG_WARNING, "window teatime in teatime-ui.xml is missing."); 
        return  false;
    }

    UIUtilW::Assign(this, m_CancelButton, "cancel");
    if (m_CancelButton)
        connect(m_CancelButton, SIGNAL(Clicked()), SLOT(Close()));

    UIUtilW::Assign(this, m_InfoText, "infotext");
    if (m_InfoText)
        m_InfoText->SetText(tr("Select a timer to start or edit. To create"
                            " a new timer press the \"New\" button."));

    UIUtilW::Assign(this, m_TitleText, "title");
    if (m_TitleText)
        m_TitleText->SetText(tr("It's teatime!"));

    bool err = false;
    UIUtilE::Assign(this, m_ButtonList, "timer_list", &err);
    UIUtilE::Assign(this, m_NewButton, "new", &err);

    if (err)
    {
        LOG_Tea(LOG_WARNING, "Theme is missing required elements."); 
        return  false;
    }

    fillTimerList();
    
    connect(m_ButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)),
                     this, SLOT(itemClicked(MythUIButtonListItem *)));
    connect(m_NewButton,    SIGNAL(Clicked()), SLOT(newClicked()));

    BuildFocusList();

    return true;
}

void TeaTime::fillTimerList(void)
{
    m_ButtonList->Reset();
    QMapIterator<int,TimerData*> it(gTeaData->m_Timers);
    while(it.hasNext())
    {
        it.next();
        TimerData * d = it.value();
        MythUIButtonListItem *item = new MythUIButtonListItem(m_ButtonList, "");

        item->SetData(qVariantFromValue(*d));
        
        InfoMap map;
        d->toMap(map);

        if (map["time_span"].isEmpty())
            map["timer_type"] = "Fixed time";
        else
            map["timer_type"] = "Duration";
    
        item->SetTextFromMap(map);
    }
    m_ButtonList->SetRedraw();
}

void TeaTime::newClicked(void)
{
    openEditScreen(NULL);
}
void TeaTime::itemClicked(MythUIButtonListItem * item)
{
    TimerData val = qVariantValue<TimerData>(item->GetData());
    openEditScreen(&val);
}

void TeaTime::openEditScreen(TimerData *td = NULL)
{
    MythScreenStack *popupStack = GetMythMainWindow()->GetMainStack();
    if (!popupStack)
    {
        LOG_Tea(LOG_WARNING, "Could not get PopupStack.");
        return;
    }

    EditTimer *et = new EditTimer(popupStack, td);
    if (!et->Create())
    {
        LOG_Tea(LOG_WARNING, "Could not create edit timer screen.");
        delete et;
        return;
    }

    connect(et, SIGNAL(editComplete(bool)), this,
                 SLOT(onEditCompleted(bool)),Qt::DirectConnection);

    popupStack->AddScreen(et);
}

void TeaTime::onEditCompleted(bool close)
{
    LOG_Tea(LOG_INFO, "Edit complete rebuilding ui.");
    if (gTeaData)
    {
        gTeaData->reInit();
        fillTimerList();
    }
    if (close)
        Close();

    LOG_Tea(LOG_INFO, "rebuilding done.");
}
