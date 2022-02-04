#include "tabrepairnew.h"
#include "ui_tabrepairnew.h"
#include "com_sql_queries.h"

tabRepairNew* tabRepairNew::p_instance = nullptr;

groupBoxEventFilter::groupBoxEventFilter(QObject *parent) :
    QObject(parent)
{
}

bool groupBoxEventFilter::eventFilter(QObject *watched, QEvent *event)
{
//    qDebug() << watched->objectName() << ": viewEventFilter: " << event;

    // когда указатель находится на заголовке (по всей ширине groupBox'а), устанавливаем курсор в виде руки с указательным пальцем
    // TODO: проверить сколько эта метода жрёт ресурсов
    QGroupBox *groupBox = static_cast<QGroupBox*>(watched);
    if (event->type() == QEvent::HoverMove)
    {
        QPoint point = static_cast<QHoverEvent*>(event)->position().toPoint();
        if (point.x() > 0 && point.x() < groupBox->width() && point.y() >0 && point.y() < 20)
            groupBox->setCursor(Qt::PointingHandCursor);
        else
            groupBox->unsetCursor();
    }

    // Нажатие левой кнопкой мыши на заголовке groupBox'а скрывает/показывает таблицу (при скрытии groupBox сжимается по вертикали)
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseButtonPress = static_cast<QMouseEvent*>(event);

        if (mouseButtonPress->button() == Qt::LeftButton)
        {
            if (mouseButtonPress->position().toPoint().x() > 0 && mouseButtonPress->position().toPoint().x() < groupBox->width() && mouseButtonPress->position().toPoint().y() >0 && mouseButtonPress->position().toPoint().y() < 20)
            {
                QTableView *table = groupBox->findChild<QTableView*>();
                table->setVisible(!table->isVisible());
            }
        }
    }
    return false;
}

tabRepairNew::tabRepairNew(MainWindow *parent) :
    QWidget(parent),
    ui(new Ui::tabRepairNew)
{
    QString query;
    // Обычные комбобоксы, которые нельзя редактировать, отображаются как неактивные элементы. Назначаем им стиль отображения как у редактируемых.

    ui->setupUi(this);
    this->setWindowTitle("Приём в ремонт");
    this->setAttribute(Qt::WA_DeleteOnClose);

    companiesModel      = parent->companiesModel;
    officesModel        = parent->officesModel;
    userData            = parent->userData;
    permissions         = parent->permissions;
    comSettings         = parent->comSettings;
    managersModel       = parent->managersModel;
    engineersModel      = parent->engineersModel;
    repairBoxesModel    = parent->repairBoxesModel;
    paymentSystemsModel = parent->paymentSystemsModel;

    groupBoxEventFilter *groupBoxEventFilterObj = new groupBoxEventFilter(this);
    ui->groupBoxClientCoincidence->installEventFilter(groupBoxEventFilterObj);
    ui->groupBoxDeviceCoincidence->installEventFilter(groupBoxEventFilterObj);
    ui->groupBoxDeviceCoincidence->hide();  // по умолчанию группу "Совпадение уст-ва" не показываем
    ui->groupBoxClientCoincidence->hide();  // по умолчанию группу "Совпадение клиента" не показываем
    ui->labelPrevRepairFromOldDB->hide();   // по умолчанию поля "Предыдущий ремонт" не показываем
    ui->lineEditPrevRepairFromOldDB->hide();
    ui->lineEditPrevRepair->hide();

    ui->lineEditEstPrice->setDisabled(true);
    ui->lineEditPrepaySumm->setDisabled(true);
    ui->comboBoxPrepayAccount->setDisabled(true);
    ui->pushButtonCashReceipt->setDisabled(true);
    ui->comboBoxProblem->lineEdit()->setPlaceholderText("неисправность");
    ui->comboBoxIncomingSet->lineEdit()->setPlaceholderText("комплектность");
    ui->comboBoxExterior->lineEdit()->setPlaceholderText("внешний вид");
    ui->comboBoxClientAdType->lineEdit()->setPlaceholderText("источник обращения");
    ui->comboBoxDevice->setPlaceholderText("устройство");
    ui->comboBoxDeviceMaker->setPlaceholderText("производитель");
    ui->comboBoxDeviceModel->lineEdit()->setPlaceholderText("модель");
    ui->comboBoxPresetEngineer->setPlaceholderText("назначить инженером");
    ui->comboBoxPresetBox->setPlaceholderText("ячейка");
    ui->comboBoxOffice->setPlaceholderText("офис");
    ui->comboBoxCompany->setPlaceholderText("организация");
    ui->comboBoxPresetPaymentAccount->setPlaceholderText("тип оплаты");
    ui->comboBoxPrepayAccount->setPlaceholderText("тип оплаты");
    ui->lineEditPrevRepair->setButtons("Search, DownArrow");
    ui->lineEditPrevRepair->setReadOnly(true);
    ui->lineEditSN->setButtons("Clear");

    setDefaultStyleSheets();

    if(QLineEdit *le = ui->comboBoxPresetEngineer->lineEdit())    // Рисуем кнопку очистки в комбобоксе (это работает)
    {
        le->setClearButtonEnabled(true);
    }

    ui->comboBoxCompany->setModel(companiesModel);
    ui->comboBoxOffice->setModel(officesModel);
    int i = 0;
    while (userData->value("current_office").toInt() != officesModel->record(i++).value("id").toInt());

    ui->comboBoxOffice->setCurrentIndex(i-1);
    ui->comboBoxPresetEngineer->setModel(engineersModel);
    ui->comboBoxPresetEngineer->setCurrentIndex(-1);
    ui->comboBoxPresetPaymentAccount->setModel(paymentSystemsModel);
    ui->comboBoxPresetPaymentAccount->setCurrentIndex(-1);
    ui->comboBoxPrepayAccount->setModel(paymentSystemsModel);
    ui->comboBoxPrepayAccount->setCurrentIndex(-1);
    ui->comboBoxPresetBox->setModel(repairBoxesModel);
    ui->comboBoxPresetBox->setCurrentIndex(-1);
    ui->spinBoxStickersCount->setValue(comSettings->value("rep_stickers_copy").toInt());

    comboboxDevicesModel = new QSqlQueryModel();
    ui->comboBoxDevice->setModel(comboboxDevicesModel);
    comboboxDeviceMakersModel = new QSqlQueryModel();
    ui->comboBoxDeviceMaker->setModel(comboboxDeviceMakersModel);
    comboboxDeviceModelsModel = new QSqlQueryModel();
    ui->comboBoxDeviceModel->setModel(comboboxDeviceModelsModel);
    comboboxProblemModel = new QSqlQueryModel();
    ui->comboBoxProblem->setModel(comboboxProblemModel);
    ui->comboBoxProblem->setCurrentIndex(-1);
    comboBoxIncomingSetModel = new QSqlQueryModel();
    ui->comboBoxIncomingSet->setModel(comboBoxIncomingSetModel);
    comboBoxExteriorModel = new QSqlQueryModel();
    ui->comboBoxExterior->setModel(comboBoxExteriorModel);

    clientAdTypesList = new QSqlQueryModel(this);
    clientAdTypesList->setQuery(QUERY_SEL_CLIENT_AD_TYPES, QSqlDatabase::database("connMain"));
    ui->comboBoxClientAdType->setModel(clientAdTypesList);
    ui->comboBoxClientAdType->setModelColumn(0);
    ui->comboBoxClientAdType->setCurrentIndex(-1);

    clientPhoneTypesList = new QStandardItemModel(this);
    clientPhoneTypesSelector[0] << new QStandardItem("мобильный") << new QStandardItem("1") << new QStandardItem(comSettings->value("phone_mask1").toString()); // в ASC формат задаётся нулями, но в поиске совпадающих клиентов  это предусмотрено
    clientPhoneTypesList->appendRow( clientPhoneTypesSelector[0] );
    clientPhoneTypesSelector[1] << new QStandardItem("городской") << new QStandardItem("2") << new QStandardItem(comSettings->value("phone_mask2").toString());
    clientPhoneTypesList->appendRow( clientPhoneTypesSelector[1] );
    ui->comboBoxClientPhone1Type->setModel(clientPhoneTypesList);
    ui->comboBoxClientPhone1Type->setModelColumn(0);
    ui->comboBoxClientPhone1Type->setCurrentIndex(clientPhoneTypesList->index(0, 0).row());    // устанавливаем первый элемент выпадающего списка
    ui->comboBoxClientPhone2Type->setModel(clientPhoneTypesList);
    ui->comboBoxClientPhone2Type->setModelColumn(0);
    ui->comboBoxClientPhone2Type->setCurrentIndex(clientPhoneTypesList->index(0, 0).row());    // устанавливаем первый элемент выпадающего списка

    clientsMatchTable = new QSqlQueryModel();       // таблица совпадения клиента (по номеру тел. или по фамилии)
    devicesMatchTable = new QSqlQueryModel();       // таблица совпадения устройства (по серийному номеру)

    comboboxDevicesModel->setQuery(QUERY_SEL_DEVICES, QSqlDatabase::database("connMain"));
    ui->comboBoxDevice->setCurrentIndex(-1);

}

tabRepairNew::~tabRepairNew()
{
    delete comboboxDeviceMakersModel;
    delete comboboxDeviceModelsModel;
    delete comboboxDevicesModel;
    for(int i=additionalFieldsWidgets.size()-1;i>=0;i--)   // в случае ошибочного выбора категории уст-ва, нужно удалить ранее добавленные виджеты доп. полей
    {
        delete additionalFieldsWidgets[i];
        additionalFieldsWidgets.removeAt(i);
    }
    delete ui;
    p_instance = nullptr;   // Обязательно блять!
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

void tabRepairNew::changeClientType()
{

    if(ui->checkBoxClientType->checkState())
    {
        ui->groupBoxClient->setTitle("Клиент (юридическое лицо)");
        ui->lineEditClientFirstName->setPlaceholderText("Название организации");
        ui->lineEditClientLastName->hide();    // скрываем поле "Фамилия"
        ui->lineEditClientPatronymic->hide();    // скрываем поле "Отчество"
        ui->gridLayoutClient->addWidget(ui->lineEditClientFirstName, 1, 0, 1, 6); // заменяем поле "Фамилия" самим собой, но растягиваем его по ширине на 6 столбцов (занимаем столбцы полей "Имя" и "Отчество")
    }
    else
    {
        ui->groupBoxClient->setTitle("Клиент (частное лицо)");
        ui->gridLayoutClient->addWidget(ui->lineEditClientLastName, 1, 0, 1, 2);
        ui->gridLayoutClient->addWidget(ui->lineEditClientFirstName, 1, 2, 1, 2); // заменяем поле "Фамилия" самим собой, но сжимаем его по ширине до двух столбцов
        ui->gridLayoutClient->addWidget(ui->lineEditClientPatronymic, 1, 4, 1, 2);
        ui->lineEditClientFirstName->setPlaceholderText("Фамилия");
        ui->lineEditClientLastName->show();    // показываем поле "Имя"
        ui->lineEditClientPatronymic->show();    // показываем поле "Отчество"
    }
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
    }

}

void tabRepairNew::changeDeviceType()
{

    int comboBoxDeviceIndex = ui->comboBoxDevice->currentIndex();
    int additionalFieldRow = 2, additionaFieldCol = 0;
    int additionalFieldType = 0;
    QSizePolicy *additionalFieldsSizePolicy = new QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QString query;

//    qDebug() << "tabRepairNew::changeDeviceType: additionalFieldsWidgets.size() =" << additionalFieldsWidgets.size() << "(before)";
    for(int i=additionalFieldsWidgets.size()-1;i>=0;i--)   // в случае ошибочного выбора категории уст-ва, нужно удалить ранее добавленные виджеты доп. полей
    {
//        qDebug() << "Removing widget; fieldId = " << additionalFieldsWidgets[i]->property("fieldId").toInt() << "; fieldType = " << additionalFieldsWidgets[i]->property("fieldType").toInt();
        delete additionalFieldsWidgets[i];
        additionalFieldsWidgets.removeAt(i);
    }
//    qDebug() << "tabRepairNew::changeDeviceType: additionalFieldsWidgets.size() =" << additionalFieldsWidgets.size() << "(after)";

    query = QUERY_SEL_DEVICE_MAKERS(comboboxDevicesModel->index(comboBoxDeviceIndex, 2).data().toString());
    comboboxDeviceMakersModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxDeviceMaker->setCurrentIndex(-1);

    // Заполнение модели выпадающего списка неисправностей
    query = QUERY_SEL_DEVICE_FAULTS(comboboxDevicesModel->index(comboBoxDeviceIndex, 1).data().toInt());
    comboboxProblemModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxProblem->setCurrentIndex(-1);


    // Заполнение модели выпадающего списка комплектности
    query = QUERY_SEL_DEVICE_SET(comboboxDevicesModel->index(comboBoxDeviceIndex, 1).data().toInt());
    comboBoxIncomingSetModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxIncomingSet->setCurrentIndex(-1);

    // Заполнение модели выпадающего списка внешнего вида
    query = QUERY_SEL_DEVICE_EXTERIOR(comboboxDevicesModel->index(comboBoxDeviceIndex, 1).data().toInt());
    comboBoxExteriorModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxExterior->setCurrentIndex(-1);

    // создание доп. полей для выбранной категории уст-ва
    QSqlQuery* additionalFieldsList = new QSqlQuery(QSqlDatabase::database("connMain"));

    query = QUERY_SEL_DEVICE_ADD_FIELDS(comboboxDevicesModel->index(comboBoxDeviceIndex, 1).data().toInt());
    additionalFieldsList->exec(query);

    while(additionalFieldsList->next())
    {
        additionalFieldType = additionalFieldsList->value(2).toInt();
        QWidget *additionalFieldWidget;
        if (additionalFieldType == 1)
        {
            QLineEdit *additionalFieldLineEdit = new QLineEdit(this);
            additionalFieldWidget = additionalFieldLineEdit;
            additionalFieldLineEdit->setPlaceholderText(additionalFieldsList->value(0).toString());
        } else if (additionalFieldType == 2)
        {
            QComboBox *additionalFieldComboBox = new QComboBox(this);
            additionalFieldWidget = additionalFieldComboBox;
            QStandardItemModel* additionalFieldComboBoxModel = new QStandardItemModel();
            // TODO: нужно проверить удаляется ли модель при удалении вджета
            QStringList additionalFieldComboBoxItems = additionalFieldsList->value(1).toString().split('\n');
            QStandardItem *newRow;
            QFontMetrics *fm = new QFontMetrics(this->font());
            int itemTextWidth, dropDownListWidth = 0;

            additionalFieldComboBox->setModel(additionalFieldComboBoxModel);
//            if (additionalFieldComboBoxItems.at(0) != "")   // Принудительно добавляем пустую строку
//            {
//                newRow = new QStandardItem();
//                newRow->setText("");
//                additionalFieldComboBoxModel->appendRow(newRow);
//            }
            for (int i=0; i<additionalFieldComboBoxItems.size(); i++)
            {
                newRow = new QStandardItem();
                newRow->setText(additionalFieldComboBoxItems.at(i));
                additionalFieldComboBoxModel->appendRow(newRow);

                // определяем наибольшую длину текста в списке элементов
                itemTextWidth = fm->size(Qt::TextSingleLine, additionalFieldComboBoxItems.at(i)).width() + 10;
                if (itemTextWidth > dropDownListWidth)
                    dropDownListWidth = itemTextWidth;
            }
            delete fm;  // больше не нужен
            additionalFieldComboBox->setMinimumWidth(100);
            additionalFieldComboBox->view()->setMinimumWidth(dropDownListWidth);
            additionalFieldComboBox->setCurrentIndex(-1);
            additionalFieldComboBox->setEditable(true);
            additionalFieldComboBox->lineEdit()->setPlaceholderText(additionalFieldsList->value(0).toString());
        } else if (additionalFieldType == 3)
        {
            QDateEdit *additionalFieldDateTimeEdit = new QDateEdit(QDate::currentDate());
            additionalFieldWidget = additionalFieldDateTimeEdit;
            additionalFieldDateTimeEdit->setCalendarPopup(true);
        } else if (additionalFieldType == 4)
        {
            // TODO: В АСЦ не реализовано, поэтому используем заглушку из лайнэдита
            QLineEdit *additionalFieldLineEdit = new QLineEdit(this);
            additionalFieldWidget = additionalFieldLineEdit;
            additionalFieldLineEdit->setPlaceholderText(additionalFieldsList->value(0).toString());
            additionalFieldLineEdit->setEnabled(false);
        }
        else
            additionalFieldWidget = new QWidget();  // на случай ошибок в БД

        additionalFieldWidget->setSizePolicy(*additionalFieldsSizePolicy);
        additionalFieldsWidgets.append(additionalFieldWidget);
        ui->gridLayoutDeviceDescription->addWidget(additionalFieldWidget, additionalFieldRow, additionaFieldCol, 1, 1);
        additionalFieldWidget->setProperty("fieldId", additionalFieldsList->value(3).toInt());
        additionalFieldWidget->setProperty("fieldType", additionalFieldType);
        additionalFieldWidget->setProperty("fieldRequired", additionalFieldsList->value(4).toBool());
        additionalFieldWidget->setProperty("fieldPrintable", additionalFieldsList->value(5).toBool());

        if (++additionaFieldCol > 5)
        {
            additionalFieldRow++;
            additionaFieldCol = 0;
        }
    }

//    qDebug() << "tabRepairNew::changeDeviceType: additionalFieldsWidgets.size() =" << additionalFieldsWidgets.size() << "(end of function)";
//    delete additionalFieldsList;

}

void tabRepairNew::changeDeviceMaker()
{
    int comboBoxDeviceIndex = ui->comboBoxDevice->currentIndex();
    int comboBoxDeviceMakerIndex = ui->comboBoxDeviceMaker->currentIndex();
    int deviceId = comboboxDevicesModel->index(comboBoxDeviceIndex, 1).data().toInt();
    int deviceMakerId = comboboxDeviceMakersModel->index(comboBoxDeviceMakerIndex, 1).data().toInt();
    QString query;

    query = QUERY_SEL_DEVICE_MODELS.arg(deviceId).arg(deviceMakerId);
    comboboxDeviceModelsModel->setQuery(query, QSqlDatabase::database("connMain"));
    ui->comboBoxDeviceModel->setCurrentIndex(-1);
}

void tabRepairNew::clearClientCreds(bool hideCoincidence)
{
    setDefaultStyleSheets();
    exist_client_id = 0;
    ui->lineEditClientLastName->clear();
    ui->lineEditClientFirstName->clear();
    ui->lineEditClientPatronymic->clear();
    ui->lineEditClientPhone1->clear();
    ui->comboBoxClientPhone1Type->setCurrentIndex(clientPhoneTypesList->index(0, 0).row());     // устанавливаем первый элемент выпадающего списка
    ui->comboBoxClientAdType->setCurrentIndex(-1);
    ui->lineEditClientAddress->clear();
    ui->lineEditClientEmail->clear();
    ui->lineEditClientPhone2->clear();
    ui->comboBoxClientPhone2Type->setCurrentIndex(clientPhoneTypesList->index(0, 0).row());     // устанавливаем первый элемент выпадающего списка
    if (hideCoincidence)
        ui->groupBoxClientCoincidence->hide();
}

void tabRepairNew::lineEditPrevRepairButtonsHandler(int button)
{
    if (button == 0)
        emit createTabSelectPrevRepair(1);
    else if (button ==1)
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

void tabRepairNew::fillClientCreds(int id)
{
    clientModel = new QSqlQueryModel();
    clientPhonesModel = new QSqlQueryModel();

    QString query;
    query = QUERY_SEL_CLIENT(id);
    clientModel->setQuery(query, QSqlDatabase::database("connMain"));

    clearClientCreds(false);    // очищаем данные клиента, но не прячем таблицу совпадений

    if (clientModel->record(0).value("type").toBool())
        ui->checkBoxClientType->setChecked(true);
    else
        ui->checkBoxClientType->setChecked(false);
    changeClientType();

    exist_client_id = id;
    ui->lineEditClientFirstName->setText(clientModel->record(0).value("name").toString());
    ui->lineEditClientLastName->setText(clientModel->record(0).value("surname").toString());
    ui->lineEditClientPatronymic->setText(clientModel->record(0).value("patronymic").toString());
    ui->lineEditClientAddress->setText(clientModel->record(0).value("address").toString());
    ui->lineEditClientEmail->setText(clientModel->record(0).value("email").toString());
//    ui->lineEditClientEmail->setText(clientModel->record(0).value("").toString());

    query = QString(QUERY_SEL_CLIENT_PHONES(id));
    clientPhonesModel->setQuery(query, QSqlDatabase::database("connMain"));

    // заполняем типы телефонов. Пока так, потом придумаю что-то более элегантное
    if(clientPhonesModel->rowCount() > 1)   // если результат содержит более одной строки, то в поле доп. номера записываем данные из второй строки результатов
    {
        ui->comboBoxClientPhone2Type->setCurrentIndex(clientPhoneTypesList->index((clientPhonesModel->index(1, 1).data().toInt() - 1), 0).row());    // сначала устанавливаем тип в комбобоксе, чтобы применилась маска к полю
        ui->lineEditClientPhone2->setText(clientPhonesModel->index(1, 0).data().toString());    // теперь устанавливаем номер телефона
    }
    if(clientPhonesModel->rowCount() > 0)
    {
        ui->comboBoxClientPhone1Type->setCurrentIndex(clientPhoneTypesList->index((clientPhonesModel->index(0, 1).data().toInt() - 1), 0).row());    //
        ui->lineEditClientPhone1->setText(clientPhonesModel->index(0, 0).data().toString());
    }

    ui->comboBoxProblem->setFocus();    // устанавливаем фокус на полее ввода неисправности
}

void tabRepairNew::fillDeviceCreds(int id)
{
    int i;
    int repair, device, deviceMaker, deviceModel, client;
    queryDevice = new QSqlQueryModel();

    QString query;
    query = QUERY_SEL_DEVICE(id);
    queryDevice->setQuery(query, QSqlDatabase::database("connMain"));

    repair = queryDevice->record(0).value("id").toInt();
    device = queryDevice->record(0).value("type").toInt();
    deviceMaker = queryDevice->record(0).value("maker").toInt();
    deviceModel = queryDevice->record(0).value("model").toInt();
    client = queryDevice->record(0).value("client").toInt();

    fillClientCreds(client);
//    if (ui->comboBoxDevice->currentIndex() < 0)        // только если пользователь не выбрал тип уст-ва
    {
        for (i = 0; i < comboboxDevicesModel->rowCount(); i++)  // перебираем все типы уст-в в поисках нужного id
        {
            if (comboboxDevicesModel->index(i, 1).data().toInt() == device)
                break;
        }
        ui->comboBoxDevice->setCurrentIndex(i);
        // TODO: сделать запросы к БД асинхронными. Готовые классы https://github.com/micjabbour/MSqlQuery
        // на данный момент по локальной сети всё работает быстро, поэтому не буду заморачиваться

        for (i = 0; i < comboboxDeviceMakersModel->rowCount(); i++)  // перебираем всех производителей уст-в в поисках нужного id
        {
            if (comboboxDeviceMakersModel->index(i, 1).data().toInt() == deviceMaker)
                break;
        }
        ui->comboBoxDeviceMaker->setCurrentIndex(i);
        for (i = 0; i < comboboxDeviceModelsModel->rowCount(); i++)  // перебираем все модели уст-в в поисках нужной id
        {
            if (comboboxDeviceModelsModel->index(i, 1).data().toInt() == deviceModel)
                break;
        }
        ui->comboBoxDeviceModel->setCurrentIndex(i);
        ui->lineEditSN->setText(queryDevice->record(0).value("serial_number").toString());
    }
    ui->lineEditPrevRepair->setText(QString::number(repair));  // устанавливаем номер предыдущего ремонта в соотв. поле
    ui->checkBoxWasEarlier->setCheckState(Qt::Checked);     // ставим галочку "Ранее было в ремонте" чтобы поле с номером ремонта отобразилось

    ui->comboBoxProblem->setFocus();    // устанавливаем фокус на полее ввода неисправности
}

void tabRepairNew::buttonSelectExistingClientHandler()
{
    emit createTabSelectExistingClient(1);
}

void tabRepairNew::findMatchingClient(QString text)
{
    QString lineEditClientPhone1DisplayText = ui->lineEditClientPhone1->displayText();  // отображаемый текст
    QString currentPhone1InputMask = clientPhoneTypesList->index(ui->comboBoxClientPhone1Type->currentIndex(), 2).data().toString().replace(QRegularExpression("[09]"), "_");   // в маске телефона меняем 0 и 9 на подчеркивание; 0 и 9 — это специальные маскировочные символы (см. справку QLineEdit, inputMask)
    QString enteredByUserDigits;    // строка символов, которые ввёл пользователь (т. е. текст отображаемый в lineEdit над которым выполнена операция т. н. XOR с заданной маской
    QStringList query_where;    // список условий для запроса к БД
    QString query;  // весь запрос к БД
    int i;

    //    qDebug() << "currentPhone1InputMask " << currentPhone1InputMask << " lineEditClientPhone1DisplayText " << lineEditClientPhone1DisplayText;
    for (i = 0; i < currentPhone1InputMask.length(); i++ )  // определяем какие символы ввёл пользователь
    {
        if(currentPhone1InputMask.at(i) != lineEditClientPhone1DisplayText.at(i))
            enteredByUserDigits.append(lineEditClientPhone1DisplayText.at(i));
    }

//    qDebug() << "enteredByUserDigits " << enteredByUserDigits;
    if(ui->lineEditClientLastName->text().length() >= 3 || enteredByUserDigits.length() >= 3 )  // если пользователь ввёл более двух символов в одно из полей
    {
        query_where.clear();
        if (ui->lineEditClientLastName->text().length() >= 3 )
            query_where << QString("LCASE(CONCAT_WS(' ', t1.`surname`, t1.`name`, t1.`patronymic`)) REGEXP LCASE('%1')").arg(ui->lineEditClientLastName->text());   // условие поиска по фамилии, имени и отчеству
        if (enteredByUserDigits.length() >= 3 )
            query_where << QString("IFNULL(t2.`phone`, '') LIKE '%1' OR IFNULL(t2.`phone_clean`, '') REGEXP '%2'").arg(lineEditClientPhone1DisplayText, enteredByUserDigits);   // условие поиска по телефонному номеру

        ui->tableViewClientMatch->setModel(clientsMatchTable);  // указываем модель таблицы
        query = QUERY_SEL_CLIENT_MATCH.arg((query_where.count()>0?"AND (" + query_where.join(" OR ") + ")":""));
        clientsMatchTable->setQuery(query, QSqlDatabase::database("connMain"));

        // изменяем размер столбцов под соедржимое.
        // TODO: нужно бы создать свой класс с наследованием QTableView, реализовать в нём пропорциональное изменение ширин столбцов
        // при изменении размера окна и тултип для длинного текста (несколько телефонов в одной ячейке). Этот класс использовать везде
        ui->tableViewClientMatch->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        if (clientsMatchTable->rowCount() > 0)
        {
            ui->groupBoxClientCoincidence->show();  // только если возвращены результаты, показываем таблицу совпадения клиента
            ui->tableViewClientMatch->show();       // может случиться ситуация, когда таблица будет скрыта, поэтому принудительно отображаем её
        }
        else
            ui->groupBoxClientCoincidence->hide();  // иначе прячем таблицу
    }
    else
    {
        clientsMatchTable->clear(); // если кол-во введённых пользователем символов меньше трёх, то удаляем результаты предыдущего запроса и прячем таблицу.
        ui->groupBoxClientCoincidence->hide();
    }
}

void tabRepairNew::findMatchingDevice(QString text)
{
//    qDebug() << "findMatchingDevice(QString text)";
    QString query;

    if( text.length() >= 3 )  // если пользователь ввёл более двух символов
    {
        ui->tableViewDeviceMatch->setModel(devicesMatchTable);  // указываем модель таблицы

        // TODO: сейчас regexp будет работать неправильно, т. к. производится преобразование регистра для расширения диапазона поиска
        // Возможно нужно доработать этот механизм, чтобы рег. выражения работали полноценно
        query = QUERY_SEL_DEVICE_MATCH(text);
        devicesMatchTable->setQuery(query, QSqlDatabase::database("connMain"));

        // прячем столбцы с кодами типа уст-ва, производителем, моделью и клиентом (они запрашиваются для автозаполнения полей при выборе совпадающего)
        ui->tableViewDeviceMatch->hideColumn(5);
        ui->tableViewDeviceMatch->hideColumn(6);
        ui->tableViewDeviceMatch->hideColumn(7);
        ui->tableViewDeviceMatch->hideColumn(8);

        // изменяем размер столбцов под соедржимое.
        // TODO: заменить QTableView на свой класс и реализовать в нём пропорциональное изменение ширин столбцов
        // при изменении размера окна и тултип для длинного текста (неисправность).
        ui->tableViewDeviceMatch->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

        if (devicesMatchTable->rowCount() > 0)
        {
            ui->groupBoxDeviceCoincidence->show();  // только если возвращены результаты, показываем таблицу совпадения устройства
            ui->tableViewDeviceMatch->show();       // может случиться ситуация, когда таблица будет скрыта, поэтому принудительно отображаем её
        }
        else
            ui->groupBoxDeviceCoincidence->hide();  // иначе прячем таблицу
    }
    else
    {
        devicesMatchTable->clear(); // если кол-во введённых пользователем символов меньше трёх, то удаляем результаты предыдущего запроса и прячем таблицу.
        ui->groupBoxDeviceCoincidence->hide();
    }
}

void tabRepairNew::phone1TypeChanged(int index)
{
    phoneTypeChanged(1, index);
}

void tabRepairNew::phone2TypeChanged(int index)
{
    phoneTypeChanged(2, index);
}

void tabRepairNew::deviceMatchTableDoubleClicked(QModelIndex item)
{
    fillDeviceCreds(devicesMatchTable->index(item.row(), 0).data().toInt());
//    ui->groupBoxClientCoincidence->hide();  // прячем таблицу совпадения клиента
    ui->tableViewDeviceMatch->hide();   // прячем таблицу, а не весь groupBox (вдруг пользователь промахнётся)
    ui->groupBoxClientCoincidence->hide();  // прячем таблицу совпадения клиента (если она по какой-то причине отображается)
}

void tabRepairNew::clientMatchTableDoubleClicked(QModelIndex item)
{
    fillClientCreds(clientsMatchTable->index(item.row(), 0).data().toInt());
//    ui->groupBoxClientCoincidence->hide();  // прячем таблицу совпадения клиента
    ui->tableViewClientMatch->hide();   // прячем таблицу, а не весь groupBox (вдруг пользователь промахнётся)
}

void tabRepairNew::lineEditSNClearHandler(int)
{
    if(ui->lineEditSN->text().length())
        ui->lineEditSN->clear();
    else
        ui->lineEditSN->setText("НЕТ S/N");

    devicesMatchTable->clear(); // то удаляем результаты предыдущего запроса и прячем таблицу.
    ui->groupBoxDeviceCoincidence->hide();
}

void tabRepairNew::setDefaultStyleSheets()
{
    QString commonComboBoxStyleSheet = "QComboBox {  border: 1px solid gray;  padding: 1px 18px 1px 3px;}\
            QComboBox::drop-down {  border: 0px;}\
            QComboBox::down-arrow{  image: url(down-arrow.png);  width: 16px;  height: 20px;}\
            QComboBox::hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}\
            QComboBox::down-arrow:hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}";
    QString commonLineEditStyleSheet = "";
    QString commonDateEditStyleSheet = "";

    ui->comboBoxDevice->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxDeviceMaker->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxDeviceModel->setStyleSheet(commonComboBoxStyleSheet);
    ui->lineEditSN->setStyleSheet(commonLineEditStyleSheet);
    ui->lineEditClientLastName->setStyleSheet(commonLineEditStyleSheet);
    ui->lineEditClientFirstName->setStyleSheet(commonLineEditStyleSheet);
    ui->lineEditClientPatronymic->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxClientAdType->setStyleSheet(commonComboBoxStyleSheet);
    ui->lineEditClientAddress->setStyleSheet(commonLineEditStyleSheet);
    ui->lineEditClientEmail->setStyleSheet(commonLineEditStyleSheet);
    ui->lineEditClientPhone1->setStyleSheet(commonLineEditStyleSheet);
    ui->comboBoxProblem->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxIncomingSet->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxExterior->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetEngineer->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetBox->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxOffice->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxCompany->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetPaymentAccount->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPrepayAccount->setStyleSheet(commonComboBoxStyleSheet);
    ui->comboBoxPresetEngineer->setStyleSheet(commonComboBoxStyleSheet);

    for(int i=0; i< additionalFieldsWidgets.size(); i++)   // установка стилей доп. полей
    {
        if ( additionalFieldsWidgets[i]->property("fieldRequired").toBool() )
        {
            if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QComboBox", Qt::CaseSensitive) == 0 )
            {
                additionalFieldsWidgets[i]->setStyleSheet(commonComboBoxStyleSheet);
            }
            else if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QLineEdit", Qt::CaseSensitive) == 0 )
                additionalFieldsWidgets[i]->setStyleSheet(commonLineEditStyleSheet);
            // Дату не окрашиваем, даже если она обязательна, т. к. не продуман механизм проверки
//            else if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QDateEdit", Qt::CaseSensitive) == 0 )
//                    additionalFieldsWidgets[i]->setStyleSheet(commonDateEditStyleSheet);   // Дату не окрашиваем, даже если она обязательна, т. к. не продуман механизм проверки
        }
    }

}

int tabRepairNew::createRepair()
{
    int error = 0;
    QString commonComboBoxStyleSheetRed = "QComboBox {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}\
            QComboBox::drop-down {  border: 0px;}\
            QComboBox::down-arrow{  image: url(down-arrow.png);  width: 16px;  height: 20px;}\
            QComboBox::hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}\
            QComboBox::down-arrow:hover{  border: 1px solid #0078D7;  background-color: #E5F1FB;}";
    QString commonLineEditStyleSheetRed = "QLineEdit {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}";
    QString commonDateEditStyleSheetRed = "QDateEdit {  border: 1px solid red;  padding: 1px 18px 1px 3px; background: #FFD1D1;}";

    setDefaultStyleSheets();

    if (ui->comboBoxDevice->currentIndex() < 0)        // если не выбран тип уст-ва
    {
        ui->comboBoxDevice->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 1;
    }
    if (ui->comboBoxDeviceMaker->currentIndex() < 0)        // если не выбран производителя
    {
        ui->comboBoxDeviceMaker->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 2;
    }
    if (ui->comboBoxDeviceModel->currentIndex() < 0 && ui->comboBoxDeviceModel->currentText() == "")        // если не выбрана или не написана модель
    {
        ui->comboBoxDeviceModel->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 3;
    }
    if (ui->lineEditSN->text() == "" && comSettings->value("is_sn_req").toBool())   // если не записан серийный номер, а он обязателен
    {
        ui->lineEditSN->setStyleSheet(commonLineEditStyleSheetRed);
        error = 4;
    }
    if (ui->lineEditClientLastName->text() == "")       // если не указана фамилия (или название организации)
    {
        ui->lineEditClientLastName->setStyleSheet(commonLineEditStyleSheetRed);
        error = 5;
    }
    if (ui->lineEditClientFirstName->text() == "" && ui->checkBoxClientType->checkState() == 0)     // если не указано имя (только для физ. лиц)
    {
        ui->lineEditClientFirstName->setStyleSheet(commonLineEditStyleSheetRed);
        error = 6;
    }
    if (ui->lineEditClientPatronymic->text() == "" && ui->checkBoxClientType->checkState() == 0 && comSettings->value("is_patronymic_required").toBool() && !exist_client_id)   // если не указано отчество и оно обязятельно (только для физ. лиц и только для новых клиентов)
    {
        ui->lineEditClientPatronymic->setStyleSheet(commonLineEditStyleSheetRed);
        error = 7;
    }
    if (ui->comboBoxClientAdType->currentIndex() < 0 && comSettings->value("visit_source_force").toBool() && !exist_client_id)        // если не указан источник обращения, а он обязателен и клиент новый
    {
        ui->comboBoxClientAdType->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 8;
    }
    if (ui->lineEditClientAddress->text() == "" && comSettings->value("address_required").toBool() && !exist_client_id)   // если не указан адрес, а он обязателен и клиент новый
    {
        ui->lineEditClientAddress->setStyleSheet(commonLineEditStyleSheetRed);
        error = 9;
    }
    if (ui->lineEditClientEmail->text() == "" && comSettings->value("email_required").toBool() && !exist_client_id)   // если не указан email, а он обязателен и клиент новый
    {
        ui->lineEditClientEmail->setStyleSheet(commonLineEditStyleSheetRed);
        error = 10;
    }
    if (!ui->lineEditClientPhone1->hasAcceptableInput() && comSettings->value("phone_required").toBool())   // если не указано отчество и оно обязятельно (только для физ. лиц)
    {
        ui->lineEditClientPhone1->setStyleSheet(commonLineEditStyleSheetRed);
        error = 11;
    }
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
    if (ui->comboBoxPresetEngineer->currentIndex() < 0 && comSettings->value("is_master_set_on_new").toBool())        // если не выбран инженер, а он обязателен
    {
        ui->comboBoxPresetEngineer->setStyleSheet(commonComboBoxStyleSheetRed);
        error = 15;
    }
    for(int i=0; i< additionalFieldsWidgets.size(); i++)   // установка стилей доп. полей
    {
        if ( additionalFieldsWidgets[i]->property("fieldRequired").toBool() )
        {
            if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QComboBox", Qt::CaseSensitive) == 0 )
            {
                if (static_cast<QComboBox*>(additionalFieldsWidgets[i])->currentIndex() < 0)
                {
                    additionalFieldsWidgets[i]->setStyleSheet(commonComboBoxStyleSheetRed);
                }
            }
            else if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QLineEdit", Qt::CaseSensitive) == 0 )
            {
                if (static_cast<QLineEdit*>(additionalFieldsWidgets[i])->text() == "")
                    additionalFieldsWidgets[i]->setStyleSheet(commonLineEditStyleSheetRed);
            }
            // Дату не окрашиваем, даже если она обязательна, т. к. не продуман механизм проверки
//            else if ( QString(additionalFieldsWidgets[i]->metaObject()->className()).compare("QDateEdit", Qt::CaseSensitive) == 0 )
//            {
//                if (static_cast<QDateEdit>(additionalFieldsWidgets[i]).text() == "")
//                    additionalFieldsWidgets[i]->setStyleSheet(commonDateEditStyleSheetRed);
//            }
        }
    }

    if (error)
    {
        qDebug() << "Ошибка создания карты ремонта: не все обязательные поля заполнены (error " << error << ")";
        return error;
    }

}

void tabRepairNew::createRepairClose()
{
    if (!createRepair())
        this->deleteLater();
}

void tabRepairNew::phoneTypeChanged(int type, int index)
{
    switch (type)
    {
        // т. к. полей для телефона всего два, то нет смысла городить какую-то универсальную конструкцию.
        case 1: ui->lineEditClientPhone1->setInputMask(""); ui->lineEditClientPhone1->setInputMask(clientPhoneTypesList->index(index, 2).data().toString() + ";_"); break;  // Here ";_" for filling blank characters with underscore
        case 2: ui->lineEditClientPhone2->setInputMask(""); ui->lineEditClientPhone2->setInputMask(clientPhoneTypesList->index(index, 2).data().toString() + ";_"); break;
    }
}
