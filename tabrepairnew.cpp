#include "global.h"
#include "appver.h"
#include "tabrepairnew.h"
#include "tabrepairs.h"
#include "ui_tabrepairnew.h"
#include "com_sql_queries.h"
#include "models/sofficemodel.h"
#include "tabprintdialog.h"

tabRepairNew* tabRepairNew::p_instance = nullptr;

enum addlField {LineEdit = 1, ComboBox, DateEdit, dummy};

tabRepairNew::tabRepairNew(MainWindow *parent) :
    tabCommon(parent),
    ui(new Ui::tabRepairNew)
{
    logUserActivity();

    additionalFields = new SFieldsModel(SFieldsModel::Repair);

    ui->setupUi(this);
    tabRepairNew::guiFontChanged();

    this->setWindowTitle("Приём в ремонт");
    this->setAttribute(Qt::WA_DeleteOnClose);

#ifdef QT_DEBUG
    main_window_test_scheduler = parent->test_scheduler;
    main_window_test_scheduler2 = parent->test_scheduler2;
#endif

    initDataModels();
    initWidgets();
#ifdef QT_DEBUG
    test_scheduler->start(200);
    connect(ui->pushButtonDbgRandomFill, &QPushButton::clicked, this, &tabRepairNew::dbgRandomFillClicked);
#else
    ui->groupBoxDebugPanel->hide();
#endif
}

tabRepairNew::~tabRepairNew()
{
    delete ui;
    delete deviceVendorsModel;
    delete devicesModel;
    delete deviceClassesModel;
    delete classProblemsModel;
    delete classIncomingSetsModel;
    delete classExteriorsModel;
    delete additionalFields;
    p_instance = nullptr;   // Обязательно блять!
}

void tabRepairNew::initDataModels()
{
    deviceClassesModel = new SSqlQueryModel();
    deviceClassesModel->setQuery(QUERY_SEL_DEVICES, QSqlDatabase::database("connMain"));
    deviceVendorsModel = new SSqlQueryModel();
    devicesModel = new SSqlQueryModel();
    classProblemsModel = new QSqlQueryModel();
    classIncomingSetsModel = new QSqlQueryModel();
    classExteriorsModel = new QSqlQueryModel();
    prepayReasonsModel = new QStandardItemModel();
    QList<QStandardItem*> *prepayReason;
    QStringList prepayReasonsList = {tr("полная предоплата"), tr("за детали"), tr("за часть стоимости деталей"), tr("за часть стоимости работ"), tr("за диагностику")};
    for (int i=0; i<prepayReasonsList.size(); i++)
    {
        prepayReason = new QList<QStandardItem*>();
        *prepayReason << new QStandardItem(prepayReasonsList.at(i));
        prepayReason->value(0)->setData(i); // в UserRole+1 храним id типа предоплаты
        prepayReasonsModel->appendRow(*prepayReason);
    }
    paymentSystemsProxyModel = new SSortFilterProxyModel();
    paymentSystemsProxyModel->setSourceModel(paymentSystemsModel);
    paymentSystemsProxyModel->setFilterRegularExpression(QRegularExpression("^(?!(" + QString::number(Global::PaymentSystemIds::Balance) + ")).*$"));
    paymentSystemsProxyModel->setFilterKeyColumn(1);

    clientModel = ui->widgetClient->model();
    connect(clientModel, &SClientModel::modelUpdated, [=]{updateWidgets();});
    setQuickRepair(0);
}

void tabRepairNew::initWidgets()
{
    showLineEditPrevRepair();   // по умолчанию поля "Предыдущий ремонт" не показываем
//    ui->pushButtonCashReceipt->hide();  // в АСЦ эта кнопка расположена в groupBox'е Финансы, но выполняет ту же функцию, что и кнопка Принять на панели задач внизу; почему так — загадка; прикручу к ней такой же функционал, и если что спрячу от пользователя
    ui->checkBoxIsQuick->setVisible(permissions->createQuickRepair);
    // В АСЦ поле "Документ" появляется, если удалить какой-либо виджет при редактировании левой панели. Его назначение не ясно; comboBox не содержит ни одного эл-та. В редакторе называется ReportTemplateSelectorView. Спрячу от пользователя.

    // QComboBox::setPlaceholderText(const QString&) https://bugreports.qt.io/browse/QTBUG-90595
    ui->comboBoxProblem->setPlaceholderText(tr("неисправность"));
    ui->comboBoxIncomingSet->setPlaceholderText(tr("комплектность"));
    ui->comboBoxExterior->setPlaceholderText(tr("внешний вид"));
    ui->comboBoxDeviceClass->setPlaceholderText(tr("устройство"));
    ui->comboBoxDeviceVendor->setPlaceholderText(tr("производитель"));
    ui->comboBoxDevice->lineEdit()->setPlaceholderText(tr("модель"));
    ui->lineEditPrevRepair->setPlaceholderText(tr("Предыдущий ремонт"));
    ui->comboBoxPresetEngineer->setPlaceholderText(tr("назначить инженером"));
    ui->comboBoxPresetEngineer->setButtons("Clear");
    ui->comboBoxPresetPlace->setPlaceholderText(tr("ячейка"));
    ui->comboBoxPresetPlace->setButtons("Clear");
    ui->comboBoxOffice->setPlaceholderText(tr("офис"));
    ui->comboBoxCompany->setPlaceholderText(tr("организация"));
//    ui->comboBoxPrepayAccount->setPlaceholderText(tr("тип оплаты"));
    ui->lineEditPrevRepair->setButtons("Search, DownArrow");
    ui->lineEditPrevRepair->setReadOnly(true);
    ui->lineEditSN->setButtons("Clear");

    setDefaultStyleSheets();

    // установка моделей данных
    ui->comboBoxCompany->setModel(companiesModel);
//    if(comSettings->isAutoSetCompanyOnRepairRecept)   // в АСЦ эта настройка, похоже, не учитывается
        ui->comboBoxCompany->setCurrentIndex(SOfficeModel::current()->defaultCompanyIndex());
//    else
//        ui->comboBoxCompany->setCurrentIndex(-1);
    ui->comboBoxCompany->setVisible(companiesModel->rowCount() > 1);
    if(companiesModel->rowCount() == 1)
    {
        ui->comboBoxCompany->setVisible(false); // в АСЦ это поле остаётся видимым даже при одной компании и офисе
    }
    ui->comboBoxOffice->setModel(officesModel);
    ui->comboBoxOffice->setCurrentIndex(officesModel->rowByDatabaseID(userDbData->currentOffice));
    ui->comboBoxOffice->setVisible(officesModel->rowCount() > 1);
    if(officesModel->rowCount() == 1)
    {
        ui->comboBoxOffice->setVisible(false);
    }
    ui->comboBoxPresetEngineer->setModel(engineersModel);
    ui->comboBoxPresetEngineer->setCurrentIndex(-1);
    ui->comboBoxPresetPaymentAccount->setModel(paymentSystemsModel);
    ui->comboBoxPresetPaymentAccount->setCurrentIndex(paymentSystemsModel->rowByDatabaseID(0, "system_id"));
    ui->comboBoxPrepayAccount->setModel(paymentSystemsProxyModel);
    ui->comboBoxPrepayAccount->setCurrentIndex(paymentSystemsProxyModel->rowByDatabaseID(userDbData->defaultPaymentSystem, "system_id"));
    ui->comboBoxPresetPlace->setModel(repairBoxesModel);
    ui->comboBoxPresetPlace->setCurrentIndex(-1);
    ui->comboBoxDeviceClass->setModel(deviceClassesModel);
    ui->comboBoxDeviceClass->setCurrentIndex(-1);
    ui->comboBoxDeviceVendor->setModel(deviceVendorsModel);
    ui->comboBoxDevice->setModel(devicesModel);
    ui->widgetClient->setClientsTabDefaultCategory(SClientModel::Categories::Regulars);
    ui->comboBoxProblem->setModel(classProblemsModel);
    ui->comboBoxProblem->setCurrentIndex(-1);
    ui->comboBoxIncomingSet->setModel(classIncomingSetsModel);
    ui->comboBoxExterior->setModel(classExteriorsModel);
    ui->comboBoxPrepayReason->setModel(prepayReasonsModel);
    ui->comboBoxPrepayReason->setCurrentIndex(-1);
    ui->widgetQuickRepairBOQ->switchEditStrategy(0);

    connect(ui->lineEditSN,SIGNAL(textEdited(QString)),ui->widgetDeviceMatch,SLOT(findBySN(QString)));
    connect(ui->widgetDeviceMatch,SIGNAL(deviceSelected(int)),this,SLOT(fillDeviceCreds(int)));
    connect(ui->checkBoxIsQuick, &QCheckBox::toggled, this, &tabRepairNew::quickRepairToggled);
    connect(ui->pushButtonQuickRepairIssue1, &QCheckBox::clicked, this, &tabRepairNew::issueQuickRepair);
    connect(ui->pushButtonQuickRepairIssue2, &QCheckBox::clicked, this, &tabRepairNew::issueQuickRepair);
    connect(ui->widgetQuickRepairBOQ, &SBillOfQuantities::amountUpdated, [=](const double value){ui->doubleSpinBoxQuickRepairAmount->setValue(value);});

    // TODO: настройка минимальных сумм в параметрах программы
    if(comSettings->classicKassa)
    {
        ui->doubleSpinBoxEstPrice->setMinimum(0.01);
        ui->doubleSpinBoxPrepaySumm->setMinimum(0.01);
    }
    else
    {
        ui->doubleSpinBoxEstPrice->setMinimum(1.00);
        ui->doubleSpinBoxPrepaySumm->setMinimum(1.00);
    }

    updateWidgets();
}

void tabRepairNew::clearWidgets()
{
    m_prevRepair = 0;
    ui->comboBoxProblem->clearEditText();
    ui->comboBoxIncomingSet->clearEditText();
    ui->comboBoxExterior->clearEditText();
    ui->lineEditSN->setText("");
    ui->lineEditPrevRepair->setText("");
    ui->lineEditPrevRepairFromOldDB->setText("");
    ui->checkBoxIsQuick->setCheckState(Qt::Unchecked);
    ui->checkBoxWasInOtherWorkshop->setCheckState(Qt::Unchecked);
    ui->checkBoxIsHighPriority->setCheckState(Qt::Unchecked);
    ui->checkBoxIsNonImportantData->setCheckState(Qt::Unchecked);
    ui->checkBoxWasEarlier->setCheckState(Qt::Unchecked);
    ui->checkBoxIsWarranty->setCheckState(Qt::Unchecked);
    ui->checkBoxIsCashReceiptDocNeeded->setCheckState(Qt::Unchecked);
    ui->checkBoxIsCheckNeeded->setCheckState(Qt::Unchecked);
    ui->comboBoxPresetEngineer->setCurrentIndex(-1);
    ui->comboBoxPresetPlace->setCurrentIndex(-1);
    ui->spinBoxStickersCount->setValue(comSettings->printRepairStickers?comSettings->defaultRepairStickersQty:0);
    ui->lineEditExtNotes->setText("");
    ui->lineEditInsideComment->setText("");
    ui->checkBoxIsEstPrice->setCheckState(Qt::Unchecked);
    ui->checkBoxIsPrepay->setCheckState(Qt::Unchecked);
    ui->comboBoxPrepayReason->setCurrentIndex(-1);
    ui->doubleSpinBoxPrepaySumm->setValue(0);
    ui->comboBoxPrepayAccount->setCurrentIndex(-1);
    ui->doubleSpinBoxEstPrice->setValue(0);
    ui->comboBoxDeviceClass->setCurrentIndex(-1);
    ui->comboBoxDeviceVendor->setCurrentIndex(-1);
    ui->comboBoxDevice->setCurrentIndex(-1);

    setQuickRepair(0);
    updateWidgets();
}

void tabRepairNew::getPrepayment(double summ)
{
    bool nErr = 1;
    int repair = repairModel->id();
    cashRegister = new SCashRegisterModel();
    cashRegister->setCompanyIndex(ui->comboBoxCompany->currentIndex());
    cashRegister->setOfficeIndex(ui->comboBoxOffice->currentIndex());
    cashRegister->setSystemId(paymentSystemsProxyModel->databaseIDByRow(ui->comboBoxPrepayAccount->currentIndex(), "system_id"));
    cashRegister->setClient(clientModel->id());
    cashRegister->setOperationType(SCashRegisterModel::RecptPrepayRepair);
    cashRegister->setRepairId(repair);
    cashRegister->setAmount(summ);
    cashRegister->setReason(QString("%1 (%2)").arg(cashRegister->constructReason(repair), ui->comboBoxPrepayReason->currentText()));
    cashRegister->setSkipLogRecording(true);
    nErr &= cashRegister->commit();
    repairModel->setPrepaidOrder(cashRegister->id());
    delete cashRegister;

    // TODO: Признак предмета расчета

    if(!nErr)
        throw Global::ThrowType::QueryError;
}

void tabRepairNew::saveInternalComment()
{
    bool nErr = 1;
    comment = new SCommentModel();
    comment->setRepair(repairModel->id());
    comment->setText(ui->lineEditInsideComment->text());
    nErr &= comment->commit();
    delete comment;

    if(!nErr)
        throw Global::ThrowType::QueryError;
}

void tabRepairNew::setModelData()
{
    int preferredPaymentAccIndex;
    int user;

    user = userDbData->id;
    preferredPaymentAccIndex = ui->comboBoxPresetPaymentAccount->currentIndex();

    if(m_prevRepair)
    {
        repairModel->setEarly(m_prevRepair);
        repairModel->setIsRepeat(1);
    }
    repairModel->setClassId(deviceClassesModel->databaseIDByRow(ui->comboBoxDeviceClass->currentIndex()));
    repairModel->setVendorId(deviceVendorsModel->databaseIDByRow(ui->comboBoxDeviceVendor->currentIndex()));
    repairModel->setTitle(ui->comboBoxDeviceClass->currentText() + " " + ui->comboBoxDeviceVendor->currentText() + " " + ui->comboBoxDevice->currentText());
    repairModel->setDeviceId(deviceId());
    repairModel->setClientId(clientModel->id());
    repairModel->setSerialNumber(ui->lineEditSN->text());
    repairModel->setCompanyIndex(ui->comboBoxCompany->currentIndex());
    repairModel->setOfficeIndex(ui->comboBoxOffice->currentIndex());
    repairModel->setStartOfficeIndex(ui->comboBoxOffice->currentIndex());
    repairModel->setManager(user);
    repairModel->setCurrentManager(user);
    repairModel->setEngineer(m_isQuick?user:engineersModel->databaseIDByRow(ui->comboBoxPresetEngineer->currentIndex()));
    repairModel->setFault(ui->comboBoxProblem->currentText());
    repairModel->setComplect(ui->comboBoxIncomingSet->currentText());
    repairModel->setLook(ui->comboBoxExterior->currentText());
    repairModel->setExtNotes(ui->lineEditExtNotes->text());
    if(ui->checkBoxIsHighPriority->isChecked())
        repairModel->setExpressRepair(1);
    if(ui->checkBoxIsQuick->isChecked())
        repairModel->setQuickRepair(1);
    if(ui->checkBoxIsWarranty->isChecked())
        repairModel->setIsWarranty(1);
    if(ui->checkBoxWasEarlier->isChecked())
        repairModel->setIsRepeat(1);
    if(ui->checkBoxIsEstPrice->isChecked())
    {
        repairModel->setIsPreAgreed(1);
        repairModel->setPreAgreedAmount(ui->doubleSpinBoxEstPrice->value());
    }
    if(ui->checkBoxIsNonImportantData->isChecked())
        repairModel->setCanFormat(1);
    if(ui->checkBoxIsCheckNeeded->isChecked())
        repairModel->setPrintCheck(1);
    if(ui->checkBoxWasInOtherWorkshop->isChecked())
        repairModel->setThirsPartySc(1);
    if(ui->lineEditPrevRepairFromOldDB->text().length())
        repairModel->setExtEarly(ui->lineEditPrevRepairFromOldDB->text());
    if( preferredPaymentAccIndex >= 0)
    {
        repairModel->setPaymentSystemIndex(preferredPaymentAccIndex);
        if(paymentSystemsModel->databaseIDByRow(preferredPaymentAccIndex, "system_id") == -1)
            repairModel->setIsCardPayment(1);
    }
}

void tabRepairNew::updateWidgets()
{
    ui->widgetQuickRepairBOQ->setReadOnly(!m_isQuick);
    ui->widgetQuickRepairBOQ->setVisible(m_isQuick);
    ui->checkBoxQuickRepairConfirmPayment->setVisible(m_isQuick);
    ui->checkBoxQuickRepairPayFromBalance->setVisible(m_isQuick && clientModel->balanceEnabled());
    // checkBox "В долг" отображается только если настроен переход "Готово к выдаче"->"Выдано в долг" и включен баланс клиента
    ui->checkBoxQuickRepairIssueInCredit->setVisible(m_isQuick && clientModel->balanceEnabled() &&
                                                     comSettings->repairStatuses[Global::RepStateIds::Ready].Contains.contains(Global::RepStateIds::ReturnedInCredit));
    ui->labelQuickRepairAmount->setVisible(m_isQuick);
    ui->doubleSpinBoxQuickRepairAmount->setVisible(m_isQuick);
    ui->pushButtonQuickRepairIssue1->setVisible(m_isQuick);
    ui->pushButtonQuickRepairIssue2->setVisible(m_isQuick);
    ui->labelQuickRepairPrintBOQ->setVisible(m_isQuick);
    ui->checkBoxQuickRepairPrintBOQ->setVisible(m_isQuick);
    ui->labelQuickRepairPrintWarrantyDoc->setVisible(m_isQuick);
    ui->checkBoxQuickRepairPrintWarrantyDoc->setVisible(m_isQuick);
    ui->labelIsCheckNeeded->setText(m_checkBoxIsCheckNeededText);

    ui->pushButtonReceipt->setVisible(!m_isQuick);
    ui->pushButtonReceiptAndClose1->setVisible(!m_isQuick);
    ui->pushButtonReceiptAndClose2->setVisible(!m_isQuick);
    ui->checkBoxIsEstPrice->setVisible(!m_isQuick);
    ui->doubleSpinBoxEstPrice->setVisible(!m_isQuick);
    ui->checkBoxIsPrepay->setVisible(!m_isQuick);
    ui->comboBoxPrepayReason->setVisible(!m_isQuick);
    ui->doubleSpinBoxPrepaySumm->setVisible(!m_isQuick);
    ui->comboBoxPrepayAccount->setVisible(!m_isQuick);
    ui->comboBoxPresetEngineer->setVisible(!m_isQuick);
    ui->comboBoxPresetPlace->setVisible(!m_isQuick);

    ui->widgetQuickRepairBOQ->updateWidgets();
    updateCheckBoxQuickRepairPrintBOQ();
    ui->spinBoxStickersCount->setValue(m_isQuick?0:(comSettings->printRepairStickers?comSettings->defaultRepairStickersQty:0));
}

void tabRepairNew::updateWidgetsOnQuickRepairToggled()
{
    // снять галочку с checkBox с включенным свойством AutoExclusive смог только так:
    ui->checkBoxQuickRepairConfirmPayment->setCheckable(false);
    ui->checkBoxQuickRepairConfirmPayment->setCheckable(true);
    ui->checkBoxQuickRepairIssueInCredit->setCheckable(false);
    ui->checkBoxQuickRepairIssueInCredit->setCheckable(true);
    ui->checkBoxQuickRepairPayFromBalance->setCheckable(false);
    ui->checkBoxQuickRepairPayFromBalance->setCheckable(true);

    ui->checkBoxIsEstPrice->setChecked(false);
    ui->doubleSpinBoxEstPrice->setValue(0);
    ui->checkBoxIsPrepay->setChecked(false);
    ui->doubleSpinBoxPrepaySumm->setValue(0);
    ui->comboBoxPrepayAccount->setCurrentIndex(-1);
    ui->comboBoxPrepayReason->setCurrentIndex(-1);
    ui->comboBoxPresetEngineer->setCurrentIndex(-1);
    ui->comboBoxPresetPlace->setCurrentIndex(-1);
    ui->checkBoxQuickRepairPrintWarrantyDoc->setChecked(false);
    updateCheckBoxQuickRepairPrintBOQ();
}

/* Установка состояния флага "Акт выполненных работ"
 * Описание алгоритма см. в SClientModel::printBOQDefaultState()
*/
void tabRepairNew::updateCheckBoxQuickRepairPrintBOQ()
{
    if(!ui->checkBoxIsQuick->isChecked())
    {
        ui->checkBoxQuickRepairPrintBOQ->setChecked(false);
        return;
    }

    ui->checkBoxQuickRepairPrintBOQ->setChecked(clientModel->printBOQDefaultState());
}

void tabRepairNew::setQuickRepair(const int state)
{
    m_isQuick = state?1:0;
    m_checkBoxIsCheckNeededText = m_isQuick?tr("Распечатать кассовый чек"):tr("Чек при выдаче");
}

bool tabRepairNew::tabCloseRequest()
{
    // TODO: сделать проверку не сохранённых данных перед закрытием
    return 1;
}

QString tabRepairNew::tabTitle()
{
    return tr("Приём в ремонт");
}

tabRepairNew* tabRepairNew::getInstance(MainWindow *parent)   // singleton: вкладка приёма в ремонт может быть только одна
{
if( !p_instance )
  p_instance = new tabRepairNew(parent);
return p_instance;
}

void tabRepairNew::getDevices()
{
//    if (!DBConnectionOK)
//        return;

}

void tabRepairNew::showLineEditPrevRepair()
{
    if(ui->checkBoxWasEarlier->checkState() || ui->checkBoxIsWarranty->checkState())
    {
        ui->lineEditPrevRepair->show();
    }
    else
    {
        ui->lineEditPrevRepair->hide();
        ui->labelPrevRepairFromOldDB->hide();
        ui->lineEditPrevRepairFromOldDB->hide();
    }

}

void tabRepairNew::enablePrepayWidgets(bool state)
{
    ui->comboBoxPrepayReason->setEnabled(state);
    ui->doubleSpinBoxPrepaySumm->setEnabled(state);
    ui->comboBoxPrepayAccount->setEnabled(state);
    ui->pushButtonReceiptAndClose1->setEnabled(state);
}

void tabRepairNew::changeDeviceClass(int index)
{
    if(index == -1)
    {
        additionalFields->clear();
        return;
    }

    int additionalFieldRow = 2, additionaFieldCol = 0;
    SFieldValueModel *additionalField;
    int classId = 0;
    QString query;
    classId = deviceClassesModel->databaseIDByRow(index);


    query = QUERY_SEL_DEVICE_MAKERS(deviceClassesModel->index(index, 2).data().toString());
    deviceVendorsModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxDeviceVendor->setCurrentIndex(-1);

    // Заполнение модели выпадающего списка неисправностей
    query = QUERY_SEL_DEVICE_FAULTS(classId);
    classProblemsModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxProblem->setCurrentIndex(-1);

    // Заполнение модели выпадающего списка комплектности
    query = QUERY_SEL_DEVICE_SET(classId);
    classIncomingSetsModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxIncomingSet->setCurrentIndex(-1);

    // Заполнение модели выпадающего списка внешнего вида
    query = QUERY_SEL_DEVICE_EXTERIOR(classId);
    classExteriorsModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxExterior->setCurrentIndex(-1);

    additionalFields->init(classId);
    foreach(additionalField, additionalFields->list())
    {
        ui->gridLayoutDeviceDescription->addWidget(additionalField->widget(), additionalFieldRow, additionaFieldCol, 1, 1);
        if(additionalField->deviceMatch())
            connect(additionalField, SIGNAL(textChanged(int,QString)), ui->widgetDeviceMatch, SLOT(findByField(int,QString)));
        if (++additionaFieldCol > 5)
        {
            additionalFieldRow++;
            additionaFieldCol = 0;
        }
    }
}

void tabRepairNew::changeDeviceVendor(int index)
{
    int comboBoxDeviceIndex = ui->comboBoxDeviceClass->currentIndex();
    int deviceClassId = deviceClassesModel->databaseIDByRow(comboBoxDeviceIndex);
    int deviceVendorId = deviceVendorsModel->databaseIDByRow(index);
    QString query;

    query = QUERY_SEL_DEVICE_MODELS.arg(deviceClassId).arg(deviceVendorId);
    devicesModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxDevice->setCurrentIndex(-1);
}

void tabRepairNew::lineEditPrevRepairButtonsHandler(int button)
{
    if (button == SLineEdit::Search)
        emit createTabSelectPrevRepair(1, this);
    else if (button == SLineEdit::DownArrow)
    {
        if (!ui->lineEditPrevRepairFromOldDB->isVisible())
        {
            ui->lineEditPrevRepairFromOldDB->show();
            ui->labelPrevRepairFromOldDB->show();
        }
        else
        {
            ui->lineEditPrevRepairFromOldDB->hide();
            ui->labelPrevRepairFromOldDB->hide();
        }
    }
}

void tabRepairNew::fillDeviceCreds(int id)
{
    int deviceClass, deviceVendor, device;

    m_prevRepair = 0;
    QSqlQueryModel* queryDevice = new QSqlQueryModel();
    queryDevice->setQuery(QUERY_SEL_DEVICE(id), QSqlDatabase::database("connMain"));

    if(queryDevice->rowCount())
    {
        m_prevRepair = id;
        deviceClass = queryDevice->record(0).value("type").toInt();
        deviceVendor = queryDevice->record(0).value("maker").toInt();
        device = queryDevice->record(0).value("model").toInt();

        ui->widgetClient->fillClientCreds(queryDevice->record(0).value("client").toInt());
//        if (ui->comboBoxDeviceClass->currentIndex() < 0)        // только если пользователь не выбрал тип уст-ва
        {
            // TODO: сделать запросы к БД асинхронными. Готовые классы https://github.com/micjabbour/MSqlQuery
            // на данный момент по локальной сети всё работает быстро, поэтому не буду заморачиваться

            ui->comboBoxDeviceClass->setCurrentIndex(deviceClassesModel->rowByDatabaseID(deviceClass));
            ui->comboBoxDeviceVendor->setCurrentIndex(deviceVendorsModel->rowByDatabaseID(deviceVendor));
            ui->comboBoxDevice->setCurrentIndex(devicesModel->rowByDatabaseID(device));
            ui->lineEditSN->setText(queryDevice->record(0).value("serial_number").toString());
        }
        ui->lineEditPrevRepair->setText(QString::number(m_prevRepair));  // устанавливаем номер предыдущего ремонта в соотв. поле
        ui->checkBoxWasEarlier->setCheckState(Qt::Checked);     // ставим галочку "Ранее было в ремонте" чтобы поле с номером ремонта отобразилось

        ui->comboBoxProblem->setFocus();    // устанавливаем фокус на полее ввода неисправности

    }
    delete queryDevice;
}

void tabRepairNew::lineEditSNClearHandler(int)
{
    if(ui->lineEditSN->text().length())
        ui->lineEditSN->clear();
    else
        ui->lineEditSN->setText("НЕТ S/N");
    ui->widgetDeviceMatch->findBySN("");
}

void tabRepairNew:: setDefaultStyleSheets()
{
    ui->comboBoxDeviceClass->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxDeviceVendor->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxDevice->setStyleSheet(commonComboBoxStyleSheet);
    ui->lineEditSN->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxProblem->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxIncomingSet->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxExterior->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetEngineer->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetPlace->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxOffice->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxCompany->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetPaymentAccount->setStyleSheet(commonComboBoxStyleSheet);
    ui->doubleSpinBoxEstPrice->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxPrepayReason->setStyleSheet(commonComboBoxStyleSheet);
    ui->doubleSpinBoxPrepaySumm->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxPrepayAccount->setStyleSheet(commonComboBoxStyleSheet);
    ui->lineEditPrevRepair->setStyleSheet(commonLineEditStyleSheet);
}

/* Проверка введённых данных
 * Возвращает 0 если данные корректны
*/
bool tabRepairNew::checkInput()
{
    int error = 0;
    setDefaultStyleSheets();

    if ( ui->comboBoxDeviceClass->currentIndex() < 0 )        // если не выбран тип уст-ва
    {
        ui->comboBoxDeviceClass->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 1;
    }
    if ( ui->comboBoxDeviceVendor->currentIndex() < 0 )        // если не выбран производителя
    {
        ui->comboBoxDeviceVendor->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 2;
    }
    if ( ui->comboBoxDevice->currentIndex() < 0 && ui->comboBoxDevice->currentText() == "" )        // если не выбрана или не написана модель
    {
        ui->comboBoxDevice->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 3;
    }
    if (ui->lineEditSN->text() == "" && comSettings->isSerialNumberRequired)   // если не записан серийный номер, а он обязателен
    {
        ui->lineEditSN->setStyleSheet(commonLineEditStyleSheetRed);
        error = 4;
    }

    error = qMax(error, ui->widgetClient->checkInput());

    if (ui->comboBoxProblem->text() == "")        // если не указана(ы) неисправность(и)
    {
        ui->comboBoxProblem->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 12;
    }
    if (ui->comboBoxIncomingSet->text() == "")        // если не указана комплектность
    {
        ui->comboBoxIncomingSet->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 13;
    }
    if (ui->comboBoxExterior->text() == "")        // если не указано состояние (внешинй вид)
    {
        ui->comboBoxExterior->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 14;
    }
    if (ui->comboBoxPresetEngineer->currentIndex() < 0 && comSettings->isEngineerRequiredOnDeviceRecept)        // если не выбран инженер, а он обязателен
    {
        ui->comboBoxPresetEngineer->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 15;
    }
    if(!additionalFields->validate())
    {
        error = 16;
    }
    if ( ui->checkBoxIsEstPrice->isChecked() )   // если установлен флаг "Клиент ознакомлен с возможной стоимостью"
        if (ui->doubleSpinBoxEstPrice->value() == ui->doubleSpinBoxEstPrice->minimum())   // соответствует ли норме введённая сумма
        {
            ui->doubleSpinBoxEstPrice->setStyleSheet(commonSpinBoxStyleSheetRed);
            error = 17;
        }
    if ( ui->checkBoxIsPrepay->isChecked() )       // если установлен флаг "Клиент вносит предоплату"
    {
        if (ui->comboBoxPrepayReason->currentIndex() < 0)       // обязательно должно быть указано основание
        {
            ui->comboBoxPrepayReason->setStyleSheet(commonComboBoxStyleSheetRed);
            error = 18;
        }
        if (ui->comboBoxPrepayAccount->currentIndex() < 0)      // опязательно должен быть выбран счёт
        {
            ui->comboBoxPrepayAccount->setStyleSheet(commonComboBoxStyleSheetRed);
            error = 19;
        }
        if ( ui->doubleSpinBoxPrepaySumm->value() == 0 )   // соответствует ли норме введённая сумма
        {
            ui->doubleSpinBoxPrepaySumm->setStyleSheet(commonLineEditStyleSheetRed);
            error = 20;
        }
    }
    if ( ui->checkBoxWasEarlier->isChecked() ) // если установлен флаг "Ранее было в ремонте", то обязательно должен быть указан номер ремонта; однако, это не касается гарантийного ремонта
        if ( ui->lineEditPrevRepair->text() == "" )
        {
            if ( ui->lineEditPrevRepairFromOldDB->text() == "" )  // также допускается не выбирать предыдущий ремонт, если указан номер ремонта в старой системе CRM
            {
                ui->lineEditPrevRepair->setStyleSheet(commonLineEditStyleSheetRed);
                error = 19;
            }
        }

    if ( ui->comboBoxCompany->currentIndex() < 0)
    {
        ui->comboBoxCompany->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 20;
    }
    if ( ui->comboBoxOffice->currentIndex() < 0)
    {
        ui->comboBoxOffice->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 21;
    }

    if (error)
    {
        qDebug() << "Ошибка создания карты ремонта: не все обязательные поля заполнены (error " << error << ")";
        return true;
    }

    return false;
}

/* Проверка введённых данных в режиме Быстрый ремонт
 * Возвращает 0 если данные корректны
*/
bool tabRepairNew::quickRepairCheckInput()
{
    QString errMsg;
    int error = 0;

    if(checkInput())
        return true;

    if(!(ui->checkBoxQuickRepairConfirmPayment->isChecked() || ui->checkBoxQuickRepairIssueInCredit->isChecked() || ui->checkBoxQuickRepairPayFromBalance->isChecked()))
    {
        error = 1;
        errMsg = tr("Подтвердите выдачу");
    }

    if(ui->widgetQuickRepairBOQ->isEmpty())
    {
        error = 2;
        errMsg = tr("Список работ пуст");
    }

    if (error)
    {
        qDebug() << "Ошибка создания быстрого ремонта ремонта (error " << error << ": " << errMsg << ")";
        shortlivedNotification *newPopup = new shortlivedNotification(this, tr("Ошибка"), errMsg, QColor("#FFC7AD"), QColor("#FFA477"));
        return true;
    }

    return false;
}

/*  Поиск имеющейся модели устройства в списке или создание новой.
 *  Возвращает id модели.
 *  Пользователь мог выбрать похожую модель из списка с целью дальнейшего редактирования,
 *  но в случае изменения текста lineEdit'а в начале или середине механизм дополнения не сработает.
 *  Чтобы не создался дубль модели в данном методе сначала производится поиск и если модель не найдена
 *  создаётся новая.
*/
int tabRepairNew::deviceId()
{
    int deviceIndex = devicesModel->findIndex(ui->comboBoxDevice->currentText());
    if ( deviceIndex < 0 )
        return createDeviceModel();

    return devicesModel->databaseIDByRow(deviceIndex);
}

int tabRepairNew::createDeviceModel()
{
    bool nErr = 1;
    int device;

    SDevMdlModel *devMdl = new SDevMdlModel(this);
    devMdl->setName(ui->comboBoxDevice->currentText());
    devMdl->setDevice(deviceClassesModel->databaseIDByRow(ui->comboBoxDeviceClass->currentIndex()));
    devMdl->setMaker(deviceVendorsModel->databaseIDByRow(ui->comboBoxDeviceVendor->currentIndex()));
    nErr &= devMdl->commit();
    device = devMdl->id();
    delete devMdl;

    if(!nErr)
        throw Global::ThrowType::QueryError;
    return device;
}

/*  Запись данных в БД
 *  Возвращает 0  в случае ошибки
*/
bool tabRepairNew::createRepair()
{
    if(checkInput())
        return false;

    bool nErr = 1;
    int repair;
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));
    m_stickersCount = ui->spinBoxStickersCount->value();
    repairModel = new SRepairModel(this);
    ui->widgetQuickRepairBOQ->linkWithRepairModel(repairModel);

    setDefaultStyleSheets();

    QUERY_LOG_START(metaObject()->className());

    try
    {
        ui->widgetClient->commit();
        clientModel = ui->widgetClient->model();

        QUERY_EXEC(query,nErr)(QUERY_BEGIN);

        setModelData();
        repairModel->commit();
        clientModel->updateRepairs(1);
        repair = repairModel->id();

        // запись значений доп. полей
        additionalFields->setObjectId(repair);
        additionalFields->commit();

        if (!ui->lineEditInsideComment->text().isEmpty())
            saveInternalComment();

        if(ui->comboBoxPresetPlace->currentIndex() >= 0)
        {
            repairModel->setBoxIndex(ui->comboBoxPresetPlace->currentIndex());
            repairModel->commit();
        }

        if (ui->checkBoxIsPrepay->isChecked())
        {
            double prepaySumm = sysLocale.toDouble(ui->doubleSpinBoxPrepaySumm->text());
            repairModel->addPrepay(prepaySumm, ui->comboBoxPrepayReason->currentText());
            getPrepayment(ui->doubleSpinBoxPrepaySumm->value());
            repairModel->commit();
        }

        if(m_isQuick)
        {
            SDialogIssueRepair *dialogIssue;

            m_quickRepairIssueList.clear();
            repairModel->BOQModel()->repair_saveTables(SSaleTableModel::RepairOpType::Link);

            m_quickRepairIssueList.append(repairModel);
            dialogIssue = new SDialogIssueRepair(m_quickRepairIssueList, this);   // Диалоговое окно не отображается
            dialogIssue->setPayFromBalance(ui->checkBoxQuickRepairPayFromBalance->isChecked());
            dialogIssue->setIssuingInCredit(ui->checkBoxQuickRepairIssueInCredit->isChecked());
            dialogIssue->setPaymentSystemId(paymentSystemsModel->databaseIDByRow(ui->comboBoxPresetPaymentAccount->currentIndex(), "system_id"));
            dialogIssue->issueRepairs();
            dialogIssue->deleteLater();
        }

#ifdef QT_DEBUG
//        throw Global::ThrowType::Debug; // это для отладки (чтобы сессия всегда завершалась ROLLBACK'OM)
#endif
        QUERY_COMMIT_ROLLBACK(query, nErr);
    }
    catch (Global::ThrowType type)
    {
        nErr = 0;
        additionalFields->resetIds();
        if(type == Global::ThrowType::Debug)
        {
            QString err = "DEBUG ROLLBACK";
            QUERY_ROLLBACK_MSG(query, err);
//            nErr = 1; // это чтобы проверить работу дальше
        }
        else if (type == Global::ThrowType::QueryError)
        {
            QUERY_COMMIT_ROLLBACK_MSG(query, nErr);
        }
        else
            QUERY_COMMIT_ROLLBACK(query, nErr);
    }
    catch(...)
    {
        nErr = 0;
    }

    QUERY_LOG_STOP;

    if (nErr)   // если все запросы выполнены без ошибок
    {
        print(repair);
        clearWidgets();
        tabRepairs::refreshIfTabExists();
        // TODO: обработка списка совпадений устройств (табл. tasks, type == 5)
    }

    ui->widgetQuickRepairBOQ->linkWithRepairModel(nullptr);

    delete query;
    return nErr;
}

void tabRepairNew::createRepairClose()
{
    if(!m_closePending)    // TODO: программа падает при двойном клике по кнопке.
    {
        m_closePending = 1;
        if (createRepair())
            this->deleteLater();
        else
            m_closePending = 0;
    }
}

void tabRepairNew::issueQuickRepair()
{
    if(quickRepairCheckInput())
        return;

    if(ui->checkBoxQuickRepairPayFromBalance->isChecked() && !ui->checkBoxQuickRepairIssueInCredit->isChecked())
        if(!clientModel->balanceEnough(-ui->doubleSpinBoxQuickRepairAmount->value()))
            return;

    createRepairClose();
}

void tabRepairNew::preferredPaymentSystemChanged(int)
{
    // при установке модели данных автоматически устанавливается индекс 0 и в модель ремонта устанавливается предпочитаемый тип оплаты, что есть неправильно
}

void tabRepairNew::guiFontChanged()
{
    QFont font;
//    font.setFamily(userLocalData->FontFamily.value);
    font.setPixelSize(userDbData->fontSize);

    ui->comboBoxDeviceClass->setFont(font);
    ui->comboBoxDeviceVendor->setFont(font);
    ui->comboBoxDevice->setFont(font);
    ui->lineEditSN->setFont(font);
    ui->comboBoxProblem->setFont(font);
    ui->comboBoxIncomingSet->setFont(font);
    ui->comboBoxExterior->setFont(font);
    ui->comboBoxPresetEngineer->setFont(font);
    ui->comboBoxPresetPlace->setFont(font);
    ui->comboBoxOffice->setFont(font);
    ui->comboBoxCompany->setFont(font);
    ui->comboBoxPresetPaymentAccount->setFont(font);
    ui->doubleSpinBoxEstPrice->setFont(font);
    ui->comboBoxPrepayReason->setFont(font);
    ui->doubleSpinBoxPrepaySumm->setFont(font);
    ui->comboBoxPrepayAccount->setFont(font);
    ui->lineEditPrevRepair->setFont(font);
}

void tabRepairNew::quickRepairToggled(const int state)
{
    setQuickRepair(state);
    updateWidgetsOnQuickRepairToggled();
    updateWidgets();

#ifdef QT_DEBUG
    if (state)
    {
        ui->widgetQuickRepairBOQ->dbgAddWork();
        ui->checkBoxQuickRepairConfirmPayment->setChecked(true);
    }
    else
    {
        ui->widgetQuickRepairBOQ->clearTable();
    }
#endif

}

void tabRepairNew::print(int repair)
{
    QMap<QString, QVariant> report_vars;

    if(m_isQuick)   // для быстрого ремонта печать акта выполненных работ
    {
        if(ui->checkBoxQuickRepairPrintBOQ->isChecked())
        {
            tabPrintDialog::printRepairWorksReports(m_quickRepairIssueList, true);
        }
        //        if(ui->checkBoxQuickRepairPrintWarrantyDoc->isChecked())
//            tabPrintDialog::printRepairWorksReports(list, false);
    }
    else  // для обычного ремонта печать квитанции
    {
        if(comSettings->printRepairReceptDoc)
        {
            report_vars.insert("type", Global::Reports::new_rep);
            report_vars.insert("repair_id", repair);
            emit generatePrintout(report_vars);
            report_vars.clear();
        }
    }

    // печать стикеров для любого типа ремонта
    if(m_stickersCount > 0)
    {
        report_vars.insert("type", Global::Reports::rep_label);
        report_vars.insert("repair_id", repair);
        report_vars.insert("copies", m_stickersCount);
        emit generatePrintout(report_vars);
        report_vars.clear();
    }

    // печать ПКО
    if(ui->checkBoxIsCashReceiptDocNeeded->isChecked())
    {
//        report_vars.insert("type", Global::Reports::pko);
//        report_vars.insert("order_id", repair);
//        emit generatePrintout(report_vars);
//        report_vars.clear();
    }
}

/* Автозаполнение данных клиента
 * Этот метод вызывается при двойном клике по строке на вкладке "Выбрать клиента"
*/
void tabRepairNew::fillClientCreds(const int id)
{
    ui->widgetClient->fillClientCreds(id);
}

#ifdef QT_DEBUG
void tabRepairNew::randomFill()
{
    int i;
    if (test_scheduler_counter == 0)   // клиент
//    if (1)
    {
        ui->widgetClient->randomFill();
//        test_scheduler_counter++;
//        test_scheduler->start(400);    //  (пере-)запускаем таймер
//        if(1)
    }
    else if (test_scheduler_counter == 1)
    {
        i = deviceClassesModel->rowCount();
        ui->comboBoxDeviceClass->setCurrentIndex(QRandomGenerator::global()->bounded(i));
//        ui->comboBoxDeviceClass->setCurrentIndex(2);
    }
    else if (test_scheduler_counter == 2)
    {
        i = deviceVendorsModel->rowCount();
        ui->comboBoxDeviceVendor->setCurrentIndex(QRandomGenerator::global()->bounded(i));
    }
    else if (test_scheduler_counter == 3)   // модель
    {
        i = devicesModel->rowCount();
        if (QRandomGenerator::global()->bounded(100) > 50 && i)  // 50/50 или выбираем из уже имеющихся моделей или случайное число (проверка i нужна, т. к. список может быть вообще пуст)
        {
            ui->comboBoxDevice->setCurrentIndex(QRandomGenerator::global()->bounded(i));
        }
        else
        {
            ui->comboBoxDevice->setCurrentText(QString::number(QRandomGenerator::global()->bounded(2147483647)));
        }
    }
    else if (test_scheduler_counter == 4)   // серийный номер
    {
        ui->lineEditSN->setText(QString::number(QRandomGenerator::global()->bounded(2147483647)));
    }
    else if (test_scheduler_counter == 5)   // неисправность
    {
        ui->comboBoxProblem->addRandomItem();
    }
    else if (test_scheduler_counter == 6)   // комплектность
    {
        ui->comboBoxIncomingSet->addRandomItem();
    }
    else if (test_scheduler_counter == 7)   // внешний вид
    {
        ui->comboBoxExterior->addRandomItem();
    }
    else if (test_scheduler_counter == 8)
    {
        additionalFields->randomFill();
    }
    else if (test_scheduler_counter == 9)   // инженер
    {
        if (QRandomGenerator::global()->bounded(100) > 50)  // 50/50
        {
            i = engineersModel->rowCount();
            ui->comboBoxPresetEngineer->setCurrentIndex(QRandomGenerator::global()->bounded(i));
        }
    }
    else if (test_scheduler_counter == 10)
    {
        // в случайном порядке включаем разные флаги
        i = QRandomGenerator::global()->bounded(16384);
        ui->checkBoxIsQuick->setChecked((i>>0)&0x01 && ui->checkBoxIsQuick->isEnabled());
        ui->checkBoxWasInOtherWorkshop->setChecked((i>>1)&0x01);
        ui->checkBoxIsHighPriority->setChecked((i>>2)&0x01);
        ui->checkBoxIsNonImportantData->setChecked((i>>3)&0x01);
        ui->checkBoxWasEarlier->setChecked((i>>4)&0x01);
        ui->checkBoxIsWarranty->setChecked((i>>5)&0x01);
        ui->checkBoxIsEstPrice->setChecked((i>>6)&0x01 && !ui->checkBoxIsQuick->isChecked());
        ui->checkBoxIsPrepay->setChecked((i>>7)&0x01 && !ui->checkBoxIsQuick->isChecked());
        ui->checkBoxIsCashReceiptDocNeeded->setChecked((i>>8)&0x01 && ui->checkBoxIsCashReceiptDocNeeded->isEnabled());
        ui->checkBoxIsCheckNeeded->setChecked((i>>9)&0x01 && ui->checkBoxIsCheckNeeded->isEnabled());
        if (ui->checkBoxWasEarlier->isChecked())
        {
            if (QRandomGenerator::global()->bounded(100) > 50)  // 50/50 или берём ремонт из базы или записываем значение "из другой БД"
            { // в данном случае это просто чтобы проверка обязательных полей проходила успешно; вызова функции заполнения полей значениями предыдущего ремонта не будет (пока)
                QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));
                query->exec(QString("SELECT `id` FROM `workshop` WHERE `state` IN (8,12,16) AND `id` < %1 ORDER BY `id` DESC LIMIT 1 ").arg(QRandomGenerator::global()->bounded(24900)));
                query->first();
                if(query->isValid())
                    ui->lineEditPrevRepair->setText(query->value(0).toString());
                else
                    ui->checkBoxWasEarlier->setChecked(0);  // это если вдруг не выбран номер
            }
            else
            {
                ui->lineEditPrevRepairFromOldDB->show();
                ui->lineEditPrevRepairFromOldDB->setText(QString::number(QRandomGenerator::global()->bounded(1000000)));
            }
        }
        if (ui->checkBoxIsEstPrice->isChecked())
        {
            ui->doubleSpinBoxEstPrice->setValue(QRandomGenerator::global()->bounded(100)*50);

        }
        if (ui->checkBoxIsPrepay->isChecked())
        {
            i = prepayReasonsModel->rowCount();
            ui->comboBoxPrepayReason->setCurrentIndex(QRandomGenerator::global()->bounded(i));
            ui->doubleSpinBoxPrepaySumm->setValue(QRandomGenerator::global()->bounded(100)*50);
            i = paymentSystemsModel->rowCount();
            ui->comboBoxPrepayAccount->setCurrentIndex(QRandomGenerator::global()->bounded(i));
        }
    }
    else if (test_scheduler_counter == 11)
    {
//        createRepair();
//        createRepairClose();
//        main_window_test_scheduler2->start(500);    // запускаем таймер закрытия вкладки приёма в ремонт
    }
    else if (test_scheduler_counter == 12)
    {}
    else if (test_scheduler_counter == 13)
    {}
    if (test_scheduler_counter < 11)
    {
        test_scheduler_counter++;
        test_scheduler->start(400);    //  (пере-)запускаем таймер
    }
//    qDebug() << "test_scheduler_handler(), test_scheduler_counter = " << test_scheduler_counter++;

}

void tabRepairNew::test_scheduler_handler()  //
{
    randomFill();
}

void tabRepairNew::test_scheduler2_handler()  // обработик таймера закрытия вкладки
{
}

void tabRepairNew::dbgRandomFillClicked()
{
    test_scheduler_counter = 0;
    ui->widgetClient->clearClientCreds();
    clearWidgets();
    randomFill();
}
#endif
