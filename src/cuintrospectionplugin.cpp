#include "cuintrospectionplugin.h"
#include <cuthreadservice.h>
#include <cutimerservice.h>
#include <cuactivitymanager.h>
#include <qustring.h>
#include <qustringlist.h>
#include <cumbia.h>
#include <cuserviceprovider.h>
#include <cuthread.h>
#include <cutimer.h>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QtDebug>
#include <QGridLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include <QtDebug>

class CuIntrospectionPluginPrivate {
public:
    int thread_count;
    QMap<QString, ThreadInfo> thmap;
    QMap<QString, TimerInfo> timermap;
    Cumbia *cumbia;
    QStringList errors;
    QDialog *dialog;
    CuIntrospectionEngineExtensionI *engine_extension;
    QStandardItemModel *model;
};

CuIntrospectionPlugin::CuIntrospectionPlugin(QObject *parent) : QObject(parent) {
    d = new CuIntrospectionPluginPrivate;
    d->cumbia = nullptr;
    d->thread_count = 0;
    d->dialog = nullptr;
    d->engine_extension = nullptr;
    d->model = nullptr;
}

CuIntrospectionPlugin::~CuIntrospectionPlugin() {
    delete d;
}

QDialog *CuIntrospectionPlugin::getDialog(QWidget *parent)
{
    if(!d->dialog) {
        int row = 0;
        d->dialog = new QDialog(parent);
        d->dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        QGridLayout *glo = new QGridLayout(d->dialog);
        QLabel *headingLabel = new QLabel("-", d->dialog);
        headingLabel->setObjectName("headingLabel");
        glo->addWidget(headingLabel, row, 0, 1, 4);
        row++;
        QTreeView *w = new QTreeView(d->dialog);
        w->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        glo->addWidget(w, row, 0, 5, 4);
        row += 5;
        QLabel *thCntLabel = new QLabel("Thread count: 0", d->dialog);
        thCntLabel->setObjectName("thCntLabel");
        glo->addWidget(thCntLabel, row, 1, 1, 2);
        QPushButton *pb = new QPushButton("Update", d->dialog);
        connect(pb, SIGNAL(clicked()), this, SLOT(updateRequest()));
        connect(d->dialog, SIGNAL(destroyed(QObject*)), this, SLOT(onDialogDestroyed(QObject *)));
        glo->addWidget(pb, row, 0, 1, 1);
        QPushButton *pbClose = new QPushButton("Close", d->dialog);
        connect(pbClose, SIGNAL(clicked()), d->dialog, SLOT(close()));
        glo->addWidget(pbClose, row, 3, 1, 1);
    }
    updateRequest();

    return d->dialog;
}

void CuIntrospectionPlugin::showDialog() {
    getDialog(nullptr);
    if(d->dialog) d->dialog->exec();
}

void CuIntrospectionPlugin::updateRequest() {
    if(d->dialog) {
        update();
        d->dialog->findChild<QTreeView *>()->setModel(toItemModel());
        d->dialog->findChild<QLabel *>("thCntLabel")->setText(QString("Thread count: %1").arg(getThreadCount()));
        QLabel *hl = d->dialog->findChild<QLabel *>("headingLabel");
        d->engine_extension ? hl->setText(d->engine_extension->dialogHeading())
                            : hl->setText(QString("cumbia-qtcontrols version %1").arg(CUMBIA_QTCONTROLS_VERSION_STR));
    }
}

void CuIntrospectionPlugin::onDialogDestroyed(QObject *) {
    d->dialog = nullptr;
}

/*!
 * \brief Initialize the object with a pointer to Cumbia and a list of search keys that will be used
 *        to determine thread names.
 *
 * \param cumbia
 * \param name_search_keys
 */
void CuIntrospectionPlugin::init(Cumbia *cumbia)
{
    d->cumbia = cumbia;
}

int CuIntrospectionPlugin::getThreadCount() const {
    return d->thread_count;
}

void CuIntrospectionPlugin::update() {
    d->thmap.clear();
    if(!d->cumbia) {
        perr("CuIntrospectionPlugin.update: you must call init() with a valid pointer to Cumbia");
    } else {
        int thcnt = 0;
        d->errors.clear();
        d->thread_count = 0;
        CuThreadService *th_service = static_cast<CuThreadService *>(d->cumbia->getServiceProvider()->get(CuServices::Thread));
        CuActivityManager* aman = static_cast<CuActivityManager *>(d->cumbia->getServiceProvider()->get(CuServices::ActivityManager));
        CuTimerService *timer_service = static_cast<CuTimerService *>(d->cumbia->getServiceProvider()->get(CuServices::Timer));
        std::vector<CuThreadInterface *> thll = th_service->getThreads();
        for(CuThreadInterface *l : thll) {
            ThreadInfo thi;
            thcnt++;
            if(l->type() == 0)  { // CuThread
                d->thread_count++;
                CuThread *cut = static_cast<CuThread *>(l);
                thi.token = cut->getToken();
                thi.activities = aman->activitiesForThread(cut);
                QString thnam = QString::fromStdString(thi.token);
                if(!thnam.isEmpty()) {
                    if(d->thmap.contains(thnam))
                        d->errors.append("CuIntrospectionPlugin: duplicate thread name " + thnam + "!");
                    else {
                        d->thmap[thnam] = thi;
                    }
                }
                else
                    d->errors.append(QString("CuIntrospectionPlugin: could not guess thread name from \"%1\"").arg(thi.token.c_str()));
            }
        }
        std::list<CuTimer *> timers = timer_service->getTimers();
        int tcnt = 0;
        for(std::list<CuTimer *>::const_iterator it = timers.begin(); it != timers.end(); ++it) {
            d->thread_count++;
            tcnt++;
            uintptr_t iptr = reinterpret_cast<uintptr_t>(*it);
            const std::list<const CuTimerListener *> &tlist = timer_service->getListeners(*it);
            TimerInfo ti;
            ti.name = QString("CuTimer_%1 [0x%2]").arg(tcnt).arg(iptr, 0, 10);
            ti.timeout = (*it)->timeout();
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            ti.timer_listeners = QList<const CuTimerListener*>(tlist.begin(), tlist.end());
#else
            ti.timer_listeners = QList<const CuTimerListener*>::fromStdList(tlist);
#endif
            d->timermap[ti.name] = ti;
        }
    }
}

QMap<QString, ThreadInfo> CuIntrospectionPlugin::getThreadInfo() {
    return d->thmap;
}

const ThreadInfo CuIntrospectionPlugin::getThreadInfo(const QString &name) {
    return d->thmap[name];
}

QStringList CuIntrospectionPlugin::errors() const {
    return d->errors;
}

QStandardItemModel *CuIntrospectionPlugin::toItemModel() const {
    int columnCnt;
    d->engine_extension == nullptr ? columnCnt = 2 : columnCnt = d->engine_extension->modelColumnCount();
    if(d->model)
        delete d->model;
    d->model = new QStandardItemModel(0, columnCnt);
    d->engine_extension == nullptr ? d->model->setHorizontalHeaderLabels(QStringList() << "Thread|Device|Key" << "Value")
                                   : d->model->setHorizontalHeaderLabels(d->engine_extension->modelHeaderLabels());
    QStandardItem *parentItem = d->model->invisibleRootItem();
    for(int i = 0; i < d->thmap.keys().size(); i++) {
        const QString thnam = d->thmap.keys()[i];
        const ThreadInfo &ti = d->thmap[thnam];
        QStandardItem *th_item = new QStandardItem(thnam);
        parentItem->appendRow(QList<QStandardItem *>() << th_item);
        for(size_t j = 0; j < ti.activities.size(); j++) {
            const CuActivity *a = ti.activities[j];
            QStandardItem *dev_it = nullptr;
            CuData atok = a->getToken();
            QuStringList keys = atok.keys();
            // order keys alphabetically
            std::sort(keys.begin(), keys.end());
            int dev_idx = keys.indexOf("device");
            if(dev_idx > -1) {
                dev_it = new QStandardItem(QuString(atok["device"]));
                th_item->appendRow(dev_it);
            }
            for(int k = 0; k < keys.size() && dev_it; k++) {
                if(k != dev_idx) {
                    QStandardItem *ait = new QStandardItem(keys[k]);
                    QStandardItem *aitv = new QStandardItem(QuString(atok, qstoc(keys[k])));
                    dev_it->appendRow(QList<QStandardItem *>() << ait << aitv);

                    if(d->engine_extension && ait->text() == "activity") {
                        QList<QList <QStandardItem *> > rows =  d->engine_extension->activityChildRows(a);
                        foreach(QList<QStandardItem *> activity_items, rows)
                            ait->appendRow(activity_items);
                    }
                }
            }
            if(!dev_it) {
                d->errors.append(QString("ThreadInfoHelper::toItemModel: token \"%1\" does not contain the \"device\" key")
                                 .arg(atok.toString().c_str()));
            }
        }
    }

    foreach(QString timernam, d->timermap.keys()) {
        QStandardItem *timer_it = new QStandardItem(timernam);
        const TimerInfo &ti = d->timermap[timernam];
        parentItem->appendRow(QList<QStandardItem *>() << timer_it << new QStandardItem(QString("%1 [ms]").arg(ti.timeout)) );
        foreach(const CuTimerListener *tlis, ti.timer_listeners) {
            const CuThread *th = dynamic_cast<const CuThread *>(tlis);
            if(th != nullptr) {
                const std::string& tok = th->getToken();
                const QString &thnam = QString::fromStdString(tok);
                timer_it->appendRow(new QStandardItem (thnam));
            }
        }


    }
    if(d->errors.size()) {
        QStandardItem *error_item = new QStandardItem("errors");
        parentItem->appendRow(error_item);
        foreach(QString err, d->errors)
            parentItem->appendRow(new QStandardItem(err));
    }
    return d->model;
}

/*!
 * \brief CuIntrospectionPlugin::installEngineExtension
 *
 * \param eei an implementation of the CuIntrospectionEngineExtensionI interface
 *
 * \note
 * CuIntrospectionPlugin takes ownership of the object passed as parameter, destroying it
 * when the object is deleted. Subsequent calls to installEngineExtension will dispose the
 * previously installed engine extension
 */
void CuIntrospectionPlugin::installEngineExtension(CuIntrospectionEngineExtensionI *eei) {
    if(d->engine_extension)
        delete d->engine_extension;
    d->engine_extension = eei;
}
