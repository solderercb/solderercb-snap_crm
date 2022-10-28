#include "global.h"
#include "appver.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "bottoolbarwidget.h"
#include "tabcommon.h"
#include "tabrepairnew.h"
#include "tabrepairs.h"
#include "tabrepair.h"
#include "tabsale.h"
#include "tabclients.h"
#include "tabclient.h"
#include "tabprintdialog.h"
#include "tabcashoperation.h"
#include "tabcashmoveexch.h"
#include "widgets/slineedit.h"
#include "com_sql_queries.h"

tabBarEventFilter::tabBarEventFilter(QObject *parent) :
    QObject(parent)
{
}

bool tabBarEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)  // закрытие вкладки по клику средней кнопкой мыши (колёсиком)
    {
        QTabBar *tabBar = static_cast<QTabBar*>(watched);
        QMouseEvent *mouseButtonPress = static_cast<QMouseEvent*>(event);

        if (mouseButtonPress->button() == Qt::MiddleButton)
        {
            QPoint point;
            int tabId = -1;
#if QT_VERSION >= 0x060000
            point = mouseButtonPress->position().toPoint();
#else
            point = mouseButtonPress->localPos().toPoint();
#endif
            tabId = tabBar->tabAt(point);
            if(tabId >= 0)
                emit tabBar->tabCloseRequested(tabId);
        }
    }
    return false;
}

MainWindow* MainWindow::p_instance = nullptr;

MainWindow::MainWindow(windowsDispatcher*) :
    QMainWindow(nullptr),
	ui(new Ui::MainWindow)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF8");
	QTextCodec::setCodecForLocale(codec);

    ui->setupUi(this);

    setWindowTitle("SNAP CRM ["+userDbData->value("current_office_name").toString()+"] ["+QSqlDatabase::database("connMain").userName()+"]");
    initGlobalModels();

    tabBarEventFilter *tabBarEventFilterObj = new tabBarEventFilter(this);  // Фильтр событий tabBar. В частности, закрытие вкладки по клику средней кнопкой мыши (колёсиком)
    ui->tabWidget->tabBar()->installEventFilter(tabBarEventFilterObj);

    this->move(0, 0);   // размер и положение окна по умолчанию
    this->resize(1440, 960);
    if(userLocalData->value("WorkspaceState").toString() == "Maximized")
    {
        setWindowState(Qt::WindowMaximized);
    }
    else
    {
        this->move(userLocalData->value("WorkspaceLeft").toInt(), userLocalData->value("WorkspaceTop").toInt());
        this->resize(userLocalData->value("WorkspaceWidth").toInt(), userLocalData->value("WorkspaceHeight").toInt());
    }

    createMenu();
#ifdef QT_DEBUG
    createTestTab();

    btnClick();

    test_scheduler = new QTimer();
    test_scheduler->setSingleShot(true);
    test_scheduler2 = new QTimer();
    test_scheduler2->setSingleShot(true);
    QObject::connect(test_scheduler, SIGNAL(timeout()), this, SLOT(test_scheduler_handler()));
    QObject::connect(test_scheduler2, SIGNAL(timeout()), this, SLOT(test_scheduler2_handler()));
    test_scheduler->start(200);
#endif

    for(int i = ui->tabWidget->count() - 1; i >= 0; i--)    // закрытие всех вкладок, настроенных в дизайнере
        closeTab(i);
}

// https://stackoverflow.com/a/17482796
void MainWindow::closeEvent(QCloseEvent *event)
{
//    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "APP_NAME",
//                                                                tr("Are you sure?\n"),
//                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
//                                                                QMessageBox::Yes);
//    if (resBtn != QMessageBox::Yes) {
//        event->ignore();
//    } else {
//        event->accept();
//    }
    appLog->appendRecord(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " Normal application exit");
    appLog->appendRecord("\r\n\r\n\r\n");
    userActivityLog->appendRecord("Logout");
}

void MainWindow::createMenu()
{
    QString toolButtonStyle = "QToolButton{\
                                   border: 0px;\
                                   background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\
                                                                     stop: 0 #f6f7fa, stop: 1 #dadbde);\
                               }\
                               \
                               QToolButton::menu-button {\
                                   width: 32px;\
                               }";
    ui->toolBar->setStyleSheet("QToolBar::separator {width: 10px; border: none;}");

    /* Кнопка Ремонты и меню */
    QMenu *workshop_menu = new QMenu();
    QAction *workshop_new = new QAction(tr("Принять"), this);
    workshop_menu->addAction(workshop_new);
    QObject::connect(workshop_new,SIGNAL(triggered()),this,SLOT(createTabRepairNew()));
    QAction *workshop_refill = new QAction(tr("Заправка"), this);
    workshop_menu->addAction(workshop_refill);
    workshop_refill->setEnabled(false);
    QAction *workshop_price = new QAction(tr("Прайс-лист"), this);
    workshop_menu->addAction(workshop_price);
    workshop_price->setEnabled(false);
    QAction *workshop_editor = new QAction(tr("Групповой редактор ремонтов"), this);
    workshop_menu->addAction(workshop_editor);
    workshop_editor->setEnabled(false);
    QToolButton* workshop_button = new QToolButton();
    workshop_button->setMenu(workshop_menu);
    workshop_button->setPopupMode(QToolButton::MenuButtonPopup);
    workshop_button->setText(tr("Ремонты"));
    workshop_button->setFixedSize(128,48);
    workshop_button->setStyleSheet(toolButtonStyle);
    ui->toolBar->addWidget(workshop_button);
    ui->toolBar->addSeparator();
    QObject::connect(workshop_button, SIGNAL(clicked()), this, SLOT(createTabRepairs()));

    /* Кнопка Товары и меню */
    QMenu *goods_menu = new QMenu();
    QAction *goods_arrival = new QAction(tr("Приход"), this);
    goods_menu->addAction(goods_arrival);
    goods_arrival->setEnabled(false);
    QAction *goods_sale = new QAction(tr("Продажа"), this);
    goods_menu->addAction(goods_sale);
    QObject::connect(goods_sale,SIGNAL(triggered()),this,SLOT(createTabSale()));
    QAction *store_docs = new QAction(tr("Документы"), this);
    goods_menu->addAction(store_docs);
    store_docs->setEnabled(false);
    QAction *purchase_manager = new QAction(tr("Менеджер закупок"), this);
    goods_menu->addAction(purchase_manager);
    purchase_manager->setEnabled(false);
    QAction *store_manager = new QAction(tr("Управление складом"), this);
    goods_menu->addAction(store_manager);
    store_manager->setEnabled(false);
    QAction *goods_editor = new QAction(tr("Групповой редактор товаров"), this);
    goods_menu->addAction(goods_editor);
    goods_editor->setEnabled(false);
    QAction *goods_uploader = new QAction(tr("Выгрузка товаров"), this);
    goods_menu->addAction(goods_uploader);
    goods_uploader->setEnabled(false);
    QAction *device_buyout = new QAction(tr("Выкуп техники"), this);
    goods_menu->addAction(device_buyout);
    device_buyout->setEnabled(false);
    QAction *stocktaking = new QAction(tr("Переучет"), this);
    goods_menu->addAction(stocktaking);
    stocktaking->setEnabled(false);
    QToolButton* goods_button = new QToolButton();
    goods_button->setMenu(goods_menu);
    goods_button->setPopupMode(QToolButton::MenuButtonPopup);
    goods_button->setText(tr("Товары"));
    goods_button->setFixedSize(128,48);
    goods_button->setStyleSheet(toolButtonStyle);
    ui->toolBar->addWidget(goods_button);
    ui->toolBar->addSeparator();

    /* Кнопка Клиенты и меню */
    QMenu *clients_menu = new QMenu();
    QAction *client_new = new QAction(tr("Новый клиент"), this);
    clients_menu->addAction(client_new);
    client_new->setEnabled(false);
    QAction *clients_calls = new QAction(tr("Вызовы"), this);
    clients_menu->addAction(clients_calls);
    clients_calls->setEnabled(false);
    QAction *clients_sms = new QAction(tr("SMS"), this);
    clients_menu->addAction(clients_sms);
    clients_sms->setEnabled(false);
//    QAction *clients_editor = new QAction(tr("Групповой редактор клиентов"), this);
//    clients_menu->addAction(clients_editor);
    QToolButton* clients_button = new QToolButton();
    clients_button->setMenu(clients_menu);
    clients_button->setPopupMode(QToolButton::MenuButtonPopup);
    clients_button->setText(tr("Клиенты"));
    clients_button->setFixedSize(128,48);
    clients_button->setStyleSheet(toolButtonStyle);
    ui->toolBar->addWidget(clients_button);
    QObject::connect(clients_button, SIGNAL(clicked()), this, SLOT(createTabClients()));
    ui->toolBar->addSeparator();

    /* Кнопка Финансы и меню */
    QMenu *finances_menu = new QMenu();
    QAction *pko_new = new QAction(tr("Приходный кассовый ордер"), this);
    finances_menu->addAction(pko_new);
    QObject::connect(pko_new,SIGNAL(triggered()),this,SLOT(createTabNewPKO()));
    QAction *rko_new = new QAction(tr("Расходный кассовый ордер"), this);
    finances_menu->addAction(rko_new);
    QObject::connect(rko_new,SIGNAL(triggered()),this,SLOT(createTabNewRKO()));
    QAction *move_new = new QAction(tr("Перемещение/обмен"), this);
    finances_menu->addAction(move_new);
    QObject::connect(move_new,SIGNAL(triggered()),this,SLOT(createTabCashMoveExch()));
    QAction *documents = new QAction(tr("Документы"), this);
    finances_menu->addAction(documents);

    QMenu *documents_submenu = new QMenu();
    QAction *invoice = new QAction(tr("Счёт"), this);
    documents_submenu->addAction(invoice);
    invoice->setEnabled(false);
    QAction *bill = new QAction(tr("Счёт-фактура"), this);
    documents_submenu->addAction(bill);
    bill->setEnabled(false);
    QAction *goodsBill = new QAction(tr("Товарная накладная"), this);
    documents_submenu->addAction(goodsBill);
    goodsBill->setEnabled(false);
    QAction *worksBill = new QAction(tr("Акт выполненных работ"), this);
    documents_submenu->addAction(worksBill);
    worksBill->setEnabled(false);
    documents->setMenu(documents_submenu);

    QAction *salary = new QAction(tr("Заработная плата"), this);
    finances_menu->addAction(salary);
    salary->setEnabled(false);
    QToolButton* finances_button = new QToolButton();
    finances_button->setMenu(finances_menu);
    finances_button->setPopupMode(QToolButton::MenuButtonPopup);
    finances_button->setText(tr("Финансы"));
    finances_button->setFixedSize(128,48);
    finances_button->setStyleSheet(toolButtonStyle);
    ui->toolBar->addWidget(finances_button);
    QObject::connect(finances_button, SIGNAL(clicked()), this, SLOT(createTabCashOperations()));
}

MainWindow::~MainWindow()
{
    delete ui;
    p_instance = nullptr;
    delete warehousesModel;
    delete usersModel;
    delete managersModel;
    delete engineersModel;
    delete repairBoxesModel;
    for (int i = 0; i<connections.size(); i++)  // закрываем соединения
        connections[i]->close();
#ifdef QT_DEBUG
    delete tableConsignmentsModel;
    delete tableGoodsModel;
    delete tree_model;
    delete comboboxSourceModel;
#endif
}

MainWindow* MainWindow::getInstance(windowsDispatcher *parent)   // singleton: MainWindow только один объект
{
    if( !p_instance )
      p_instance = new MainWindow(parent);

    return p_instance;
}

void MainWindow::createTabRepairs(int type, QWidget* caller)
{
    tabRepairs *subwindow = tabRepairs::getInstance(type, this);
    if (ui->tabWidget->indexOf(subwindow) == -1) // Если такой вкладки еще нет, то добавляем
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());

    if (type == 0)
    {
        QObject::connect(subwindow,SIGNAL(doubleClicked(int)), this, SLOT(createTabRepair(int)));
    }
    else
    {
        QObject::connect(subwindow,SIGNAL(buttonRepairNewClicked()), this, SLOT(createTabRepairNew()));
        // Сигнал экземпляра вкладки выбора предыдущего ремонта подключаем к слоту вызывающей вкладки для заполнения соотв. полей
        // и к слоту MainWindow, в котором происходит переключение на вкладку приёма в ремонт при закрытии вкладки выбора ремонта
        QObject::connect(subwindow,SIGNAL(doubleClicked(int)), caller, SLOT(fillDeviceCreds(int)));
        subwindow->setCallerPtr(caller);
        QObject::connect(subwindow,SIGNAL(activateCaller(QWidget*)),this,SLOT(reactivateCallerTab(QWidget*)));
    }

    ui->tabWidget->setCurrentWidget(subwindow); // Переключаемся на вкладку Ремонты/Выбрать ремонт
}

void MainWindow::createTabRepair(int repair_id)
{
    tabRepair *subwindow = tabRepair::getInstance(repair_id, this);
    if(ui->tabWidget->indexOf(subwindow) == -1)
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    updateTabIcon(subwindow);
    ui->tabWidget->setCurrentWidget(subwindow);
    QObject::connect(subwindow,SIGNAL(createTabPrevRepair(int)), this, SLOT(createTabRepair(int)));
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString,QVariant>)), this, SLOT(createTabPrint(QMap<QString,QVariant>)));
    QObject::connect(subwindow,SIGNAL(createTabClient(int)), this, SLOT(createTabClient(int)));
    QObject::connect(subwindow,SIGNAL(updateTabTitle(QWidget*)), this, SLOT(updateTabTitle(QWidget*)));
}

void MainWindow::createTabRepairNew()
{
    tabRepairNew *subwindow = tabRepairNew::getInstance(this);
    if (ui->tabWidget->indexOf(subwindow) == -1) // Если такой вкладки еще нет, то добавляем
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    ui->tabWidget->setCurrentWidget(subwindow); // Переключаемся на вкладку Приём в ремонт
    QObject::connect(subwindow,SIGNAL(createTabSelectPrevRepair(int,QWidget*)), this, SLOT(createTabRepairs(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabSelectExistingClient(int,QWidget*)), this, SLOT(createTabClients(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabClient(int)), this, SLOT(createTabClient(int)));
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString,QVariant>)), this, SLOT(createTabPrint(QMap<QString,QVariant>)));
}

void MainWindow::createTabPrint(QMap<QString, QVariant> report_vars)
{
    report_vars.detach();
//    qDebug() << "MainWindow::createTabPrint()";
//    qDebug() << report_vars;
    tabPrintDialog *subwindow;

    subwindow = new tabPrintDialog(this, report_vars);
    ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    ui->tabWidget->setCurrentWidget(subwindow);
}

/*  Вкладка со списком проведённых кассовых операций
 */
void MainWindow::createTabCashOperations()
{

}

/* Вкладка создания новой кассовой операции или просмотра проведённой операции
 */
void MainWindow::createTabCashOperation(int orderId, QMap<int, QVariant> data)
{
    tabCashOperation *subwindow = tabCashOperation::getInstance(orderId, this);
    if (ui->tabWidget->indexOf(subwindow) == -1) // Если такой вкладки еще нет, то добавляем
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    QObject::connect(subwindow,SIGNAL(createTabSelectExistingClient(int,QWidget*)), this, SLOT(createTabClients(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabSelectRepair(int,QWidget*)), this, SLOT(createTabRepairs(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabSelectDocument(int,QWidget*)), this, SLOT(createTabDocuments(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabSelectInvoice(int,QWidget*)), this, SLOT(createTabInvoices(int,QWidget*)));
    QObject::connect(subwindow,SIGNAL(updateTabTitle(QWidget*)), this, SLOT(updateTabTitle(QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabUndoOperation(int,QMap<int,QVariant>)), this, SLOT(createTabCashOperation(int,QMap<int,QVariant>)));
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString,QVariant>)), this, SLOT(createTabPrint(QMap<QString,QVariant>)));
    subwindow->prepareTemplate(data);

    ui->tabWidget->setCurrentWidget(subwindow);
}

void MainWindow::createTabNewPKO()
{
    createTabCashOperation(tabCashOperation::PKO);
}

void MainWindow::createTabNewRKO()
{
    createTabCashOperation(tabCashOperation::RKO);
}

/* Вкладка перемещения/обмена денежных средств
 */
void MainWindow::createTabCashMoveExch()
{
    tabCashMoveExch *subwindow = tabCashMoveExch::getInstance(this);
    if (ui->tabWidget->indexOf(subwindow) == -1) // Если такой вкладки еще нет, то добавляем
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    QObject::connect(subwindow,SIGNAL(updateTabTitle(QWidget*)), this, SLOT(updateTabTitle(QWidget*)));
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString,QVariant>)), this, SLOT(createTabPrint(QMap<QString,QVariant>)));

    ui->tabWidget->setCurrentWidget(subwindow);
}
void MainWindow::createTabDocuments(int type, QWidget *caller)
{
    qDebug().nospace() << "[MainWindow] createTabDocuments()";
}

void MainWindow::createTabInvoices(int type, QWidget *caller)
{
    qDebug().nospace() << "[MainWindow] createTabInvoices()";
}

void MainWindow::reactivateCallerTab(QWidget *caller)
{
    ui->tabWidget->setCurrentWidget(caller); // Переключаемся на вкладку caller
}

void MainWindow::reactivateTabRepairNew(int)
{
    ui->tabWidget->setCurrentWidget(tabRepairNew::getInstance());
}

void MainWindow::createTabSale(int doc_id)
{
    tabSale *subwindow = tabSale::getInstance(doc_id, this);
    if(ui->tabWidget->indexOf(subwindow) == -1)
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    ui->tabWidget->setCurrentWidget(subwindow);
    QObject::connect(subwindow,SIGNAL(createTabSelectExistingClient(int, QWidget *)), this, SLOT(createTabClients(int, QWidget *)));
    QObject::connect(subwindow,SIGNAL(createTabClient(int)), this, SLOT(createTabClient(int)));
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString, QVariant>)), this, SLOT(createTabPrint(QMap<QString, QVariant>)));
    QObject::connect(subwindow,SIGNAL(updateTabTitle(QWidget*)), this, SLOT(updateTabTitle(QWidget*)));
    QObject::connect(subwindow,SIGNAL(createTabClient(int)), this, SLOT(createTabClient(int)));
}

/*  Создание вкладки Клиенты (type = 0) или Выбрать клиента (type = 1)
 *  caller - указатель на вызывающую вкладку-виджет, необходимый для правильной передачи id выбранного клиента
 *  (данный слот может быть вызван с вкладок Приём в ремонт и Продать, а в будущем и других)
*/
void MainWindow::createTabClients(int type, QWidget *caller)
{
    tabClients *subwindow = tabClients::getInstance(type, this);
    if (ui->tabWidget->indexOf(subwindow) == -1) // Если такой вкладки еще нет
    {
        if(caller)  // если передан параметр, то вставляем её сразу после вызывающей
            ui->tabWidget->insertTab(ui->tabWidget->indexOf(caller) + 1, subwindow, subwindow->tabTitle());
        else    // иначе добавляем в конец
            ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    }

    if (type == 0)
    {
        //
        QObject::connect(subwindow,SIGNAL(doubleClicked(int)), this, SLOT(createTabClient(int)));
    }
    else
    {
        // Сигнал вкладки выбора клиента подключаем к слоту вызывающей вкладки для заполнения соотв. полей
        QObject::connect(subwindow,SIGNAL(doubleClicked(int)),caller,SLOT(fillClientCreds(int)));
        // а чтобы после выбора клиента произошло автоматическое переключение на вызывающую вкладку, требуются доп. "телодвижения"
        subwindow->setCallerPtr(caller);
        QObject::connect(subwindow,SIGNAL(activateCaller(QWidget*)),this,SLOT(reactivateCallerTab(QWidget*)));
    }

    ui->tabWidget->setCurrentWidget(subwindow);
    subwindow->lineEditSearchSetFocus();

}

void MainWindow::createTabClient(int id)
{
//    if(!permissions->value("X"))    // TODO: разрешение на просмотр карты клиента
//        return;
    tabClient *subwindow = tabClient::getInstance(id, this);
    if(ui->tabWidget->indexOf(subwindow) == -1)
        ui->tabWidget->addTab(subwindow, subwindow->tabTitle());
    ui->tabWidget->setCurrentWidget(subwindow);
    QObject::connect(subwindow,SIGNAL(generatePrintout(QMap<QString,QVariant>)), this, SLOT(createTabPrint(QMap<QString,QVariant>)));
}

void MainWindow::closeTab(int index)
{
    QWidget *w = ui->tabWidget->widget(index);
    if (QString(w->metaObject()->className()).compare("QWidget", Qt::CaseSensitive) != 0)  // Временное: на момент первоначальной разработки могут использоваться обычные QWidget-вкладки и при их закрытии приложение будет падать
    {
        tabCommon &tab = *(static_cast<tabCommon*>(w)); // мои классы ОБЯЗАТЕЛЬНО должны наследовать tabCommon
        if (tab.tabCloseRequest())   // Перед закрытием  вкладки нужно проверить нет ли несохранённых данных
        {
            delete w;
//            ui->tabWidget->removeTab(index);
            return;
        }
    }
    delete w;
    }

void MainWindow::updateTabTitle(QWidget *w)
{
    ui->tabWidget->setTabText(ui->tabWidget->indexOf(w), static_cast<tabCommon*>(w)->tabTitle());
    updateTabIcon(w);
}

void MainWindow::updateTabIcon(QWidget *w)
{
    QIcon *icon;

    icon = static_cast<tabCommon*>(w)->tabIcon();
    if(icon)
        ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(w), *icon);
    else
        ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(w), QIcon());
}

#ifdef QT_DEBUG
void MainWindow::btnClick()
{
    enum {store_cats_id, store_cats_parent, store_cats_name, store_cats_position};

    if (!(QSqlDatabase::database("connMain").isOpen()))
    {
        return;
    }

    QSqlQuery* cat_tree = new QSqlQuery(QSqlDatabase::database("connMain"));
//    QStandardItem* parentItem;
//    QStandardItem* childItem;
    QVector<uint32_t> store_cats_ids;
    QVector<QStandardItem*> store_cats;
    uint32_t parent = 0, i = 0, j = 0;

//    QStandardItem* testItem = new QStandardItem("test");

//	parentItem = tree_model->invisibleRootItem();
    cat_tree->exec("SELECT `id`,`parent`,`name` FROM store_cats WHERE `store_id` = 1 AND `enable` = 1 ORDER BY `parent`, `position`;");

    store_cats.resize(cat_tree->size());
    store_cats_ids.resize(cat_tree->size());

    while(cat_tree->next())
    {
        QStandardItem* item = new QStandardItem(cat_tree->value(store_cats_name).toString());
        item->setData(cat_tree->value(store_cats_id).toString(), Qt::UserRole+1); // Кроме текстовой записи сохраняем данные столбцов code и parent
        item->setData(cat_tree->value(store_cats_parent).toString(), Qt::UserRole+2); // Работает. НЕ ТРОГАТЬ!
        if(!cat_tree->value(store_cats_parent).toInt()) // Если текущий элемент выборки не содержит значения parent, то сразу добавляем его в TreeView (это корневой эл-т)
            tree_model->appendRow(item);
        store_cats_ids[i] = cat_tree->value(store_cats_id).toInt();
        store_cats[i] = item;  // указатели на объекты храним в массиве; ключ массива — id категории товара
        i++;

//        qDebug() << cat_tree->value(store_cats_id).toInt();
    }
    for(uint i=0; i < store_cats.size(); i++)
    {
//        if (store_cats[i])  // обработку производим только тех эл-тов массива, в которых содержится указатель на объект
//        {
//            childItem = store_cats[i];
            parent = store_cats[i]->data(Qt::UserRole+2).toInt();
            if(parent)  // если записан parent
            {
                j = 0;
                while (store_cats_ids[j] != parent)
                    j++;
//                parentItem = store_cats[j];
//                qDebug() << store_cats[i]->text().toStdWString() << " and it's parent " << store_cats[j]->text().toStdWString();
                store_cats[j]->appendRow(store_cats[i]);
            }
//        }
    }
    delete cat_tree;
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    readGoods(index, ui->comboBoxSourceWarehouse->currentData(Qt::UserRole+1).toString());
}

void MainWindow::on_treeView_activated(const QModelIndex &index)
{
    readGoods(index, ui->comboBoxSourceWarehouse->currentData(Qt::UserRole+1).toString());
}

void MainWindow::readGoods(const QModelIndex &index, const QString &warehouse_code)
{
    if (!(QSqlDatabase::database("connMain").isOpen()))
    {
        return;
    }

    QVariant a = ui->treeView->model()->data(index, Qt::UserRole+1);
    QVariant b = ui->treeView->model()->data(index, Qt::UserRole+2);
    ui->label->setText("code: "
                                         + a.toString()
                                         + "; parent: "
                                         + b.toString()); // Работает. НЕ ТРОГАТЬ!

    tableGoodsModel->setRowCount(0); // Удаляем имеющиеся строки
    QSqlQuery* goods = new QSqlQuery(QSqlDatabase::database("connMain"));
    QString query;
    QStandardItem* newCol;

    query = QString("SELECT t1.`articul`, t1.`name`, t1.`description` FROM store_items AS t1 WHERE t1.`category` LIKE '%%1%' AND t1.`count`>0 AND t1.`store` LIKE '%2' GROUP BY t1.`articul` ORDER BY t1.`articul`;").arg(ui->treeView->model()->data(index, Qt::UserRole+1).toString(),warehouse_code);
    goods->exec(query);

    while(goods->next())
    {
        QList<QStandardItem*> newRow;

        for(uint8_t i=0;i<3;i++)
        {
            newCol = new QStandardItem(goods->value(i).toString()); // Артикул, Наименование и Примечание
            newRow.append(newCol);
        }

        tableGoodsModel->appendRow(newRow);
    }

    delete goods;
}

void MainWindow::get_warehouses_list()
{
    if (!(QSqlDatabase::database("connMain").isOpen()))
    {
        return;
    }

    QSqlQuery* warehouse_list = new QSqlQuery(QSqlDatabase::database("connMain"));
    QString query;
    QStandardItem *newRow;
    QStandardItem *newRow2;

    query = QString("SELECT `name`, `id`, `office` FROM stores WHERE `active` = 1;");
    warehouse_list->exec(query);

    newRow = new QStandardItem();
    newRow->setText("All");
    newRow->setData("0",Qt::UserRole+1);
    comboboxSourceModel->appendRow(newRow);	// Добавляем строку в модель comboBox (склад-источник)

    while(warehouse_list->next())
    {
        newRow = new QStandardItem();
        newRow2 = new QStandardItem();
        newRow->setText(warehouse_list->value(0).toString());
        newRow->setData(warehouse_list->value(1).toString(),Qt::UserRole+1);
        newRow2->setText(warehouse_list->value(0).toString());
        newRow2->setData(warehouse_list->value(1).toString(),Qt::UserRole+1);
        comboboxSourceModel->appendRow(newRow);	// Добавляем строку в модель comboBox (склад-источник)
        comboboxDestModel->appendRow(newRow2);	// Добавляем строку в модель comboBox (склад-приёмник)
    }

//	ui->comboBoxDestWarehouse->setCurrentIndex(1); // По умолчанию складом-приёмником является второй элемент в списке

    delete warehouse_list;
}

void MainWindow::createTestTab()
{

    QPushButton* pushButton05 = new QPushButton();
    pushButton05->setText("pushButton05");
//    pushButton05->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    ui->gridLayout_4->addWidget(pushButton05, 0, 0, 1, 1);
    QPushButton* pushButton04 = new QPushButton();
    pushButton04->setText("pushButton04");
    ui->gridLayout_4->addWidget(pushButton04, 1, 1, 1, 1);
    QPushButton* pushButton03 = new QPushButton();
    pushButton03->setText("pushButton03");
    ui->gridLayout_4->addWidget(pushButton03, 2, 2, 1, 1);
    QPushButton* pushButton02 = new QPushButton();
    pushButton02->setText("pushButton02");
    ui->gridLayout_4->addWidget(pushButton02, 3, 3, 1, 1);
    QPushButton* pushButton01 = new QPushButton();
    pushButton01->setText("pushButton01");
    ui->gridLayout_4->addWidget(pushButton01, 4, 4, 1, 1);

    comboboxSourceModel = new QStandardItemModel();
    ui->comboBoxSourceWarehouse->setModel(comboboxSourceModel);
    comboboxDestModel = new QStandardItemModel();
    ui->comboBoxDestWarehouse->setModel(comboboxDestModel);

    tree_model = new QStandardItemModel();
    ui->treeView->setUniformRowHeights(true); // all items in the treeview have the same height
    ui->treeView->setModel(tree_model);

    tableGoodsModel = new QStandardItemModel(0,3);	// Три столбца
//	proxy_tableGoodsHeaders = new QSortFilterProxyModel(this);
//	proxy_tableGoodsHeaders->setSourceModel(tableGoodsModel);
//	proxy_tableGoodsHeaders->setSortRole(Qt::EditRole);
    ui->tableGoods->setModel(tableGoodsModel);
//	ui->tableGoods->setModel(proxy_tableGoodsHeaders);
//	ui->tableGoods->horizontalHeader()->setEnabled(true);
    tableGoodsHeaders << "Артикул" << "Наименование" << "Примечание";
    tableGoodsModel->setHorizontalHeaderLabels(tableGoodsHeaders);
    ui->tableGoods->horizontalHeader()->setStretchLastSection(true);
//	or
//	ui->tableGoods->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // Ширина столбцов одинаковая, неизменяемая.
//	ui->label->setText(QString::number(ui->tableGoods->geometry().width()));
    ui->tableGoods->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableGoods->verticalHeader()->setDefaultSectionSize(18);
    ui->tableGoods->setColumnWidth(0, 50);
    ui->tableGoods->setColumnWidth(1, 200);
/*
    ui->tableGoods->setColumnWidth(0, (ui->verticalLayout_3->geometry().width()-qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent))*0.12-1);
    ui->tableGoods->setColumnWidth(1, (ui->verticalLayout_3->geometry().width()-qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent))*0.44-1);
    ui->tableGoods->setColumnWidth(2, (ui->verticalLayout_3->geometry().width()-qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent))*0.44-1);
*/

    tableConsignmentsModel = new QStandardItemModel(0,8); // Семь столбцов
    tableConsignmentsHeaders << "Артикул" << "Наименование" << "Примечание" << "Кол-во" << "Реализация" << "Резерв" << "Цена" << "Место";
    tableConsignmentsModel->setHorizontalHeaderLabels(tableConsignmentsHeaders);
    ui->tableConsignments->setModel(tableConsignmentsModel);
    ui->tableConsignments->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableConsignments->verticalHeader()->setDefaultSectionSize(18);

    QObject::connect(ui->btnSendQuery,SIGNAL(clicked(bool)),this,SLOT(btnClick()));
    get_warehouses_list();
}

// При смене склада обновляем таблицу
void MainWindow::on_comboBoxSourceWarehouse_currentIndexChanged(int cur_src_index)
{
    uint8_t max_dest_index = ui->comboBoxDestWarehouse->model()->rowCount(); // Кол-во строк combobox склада-приёмника [0..x]
    uint8_t cur_dest_index = ui->comboBoxDestWarehouse->currentIndex();	// текущий индекс эл-та combobox склада-приёмника
    uint8_t new_dest_index = cur_dest_index;	// новый индекс эл-та combobox склада-приёмника
    if (cur_dest_index == cur_src_index) // Если индекс элемента combobox склада-приёмника равен индексу эл-та combobox склада-источника
    {
        if (new_dest_index==max_dest_index-1)	// Если после увеличения индекса окажется, что он превышает кол-во элементов combobox'а склада-приёмника
            new_dest_index--;	// то уменьшаем индекс элемента combobox склада-приёмника
        else
            new_dest_index++;	// иначе увеличиваем индекс элемента combobox склада-приёмника
    }
    ui->comboBoxDestWarehouse->setCurrentIndex(new_dest_index);	// переключаем combobox
    for (uint8_t i=0;i<max_dest_index;i++)	// в цикле включаем все элементы combobox'а
        if(comboboxDestModel->index(i,0).isValid()) // действие будет выполняться только для действительных элементов
            comboboxDestModel->item(i)->setEnabled(1);
    if(comboboxDestModel->index(cur_src_index,0).isValid())	// действие будет выполняться только для действительных элементов
        comboboxDestModel->item(cur_src_index)->setEnabled(0); // отключаем пункт списка (будет отображаться, но его нельзя будет выбрать)

    readGoods(ui->treeView->currentIndex(), ui->comboBoxSourceWarehouse->itemData(cur_src_index, Qt::UserRole+1).toString());	// перечитываем таблицу товаров
 }

void MainWindow::on_tableGoods_activated(const QModelIndex &index)
{
    readConsignments(index, ui->comboBoxSourceWarehouse->currentData(Qt::UserRole+1).toString());
}

void MainWindow::on_tableGoods_clicked(const QModelIndex &index)
{
    readConsignments(index, ui->comboBoxSourceWarehouse->currentData(Qt::UserRole+1).toString());
}

void MainWindow::readConsignments(const QModelIndex &index, const QString &warehouse_code)
{
    if (!(QSqlDatabase::database("connMain").isOpen()))
    {
        return;
    }

    ui->label->setText("Art: " + index.model()->index(index.row(),0).data().toString());

    tableConsignmentsModel->setRowCount(0); // Удаляем имеющиеся строки
    QSqlQuery* consignments = new QSqlQuery(QSqlDatabase::database("connMain"));
    QString query;
    QStandardItem* newCol;
//	"Артикул" << "Наименование" << "Примечание" << "Кол-во" << "Реализация" << "Резерв" << "Цена" << "Место"
    query = QString("SELECT `art`,`name`,`primech`,`kolv`-`sale`-`reserv`,`rent`,`reserv`,`price3`,`place`,`index_i` FROM goods_base WHERE `art` = %1 AND `kolv`-`sale`-`reserv`>0 AND `sklad` LIKE '%2' AND `status` = 1;").arg(index.model()->index(index.row(),0).data().toString(),warehouse_code);
    consignments->exec(query);

    while(consignments->next())
    {
        QList<QStandardItem*> newRow;

        for(uint8_t i=0;i<8;i++)
        {
            newCol = new QStandardItem(consignments->value(i).toString()); // Артикул, Наименование, Примечание, Кол-во доступно, Кол-во на реализации, Кол-во в резерве, Цена и Место хранения.
            newRow.append(newCol);
        }

        tableConsignmentsModel->appendRow(newRow);
    }

    delete consignments;
//	ui->comboBoxSourceWarehouse->currentData(Qt::UserRole+1).toString();	// source warehouse selected in comboboxSourceWarehouse
//	ui->treeView->model()->data(index, Qt::UserRole+1).toString();	// index data
}

void MainWindow::test_scheduler_handler()  // обработик таймера открытия вкладки
{
    qDebug() << "test_scheduler_handler(), test_scheduler_counter = " << test_scheduler_counter++;
//    createTabClients(0);
//    QSqlQuery rand_rep_id = QSqlQuery(QSqlDatabase::database("connMain"));
//    rand_rep_id.exec("SELECT ROUND(RAND()*25000, 0) INTO @id;");
//    rand_rep_id.exec("SELECT `id` FROM `workshop` WHERE `id` >= @id AND `hidden` = 0 LIMIT 1;");
//    qDebug() << rand_rep_id.lastError().databaseText();
//    if (rand_rep_id.next())
//    {
//        qDebug() << rand_rep_id.value(0);
//        createTabRepair(rand_rep_id.value(0).toInt());
//    }
    createTabRepair(25481);
//    createTabSale(16316);
//    createTabSale(0);
//    if (test_scheduler_counter < 375)
//    {
//        createTabRepairNew();
//        createTabNewPKO();
//        createTabCashOperation(36192);
//        createTabCashOperation(42019);
//        createTabClient(143);
//        createTabCashMoveExch();
//        QMap<QString, QVariant> report_vars;
//        report_vars.insert("repair_id", 24972);
//        report_vars.insert("type", "new_rep");
//        createTabPrint(report_vars);
//        QMap<QString, QVariant> report_vars2;
//        report_vars2.insert("type", "rep_label");
//        report_vars2.insert("repair_id", 24972);
//        createTabPrint(report_vars2);
//    }
//    test_scheduler2->start(1000);    //  (пере-)запускаем таймер закрытия вкладки

}

void MainWindow::test_scheduler2_handler()  // обработик таймера закрытия вкладки
{
//    qDebug() << "MainWindow::test_scheduler2_handler(), clientTabId = " << ui->tabWidget->indexOf(tabRepairNew::getInstance(this));
    closeTab(ui->tabWidget->indexOf(tabRepairNew::getInstance(0)));
    test_scheduler->start(500);    //  перезапускаем таймер открытия вкладки
}
#endif

