#ifndef TABREPAIRNEW_H
#define TABREPAIRNEW_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QToolButton>
#include <QStyle>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QDate>
#include <QDateEdit>
#include <QRandomGenerator>
#ifdef QT_DEBUG
#include <QTimer>
#endif
#include "mainwindow.h"
#include "tabcommon.h"

namespace Ui {
    class tabRepairNew;
}

class groupBoxEventFilter : public QObject
{
    Q_OBJECT
signals:

public:
    groupBoxEventFilter(QObject*);
private:
protected:
    bool eventFilter(QObject*, QEvent*) override;
};

class tabRepairNew : public tabCommon
{
    Q_OBJECT

signals:
    void createTabSelectPrevRepair(int);
    void createTabSelectExistingClient(int);
    void createTabClient(int);
    void generatePrintout(QMap<QString, QVariant>);

public:
    explicit tabRepairNew(MainWindow *parent = nullptr);
    static tabRepairNew* getInstance(MainWindow *parent = nullptr);
    ~tabRepairNew();
    virtual bool tabCloseRequest();
#ifdef QT_DEBUG
    void randomFill();
#endif

private:
    Ui::tabRepairNew *ui;
    static tabRepairNew* p_instance;
    MainWindow *parent;
    void setDefaultStyleSheets();
    void getDevices();
    QString genUserWebPass() const;
//    QStandardItemModel* comboboxDevicesModel;
    QSqlQueryModel* comboboxDevicesModel;
    QSqlQueryModel* comboboxDeviceMakersModel;
    QSqlQueryModel* comboboxDeviceModelsModel;
    QSqlQueryModel* comboboxProblemModel;
    QSqlQueryModel* comboBoxIncomingSetModel;
    QSqlQueryModel* comboBoxExteriorModel;
    QSqlQueryModel* clientModel;
    QSqlQueryModel* queryDevice;
    QSqlQueryModel* clientPhonesModel;
    QSqlQueryModel* clientAdTypesList;
    QStandardItemModel* clientPhoneTypesList;
    QList<QStandardItem*> clientPhoneTypesSelector[2];
    QStandardItemModel* prepayReasonsModel;
    QList<QStandardItem*> *prepayReason;
    QSqlQueryModel* clientsMatchTable;
    QSqlQueryModel* devicesMatchTable;
    QVector<QWidget*> additionalFieldsWidgets;
    int client = 0;
    void phoneTypeChanged(int, int);
    QMessageBox msgBox;
#ifdef QT_DEBUG
    QTimer *test_scheduler, *test_scheduler2, *main_window_test_scheduler, *main_window_test_scheduler2;
    uint test_scheduler_counter = 0;
#endif

    groupBoxEventFilter *groupBoxEventFilterObj;
    QString commonComboBoxStyleSheet = "QComboBox {  border: 1px solid gray;  padding: 1px 18px 1px 3px;}\
            QComboBox::drop-down {  border: 0px;}\
            QComboBox::down-arrow{  image: url(down-arrow.png);  width: 16px;  height: 20px;}\
            QComboBox::hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}\
            QComboBox::down-arrow:hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}";
    QString commonLineEditStyleSheet = "";
    QString commonDateEditStyleSheet = "";

    QString commonComboBoxStyleSheetRed = "QComboBox {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}\
            QComboBox::drop-down {  border: 0px;}\
            QComboBox::down-arrow{  image: url(down-arrow.png);  width: 16px;  height: 20px;}\
            QComboBox::hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}\
            QComboBox::down-arrow:hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}";
    QString commonLineEditStyleSheetRed = "QLineEdit {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}";
    QString commonDateEditStyleSheetRed = "QDateEdit {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}";

private slots:
    void changeClientType();
    void showLineEditPrevRepair();
    void enablePrepayWidgets(bool);
    void changeDeviceType();
    void changeDeviceMaker();
    void clearClientCreds(bool hideCoincidence = true);
    void lineEditPrevRepairButtonsHandler(int);
    void fillClientCreds(int);
    void fillDeviceCreds(int);
    void buttonSelectExistingClientHandler();
    void buttonCreateTabClientHandler();
    void findMatchingClient(QString);
    void findMatchingDevice(QString);
    void phone1TypeChanged(int);
    void phone2TypeChanged(int);
    void deviceMatchTableDoubleClicked(QModelIndex);
    void clientMatchTableDoubleClicked(QModelIndex);
    void lineEditSNClearHandler(int);
    int createRepair();
    void createRepairClose();
#ifdef QT_DEBUG
    void test_scheduler_handler();
    void test_scheduler2_handler();
#endif
};

#endif // TABREPAIRNEW_H
