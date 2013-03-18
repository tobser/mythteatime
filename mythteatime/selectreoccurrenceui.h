#ifndef SELECT_REOCCURRENCE_UI_H 
#define SELECT_REOCCURRENCE_UI_H 

#include <teatimeui.h>


class SelectReoccurrence : public MythScreenType 
{
    Q_OBJECT

    public:
        SelectReoccurrence(MythScreenStack *parent);
        bool Create(void);

        private slots:
            void onItemClicked(MythUIButtonListItem *item);

    private:
        void addButton(const QString text, const QString id);
        void SetAllItemsCheckState(const MythUIButtonListItem::CheckState cs);
        bool AllDaysChecked();
        MythUIButtonList *m_ReoccList;
        MythUIButton     *m_OkButton;
};
#endif
