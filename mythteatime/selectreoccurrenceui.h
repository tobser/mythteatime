#ifndef SELECT_REOCCURRENCE_UI_H 
#define SELECT_REOCCURRENCE_UI_H 

#include <teatimeui.h>

class SelectReoccurrence : public MythScreenType 
{
    Q_OBJECT

    public:
        SelectReoccurrence(MythScreenStack *parent, QString reoccurrence);
        bool create(void);

    signals:
        void selectionCompleted(const QString selection);

    private slots:
            void onItemClicked(MythUIButtonListItem *item);
            void onOkClicked(void);

    private:
        void addButton(const QString text, const QString id);
        void setAllItemsCheckState(const MythUIButtonListItem::CheckState cs);
        bool allDaysChecked();

        MythUIButtonList *m_ReoccList;
        MythUIButton     *m_OkButton;
        QString          m_Selection;
};
#endif
