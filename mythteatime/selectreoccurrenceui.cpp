#include <selectreoccurrenceui.h>
#include <mythuibuttonlist.h>

SelectReoccurrence::SelectReoccurrence (MythScreenStack *parent, QString selection):
    MythScreenType(parent, "select_reoccurrence"),
    m_ReoccList(NULL),
    m_OkButton(NULL)
{
    if (selection.isEmpty())
        selection = "one_shot";

    m_Selection=selection;
}

bool SelectReoccurrence::Create(void)
{
    // Load the theme for this screen
    if (!LoadWindowFromXML("teatime-ui.xml", "select_reoccurrence", this))
    {
        LOG_Tea(LOG_WARNING, "window \"select_reoccurrence\" in teatime-ui.xml is missing.");
        return  false;
    }

    // assign required elements
    bool err = false;
    UIUtilE::Assign(this, m_OkButton, "ok", &err);
    UIUtilE::Assign(this, m_ReoccList, "list", &err);

    if (err)
    {
        LOG_Tea(LOG_WARNING, "Theme is missing required elements.");
        return  false;
    }

    addButton(tr("One shot"),"one_shot");
    addButton(tr("Daily"),"daily");

    for (int i = 1; i <= 7 ; i++)
    {
        addButton(QDate::longDayName(i),QString::number(i));
    }

    connect(m_ReoccList, SIGNAL(itemClicked(MythUIButtonListItem *)),
            this, SLOT(onItemClicked(MythUIButtonListItem*)));

    connect(m_OkButton, SIGNAL(Clicked(void)),
            this, SLOT(onOkClicked(void)));

    BuildFocusList();

    return true;

}

void SelectReoccurrence::onItemClicked(MythUIButtonListItem *item)
{
    LOG_Tea(LOG_WARNING, item->GetText());
    MythUIButtonListItem::CheckState cs = item->state();
    QString id = item->GetData().toString();

    if (id == "one_shot")
    {
        if (cs ==MythUIButtonListItem::FullChecked)
            return;

        SetAllItemsCheckState( MythUIButtonListItem::NotChecked);
    }
    else if (id == "daily")
    {
        if (cs == MythUIButtonListItem::FullChecked)
            SetAllItemsCheckState( MythUIButtonListItem::NotChecked);
        else
            SetAllItemsCheckState( MythUIButtonListItem::FullChecked);

        m_ReoccList->GetItemByData("one_shot")->setChecked(MythUIButtonListItem::NotChecked);
    }
    if (cs == MythUIButtonListItem::FullChecked)
        item->setChecked(MythUIButtonListItem::NotChecked);
    else
        item->setChecked(MythUIButtonListItem::FullChecked);

    if (id != "one_shot" && id != "daily")
    {
        m_ReoccList->GetItemByData("one_shot")->setChecked(MythUIButtonListItem::NotChecked);
        if (AllDaysChecked())
            m_ReoccList->GetItemByData("daily")->setChecked(MythUIButtonListItem::FullChecked);
        else
            m_ReoccList->GetItemByData("daily")->setChecked(MythUIButtonListItem::NotChecked);
    }
}
void SelectReoccurrence::SetAllItemsCheckState(const MythUIButtonListItem::CheckState cs)
{
    int cnt = m_ReoccList->GetCount();
    for (int i = 0; i < cnt ; i++)
    {
        MythUIButtonListItem * item =  m_ReoccList->GetItemAt(i);
        item->setChecked(cs);
    }
}

bool SelectReoccurrence::AllDaysChecked()
{
    int cnt = m_ReoccList->GetCount();
    for (int i = 0; i < cnt ; i++)
    {
        MythUIButtonListItem * item =  m_ReoccList->GetItemAt(i);
        QString data = item->GetData().toString();
        if (data != "one_shot" && data != "daily")
            if (item->state() == MythUIButtonListItem::NotChecked)
                return false;
    }
    return true;
}

void SelectReoccurrence::addButton(const QString text, const QString id)
{
    MythUIButtonListItem::CheckState cs = MythUIButtonListItem::NotChecked;
    if (m_Selection.contains(id))
        cs = MythUIButtonListItem::FullChecked;

     MythUIButtonListItem *button = new MythUIButtonListItem(m_ReoccList, text,"",true, cs);
     button->SetText(text,"text");
     button->SetData(QVariant(id));
}
void SelectReoccurrence::onOkClicked(void)
{
    int cnt = m_ReoccList->GetCount();
    QString selection;
    for (int i = 0; i < cnt ; i++)
    {
        MythUIButtonListItem * item =  m_ReoccList->GetItemAt(i);
        if (item->state() == MythUIButtonListItem::FullChecked)
        {
            QString data = item->GetData().toString();
            if (data == "daily" || data == "one_shot")
            {
                selection = data;
                break;
            }

            if (!selection.isEmpty())
                selection += ",";

            selection += data;
        }
    }
    LOG_Tea(LOG_INFO, "Selected reoccurrence: " + selection);
    emit SelectionCompleted(selection);
    Close();
}

