// C++ headers
#include <unistd.h>

// QT headers
#include <QApplication>
#include <QTimer>
#include <QString>

// MythTV headers
#include <mythcontext.h>
#include <mythplugin.h>
#include <mythpluginapi.h>
#include <mythversion.h>
#include <mythmainwindow.h>
#include <dbutil.h>

// teatime headers
#include "teatimeui.h"
#include "data.h"

static void openTimerScreen(void);
TeaTimeData* gTeaData = NULL;

static void setupKeys(void)
{
    REG_JUMPEX(QT_TRANSLATE_NOOP("MythControls", "Teatimer"),
            "Opens a dialog to setup a timer.", "" , openTimerScreen, false);

    LOG_Tea(LOG_INFO, "Registered JumpPoint \"Teatimer\"");
}

static bool CreateTable(MSqlQuery query)
{
    LOG_Tea(LOG_INFO, "Creating initial database talbes");
    bool success = query.exec(
            " CREATE TABLE IF NOT EXISTS `teatime`  ("
            "   `id` MEDIUMINT NOT NULL AUTO_INCREMENT,"
            "    PRIMARY KEY(`id`),"
            "   `message_text` text,  "
            "   `exec_date_time` timestamp NULL DEFAULT NULL "
            "        COMMENT 'time the timer actions will be executed',"
            "   `date_time` timestamp NULL DEFAULT NULL COMMENT 'Either this "
                            "or time_span have to contain valid data',"
            "   `time_span` time DEFAULT NULL COMMENT 'Either this or "
                            "date_time have to contain valid data',"
            "   `pause_playback` enum('no','yes') DEFAULT NULL,"
            "   `hostname` varchar(64) NOT NULL"
            " ) ENGINE=MyISAM DEFAULT CHARSET=utf8"
            "    COMMENT 'stores timer data for the teatime plugin';"
            );

    if (!success)
    {
        LOG_Tea(LOG_WARNING, QString("Could not create initial teatime table: %1")
                                    .arg(query.lastError().text()));
        return false;
    }

    success = query.exec(
            " CREATE TABLE IF NOT EXISTS `teatime_rundata`  ("
            " 	`timer_id` MEDIUMINT NOT NULL,   "
            " 	`run_order` TINYINT NOT NULL,"
            " 	`type` varchar(32) NOT NULL,"
            " 	`data` text NOT NULL"
            " ) ENGINE=MyISAM DEFAULT CHARSET=utf8 "
            "   COMMENT 'stores what shall be executed and in which order "
                        "when the timer is expired';"
            );
    if (!success)
    {
        LOG_Tea(LOG_WARNING, QString("Could not create initial teatime_rundata: %1")
                                    .arg(query.lastError().text()));
        return false;
    }

    gCoreContext->SaveSettingOnHost("TeatimeDBSchemaVer", "1", NULL);
    QString q = QString("INSERT INTO `mythconverg`.`teatime` "
            "   (`message_text`, `time_span`, `pause_playback`, `hostname`) "
            "   VALUES "
            "   ( 'Tea is ready!', '00:05:00', 'yes', '%1')")
                    .arg(gCoreContext->GetHostName());

    if (!query.exec(q))
    {
        LOG_Tea(LOG_WARNING, QString("Could not create sample data: %1")
                                    .arg(query.lastError().text()));
        return false;
    }

    LOG_Tea(LOG_INFO, "Creating initial database tables completed successfully.");
    return true;
}
static bool UpdateTableV1(MSqlQuery query)
{
    LOG_Tea(LOG_INFO, "upgading database tables to version 2.");
    // remove paus playback setting, we try allways to do it now
    bool success = query.exec("ALTER TABLE `mythconverg`.`teatime` DROP `pause_playback`");
    if (!success)
    {
        LOG_Tea(LOG_WARNING, QString("upgade to table verison 2 failed: %1")
                             .arg(query.lastError().text()));
        return false;
    }

    gCoreContext->SaveSettingOnHost("TeatimeDBSchemaVer", "2", NULL);
    LOG_Tea(LOG_INFO, "upgading database tables to version 2 successfully completed.");
    return true;
}

static bool UpdateTableV2(MSqlQuery query)
{
    LOG_Tea(LOG_INFO, "upgading database tables to version 3.");
    // remove pause playback setting, we try allways to do it now
    bool success = query.exec("ALTER TABLE `mythconverg`.`teatime` ADD `reoccurrence` VARCHAR(16) DEFAULT 'one_shot' ");
    if (!success)
    {
        LOG_Tea(LOG_WARNING, QString("upgade to table verison 3 failed: %1")
                             .arg(query.lastError().text()));
        return false;
    }

    gCoreContext->SaveSettingOnHost("TeatimeDBSchemaVer", "3", NULL);
    LOG_Tea(LOG_INFO, "upgading database tables to version 3 successfully completed.");
    return true;
}

static bool UpdateTableV3(MSqlQuery query)
{
    LOG_Tea(LOG_INFO, "upgading database tables to version 4.");
    bool success = query.exec("ALTER TABLE `mythconverg`.`teatime` ADD `show_message` BOOLEAN DEFAULT '1' ");
    if (!success)
    {
        LOG_Tea(LOG_WARNING, QString("upgade to table verison 4 failed: %1")
                             .arg(query.lastError().text()));
        return false;
    }

    gCoreContext->SaveSettingOnHost("TeatimeDBSchemaVer", "4", NULL);
    LOG_Tea(LOG_INFO, "upgading database tables to version 4 successfully completed.");
    return true;
};

static bool updateDb()
{
    LOG_Tea(LOG_INFO, "Checking DB state.");
    MSqlQuery query(MSqlQuery::InitCon());
    if (!DBUtil::TryLockSchema(query, 60))
    {
        LOG_Tea(LOG_WARNING, "Could not get db schema lock.");
        return false;
    }

    int dbVer =  gCoreContext->GetSetting("TeatimeDBSchemaVer", "0").toInt();

    int currentDbVersion = 4;

    if (dbVer > currentDbVersion )
    {
        LOG_Tea(LOG_WARNING, QString("Need schema version %1, but found %2.")
                .arg(currentDbVersion).arg(dbVer));
        QString message = QObject::tr("Failed to start Mythteatime (database schema mismatch).\n\n") +
                          QObject::tr("Please upgrade the Mythteatime plugin.");
        MythEvent* me = new MythEvent(MythEvent::MythUserMessage, message);
        QCoreApplication::instance()->postEvent(GetMythMainWindow(), me);
        return false;
    }

    bool success = true;
    switch(dbVer)
    {
        case 0:
            if (!CreateTable(query))
            {
                success = false;
                break;
            }
            /* FALLTHRU */
        case 1:
            if (!UpdateTableV1(query))
            {
                success = false;
                break;
            }
            /* FALLTHRU */
        case 2:
            if (!UpdateTableV2(query))
            {
                success = false;
                break;
            }
            /* FALLTHRU */
        case 3:
            if (!UpdateTableV3(query))
            {
                success = false;
                break;
            }
            /* FALLTHRU */
        default:
            break;
    }

    DBUtil::UnlockSchema(query);

    return success;
}



static void openTimerScreen(void)
{
    MythScreenStack *st = GetMythMainWindow()->GetMainStack();
    if (!st)
    {
        LOG_Tea(LOG_WARNING, "Could not get main stack.");
        return;
    }

    QString result = st->GetLocation(true);
    if (result.contains("Playback"))
    {
            LOG_Tea(LOG_INFO, "Can not create Teatime UI while in playback.");
            return;
    }

    if (result.contains("teatime"))
        return;

    TeaTime* teatime = new TeaTime(st);
    if (!teatime->create())
    {
        LOG_Tea(LOG_WARNING, "Could not create Teatime UI.");
        delete teatime;
        teatime = NULL;
        return;
    }
    st->AddScreen(teatime);
}

int mythplugin_init(const char *libversion)
{
    if (!gCoreContext->TestPluginVersion("mythteatime",
        libversion, MYTH_BINARY_VERSION))
        return -1;

    if (!updateDb())
        return -2;

    setupKeys();

    gTeaData = new TeaTimeData();
    if (!gTeaData->initialize())
    {
        LOG_Tea(LOG_WARNING, "Failed to init TeaTimeData.");
        return -3;
    }

    LOG_Tea(LOG_INFO, "Teatime plugin started.");
    return 0;
}

int mythplugin_run(void)
{
    openTimerScreen();
    return 0;
}

int mythplugin_config(void)
{
    return 0;
}

void mythplugin_destroy(void)
{
    if (gTeaData)
    {
        gTeaData->shutdown();
        delete gTeaData;
        gTeaData = NULL;
    }
}
