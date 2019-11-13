#ifndef CUINTROSPECTIONPLUGIN_H
#define CUINTROSPECTIONPLUGIN_H

#include <cumbiaintrospectionplugin_i.h>
#include <QObject>
#include <QMap>

class QDialog;
class CuIntrospectionPluginPrivate;
class QStandardItem;
class Cumbia;
class CuIntrospectionEngineExtensionPlugin_I;

class CuIntrospectionPlugin :  public QObject, public CumbiaIntrospectionPlugin_I
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "cumbia-qtcontrols-introspection-plugin.json")

public:
    explicit CuIntrospectionPlugin(QObject *parent = nullptr);

    ~CuIntrospectionPlugin();

    Q_INTERFACES(CumbiaIntrospectionPlugin_I)

public slots:
    void showDialog();

private slots:
    void updateRequest();
    void onDialogDestroyed(QObject *o);

private:

    // CumbiaIntrospectionPlugin_I interface
public:
    void init(Cumbia *cumbia, const QStringList& name_search_keys = QStringList() << "th_tok" << "device");
    int getThreadCount() const;
    void update();
    QString findName(const CuData &data_tok) const;
    QMap<QString, ThreadInfo> getThreadInfo();
    const ThreadInfo getThreadInfo(const QString &name);
    QStringList errors() const;
    QStandardItemModel *toItemModel() const;
    void installEngineExtension(CuIntrospectionEngineExtensionPlugin_I *eei);
    QDialog *getDialog(QWidget *parent);

private:
    CuIntrospectionPluginPrivate *d;
};

#endif // CUINTROSPECTIONPLUGIN_H
