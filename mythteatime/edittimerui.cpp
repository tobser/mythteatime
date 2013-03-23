#include <edittimerui.h>
#include <selectreoccurrenceui.h>
//myth
#include <mythmainwindow.h>

EditTimer::EditTimer(MythScreenStack *parent, TimerData * timer):
    MythScreenType(parent, "edit_teatimer"),
    m_OkButton(NULL),
    m_DeleteButton(NULL),
    m_CancelButton(NULL),
    m_AddActionButton(NULL),
    m_ReoccurrenceButton(NULL),
    m_TimeSpinbox(NULL),
    m_InfoText(NULL),
    m_TitleText(NULL),
    m_TimeEdit(NULL),
    m_MessageTextEdit(NULL),
    m_FixedTimeCb(NULL),
    m_Actions(NULL),
    m_Data(timer)
{
}

bool EditTimer::create(void)
{
    // Load the theme for this screen
    if (!LoadWindowFromXML("teatime-ui.xml", "edit_teatimer", this))
    {
        LOG_Tea(LOG_WARNING, "window teatime in teatime-ui.xml is missing.");
        return  false;
    }

    // assign optional elements
    UIUtilW::Assign(this, m_CancelButton, "cancel");
    if (m_CancelButton)
        connect(m_CancelButton, SIGNAL(Clicked()), SLOT(Close()));

    UIUtilW::Assign(this, m_TitleText, "title");
    if (m_TitleText)
        m_TitleText->SetText(tr("Edit Timer"));

    UIUtilW::Assign(this, m_InfoText, "infotext");
    if (m_InfoText)
        m_InfoText->SetText(tr("\"OK\" saves and starts the timer. "
                    "\"Delete\" removes the timer. "
                    "\"Cancel\" discards all changes and goes back"
                    " to the previos screen."));

    // assign required elements
    bool err = false;
    UIUtilE::Assign(this, m_OkButton, "ok", &err);
    UIUtilE::Assign(this, m_DeleteButton, "delete", &err);
    UIUtilE::Assign(this, m_AddActionButton, "add_action", &err);
    UIUtilE::Assign(this, m_MessageTextEdit, "message", &err);
    UIUtilE::Assign(this, m_TimeSpinbox, "time_span", &err);
    UIUtilE::Assign(this, m_FixedTimeCb, "fixed_time", &err);
    UIUtilE::Assign(this, m_TimeEdit, "time", &err);
    UIUtilE::Assign(this, m_Actions, "action_list", &err);
    UIUtilE::Assign(this, m_ReoccurrenceButton, "reoccurrence", &err);
    if (err)
    {
        LOG_Tea(LOG_WARNING, "Theme is missing required elements.");
        return  false;
    }

    m_TimeSpinbox->SetRange(0, 1439, 1, 5);
    enableTimespanUi();

    if (m_Data.Id >= 0)
    {
        // set values from db
        m_MessageTextEdit->SetText(m_Data.Message_Text);
        m_TimeEdit->SetText(m_Data.Date_Time.toString());
        m_TimeSpinbox->SetValue(m_Data.getTimespanMinutes());

        if (!m_Data.Reoccurrence.startsWith("time_span"))
        {
            m_FixedTimeCb->SetCheckState( MythUIStateType::Full);
            enableNoneTimespanUi();
        }
    }

    connect(m_Actions, SIGNAL(itemClicked(MythUIButtonListItem *)),
            this, SLOT(onActionItemClicked(MythUIButtonListItem *)));
    connect(m_OkButton, SIGNAL(Clicked()),
            this,  SLOT(onOkClicked()));
    connect(m_DeleteButton, SIGNAL(Clicked()),
            this, SLOT(onDeleteClicked()));
    connect(m_AddActionButton, SIGNAL(Clicked()),
            this, SLOT(onAddActionClicked()));
    connect(m_FixedTimeCb, SIGNAL(toggled(bool)),
            this, SLOT(fixedTimeCbToggled(bool)));
    connect(m_ReoccurrenceButton, SIGNAL(Clicked()),
            this, SLOT(onReoccurrenceClicked()));

    buildActionButtonList();

    BuildFocusList();

    if (m_Data.Id < 0)
        SetFocusWidget(m_MessageTextEdit);
    else
        SetFocusWidget(m_OkButton);

    LOG_Tea(LOG_INFO, QString("Editor for %1 created.")
            .arg(m_Data.toString()));

    return true;
}

void EditTimer::enableTimespanUi(void)
{
    m_TimeEdit->SetVisible(false);
    m_TimeEdit->SetEnabled(false);

    m_ReoccurrenceButton->SetEnabled(false);
    m_ReoccurrenceButton->Hide();

    m_TimeSpinbox->SetEnabled(true);
    m_TimeSpinbox->SetVisible(true);
    m_TimeSpinbox->SetValue(m_Data.getTimespanMinutes());
    m_TimeSpinbox->Show();

    SetRedraw();

}
void EditTimer::enableNoneTimespanUi(void)
{

    m_TimeSpinbox->SetEnabled(false);
    m_TimeSpinbox->Hide();

    m_TimeEdit->SetText(m_Data.Date_Time.toString());
    m_TimeEdit->SetEnabled(true);
    m_TimeEdit->SetVisible(true);

    m_ReoccurrenceButton->SetEnabled(true);
    m_ReoccurrenceButton->Show();

    // set the correct button text:
    if (m_Data.Reoccurrence.startsWith("time_span"))
        m_Data.Reoccurrence = "one_shot";

    onReoccurrenceSelctionComplete(m_Data.Reoccurrence);

    SetRedraw();
}

void EditTimer::buildActionButtonList(void)
{
    m_Actions->Reset();
    if (m_Data.Exec_Actions.count() == 0)
    {
        m_Actions->SetEnabled(false);
        return;
    }

    m_Actions->SetEnabled(true);

    for (int i=0; i < m_Data.Exec_Actions.count(); i++)
    {
        TeaAction t = m_Data.Exec_Actions[i];
        MythUIButtonListItem *item = new MythUIButtonListItem(m_Actions, "");
        item->SetData(i);
        InfoMap map;
        map["action_type"] = t.Action_Type;
        map["action"] = t.Action_Data;
        map["order_number"] = QString("%1").arg(i+1);
        item->SetTextFromMap(map);
    }
    m_Actions->SetRedraw();
}

void EditTimer::onDeleteClicked(void)
{
    if (m_Data.Id >= 0)
    {
        m_Data.removeFromDb();
    }
    emit editComplete();
    Close();
}

void EditTimer::onOkClicked(void)
{
    LOG_Tea(LOG_INFO, "Ok");

    QString err;
    if (!updateLocalDataFromUi(err))
    {
        LOG_Tea(LOG_WARNING, err);
        return;
    }

    m_Data.saveToDb(true);

    emit editComplete();
    Close();
}

void EditTimer::customEvent(QEvent *event)
{
    if (event->type() == DialogCompletionEvent::kEventType)
    {
        DialogCompletionEvent *dce = (DialogCompletionEvent*)(event);

        TeaAction ta;
        ta.Action_Data = dce->GetData().toString();
        if (ta.Action_Data == "0")
            return;

        if (dce->GetId() == "ADD_JUMPPOINT")
        {
            LOG_Tea(LOG_INFO, ta.Action_Data);
            ta.Action_Type = "JumpPoint";
            m_Data.Exec_Actions << ta;
        }
        if (dce->GetId() == "ADD_SYSEVENT")
        {
            LOG_Tea(LOG_INFO, ta.Action_Data);
            ta.Action_Type = "SysEvent";
            m_Data.Exec_Actions << ta;
        }
        if (dce->GetId() == "ADD_CUSTOM_CMD")
        {
            ta.Action_Type = "Command";
            ta.Action_Data = dce->GetResultText();
            LOG_Tea(LOG_INFO, ta.Action_Data);
            m_Data.Exec_Actions << ta;
        }
        buildActionButtonList();
    }
}

bool EditTimer::updateLocalDataFromUi(QString & /*&err*/)
{
    // err shall someday contain some usefull info to tell teh user what's
    // wrong with his input (e.g. broken date..)
    //
    m_Data.Message_Text = m_MessageTextEdit->GetText();

    if (m_FixedTimeCb->GetCheckState() == MythUIStateType::Off)
    {
        // set timespan:
        int min = m_TimeSpinbox->GetValue().toInt();
        m_Data.setTimeSpanFromSecs(min * 60);
        if (min == 0)
            m_Data.Reoccurrence = "time_span";
        else
            m_Data.Reoccurrence = "time_span_active";
    }
    else
    {
        // set date
        m_Data.Date_Time = QDateTime::fromString(m_TimeEdit->GetText());
    }

    return true;
}


void EditTimer::onAddActionClicked(void)
{
    MythDialogBox *dialog = createDialog(tr("Add Action"));
    if (!dialog)
        return;

    dialog->SetReturnEvent(this, "ADD_ACTION");
    dialog->AddButton(tr("Add Jumppoint"), SLOT(jumppointMenu()), true);
    dialog->AddButton(tr("Custom Command"), SLOT(newCustomCmd()));
    if (hasSysevents())
        dialog->AddButton(tr("Add Systemevent"), SLOT(syseventMenu()), true);
}

bool EditTimer::hasSysevents(void)
{
    MSqlQuery q(MSqlQuery::InitCon());
    q.prepare("SELECT count(*) FROM `settings` "
            " WHERE `hostname` = :HOST AND `value` LIKE 'EventCmdKey%'");
    q.bindValue(":HOST",  gCoreContext->GetHostName());

    if (!q.exec())
    {
        LOG_Tea(LOG_WARNING, "Could not read sysevent count from DB.");
        return false;
    }

    if (!q.next())
        return false;

    return (q.value(0).toInt() > 0);
}

void EditTimer::newCustomCmd()
{
    editCustomCmd("");
}
void EditTimer::editCustomCmd(QString cmd)
{
    MythScreenStack *st = GetMythMainWindow()->GetStack("popup stack");
    if (!st)
    {
        LOG_Tea(LOG_WARNING, QString("Could not get \"popup stack\" to "
                    "display cmd inputbox."));
        return;
    }
    MythTextInputDialog *d = new MythTextInputDialog(st,
            tr("Create Custom Command"),
            FilterNone,
            false,
            cmd);
    if (!d->Create())
    {
        LOG_Tea(LOG_WARNING, "Could not create cmd inputbox.");
        return;
    }

    d->SetReturnEvent(this, "ADD_CUSTOM_CMD");

    st->AddScreen(d);
}

void EditTimer::jumppointMenu(void)
{
    MythDialogBox *dialog = createDialog(tr("Select Jumppoint"));
    if (!dialog)
        return;

    dialog->SetReturnEvent(this, "ADD_JUMPPOINT");

    QMap<QString,QString>::iterator i = m_Data.jumpDest.begin();
    while(i != m_Data.jumpDest.end())
    {
        QString jp = i.key();
        dialog->AddButton(jp,jp,false,false);
        i++;
    }
}
void EditTimer::syseventMenu(void)
{
    MSqlQuery q(MSqlQuery::InitCon());
    q.prepare("SELECT value,data FROM `settings` "
            " WHERE `hostname` = :HOST AND `value` LIKE 'EventCmdKey%'");
    q.bindValue(":HOST",  gCoreContext->GetHostName());

    if (!q.exec())
    {
        LOG_Tea(LOG_WARNING, "Could not read sysevents from DB.");
        return;
    }

    MythDialogBox *dialog = createDialog(tr("Select Sysevent"));
    if (!dialog)
        return;

    dialog->SetReturnEvent(this, "ADD_SYSEVENT");

    while (q.next())
    {
        QString ev = q.value(0).toString();
        QString cmd = q.value(1).toString();
        QString val = QString("%1 (%2)").arg(ev).arg(cmd);
        dialog->AddButton(val,ev,false,false);
    }
}

void EditTimer::onActionItemClicked(MythUIButtonListItem * /* item*/)
{
    if (m_Data.Exec_Actions.count() == 0)
        return;

    MythDialogBox *dialog = createDialog(tr("Modify Action"));
    if (!dialog)
        return;

    dialog->SetReturnEvent(this, "MODIFY_ACTION");
    if (m_Data.Exec_Actions.count() > 1)
    {
        dialog->AddButton(tr("Move Up"), SLOT(moveActionUp()));
        dialog->AddButton(tr("Move Down"), SLOT(moveActionDown()));
    }
    dialog->AddButton(tr("Remove"), SLOT(onRemoveAction(void)) );
}

MythDialogBox* EditTimer::createDialog(const QString title)
{
    MythScreenStack *st = GetMythMainWindow()->GetStack("popup stack");
    if (!st)
    {
        LOG_Tea(LOG_WARNING, QString("Could not get \"popup stack\" to "
                    "display \"%1\" dialogbox.").arg(title));
        return NULL;
    }
    MythDialogBox *dialog = new MythDialogBox(title, st, "menu");

    if (!dialog->Create())
    {
        delete dialog;
        LOG_Tea(LOG_WARNING, QString("Could not create dialogbox \"%1\".")
                .arg(title));
        return NULL;
    }

    st->AddScreen(dialog);
    return dialog;
}

void EditTimer::onRemoveAction(void)
{
    MythUIButtonListItem *item = m_Actions->GetItemCurrent();
    if (!item)
        return;

    int index = item->GetData().toInt();
    m_Data.Exec_Actions.removeAt(index);
    buildActionButtonList();
}

void EditTimer::moveAction(bool up)
{
    MythUIButtonListItem *item = m_Actions->GetItemCurrent();
    if (!item)
        return;

    int index = item->GetData().toInt();
    int newIdx = (up ? (index -1) : (index + 1));

    int listSize = m_Data.Exec_Actions.count();
    TeaAction t = m_Data.Exec_Actions[index];
    m_Data.Exec_Actions.removeAt(index);
    if (newIdx < 0)
        m_Data.Exec_Actions << t; // was first, shall be last:
    else if (newIdx >= listSize)
        m_Data.Exec_Actions.insert(0,t); // was last, becomes first
    else
        m_Data.Exec_Actions.insert(newIdx,t);

    buildActionButtonList();
}

void EditTimer::fixedTimeCbToggled(bool checked)
{
    if (checked)
        enableNoneTimespanUi();
    else
        enableTimespanUi();
}

void EditTimer::onReoccurrenceClicked(void)
{
    MythScreenStack *st = GetMythMainWindow()->GetStack("popup stack");
    if (!st)
    {
        LOG_Tea(LOG_WARNING, QString("Could not get \"popup stack\"")); 
        return ;
    }

    SelectReoccurrence *sel = new SelectReoccurrence(st, m_Data.Reoccurrence);
    if (!sel->create())
    {
        LOG_Tea(LOG_WARNING, QString("Could not create select reoccurrence popup."));
        return ;
    }
    connect(sel, SIGNAL(selectionCompleted(QString)),
            this, SLOT(onReoccurrenceSelctionComplete(QString)));

    st->AddScreen(sel);
}
void EditTimer::onReoccurrenceSelctionComplete(const QString selection)
{
    LOG_Tea(LOG_INFO, "Received selection "+ selection);
    m_Data.Reoccurrence = selection;
    QString text;
    if (selection.contains("daily"))
        text = tr("Daily");
    else if (selection.contains("one_shot"))
        text = tr("One shot");
    else
        text= tr("Days: %1").arg(selection);

    m_ReoccurrenceButton->SetText(text);
}
