#include "scartridgeform.h"
#include "ui_scartridgeform.h"
#include "models/scartridgematerialsmodel.h"
#include "tabrepairs.h"

SCartridgeForm::SCartridgeForm(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SCartridgeForm)
{
    ui->setupUi(this);
    guiFontChanged();
    setDefaultStyleSheets();

//    ui->frameHeader->setStyleSheet("QFrame::hover {background-color: rgb(205,230,247);}");    // подсветка заголовка

    connect(userDbData, &SUserSettings::fontSizeChanged, this, &SCartridgeForm::guiFontChanged);
    installEventFilter(this);
}

SCartridgeForm::SCartridgeForm(const int repairId, QWidget *parent) :
    SCartridgeForm(parent)
{
    m_repairId = repairId;
    try
    {
        initWidgets();
    }
    catch (...)
    {
        m_repairId = 0;
    }
}

SCartridgeForm::~SCartridgeForm()
{
    delete ui;
    delete m_cartridgeCard; // перед удалением модели m_repair
    if(m_repair)
        delete m_repair;
    if(m_BOQModel)
        delete m_BOQModel;
    if(statusesProxyModel)
        delete statusesProxyModel;
}

void SCartridgeForm::initModels()
{
    m_cartridgeCard = new SCartridgeCardModel(this);
    if(m_repairId)
    {
        m_repair = new SRepairModel();
        connect(m_repair, &SRepairModel::modelUpdated, this, &SCartridgeForm::updateWidgets);
        m_cartridgeCard->setParent(m_repair);

        statusesProxyModel = new SSortFilterProxyModel;
        statusesProxyModel->setSourceModel(comSettings->repairStatuses.Model);
        statusesProxyModel->setFilterKeyColumn(Global::RepStateHeaders::Id);
        statusesProxyModel->setFilterRegularExpression("");

        m_BOQModel = new SSaleTableModel();
        m_BOQModel->setMode(SSaleTableModel::WorkshopSale);
        m_BOQModel->setPriceColumn(SStoreItemModel::PriceOptionService);
        connect(m_BOQModel, &SSaleTableModel::amountChanged, this, &SCartridgeForm::updateTotalSumms);
        connect(m_BOQModel, &SSaleTableModel::tableSaved, this, &SCartridgeForm::saveTotalSumms);
        connect(m_BOQModel, &SSaleTableModel::modelReset, this, &SCartridgeForm::updateLists);
        connect(m_BOQModel, &SSaleTableModel::modelReset, this, &SCartridgeForm::updateWorksActionsCheckedState);
        m_BOQModel->setEditStrategy(SSaleTableModel::OnManualSubmit);
        m_BOQModel->setRepairType(SSaleTableModel::CartridgeRepair);
        m_BOQModel->setCartridgeCardModel(m_cartridgeCard);
        m_repair->setBOQModel(m_BOQModel);
    }

    updateModels();
}

void SCartridgeForm::updateModels()
{
    if(m_repairId)
    {
        m_repair->load(m_repairId);
        if(!m_repair->id())
            throw 1;

        m_serialNumber = m_repair->serialNumber();
        m_cardId = m_repair->cartridge()->cardId();
        m_clientId = m_repair->clientId();
        m_BOQModel->repair_loadTable(m_repair->id());
        updateStatesModel(m_repair->state());
    }

    updateCardModel();
}

void SCartridgeForm::updateCardModel()
{
    m_cartridgeCard->load(m_cardId);
    if(m_repairId)
    {
        updateWorksCheckBoxes();
        updateWorksMenu();
    }
}

void SCartridgeForm::randomFill()
{
    if(ui->lineEditSerial->text().isEmpty())
        ui->lineEditSerial->setText(QString::number(QRandomGenerator::global()->bounded(9999)));
}

void SCartridgeForm::initWidgets()
{
    QIcon activeCheckBoxIcon;
    activeCheckBoxIcon.addFile(QString::fromUtf8(":/icons/light/checkbox-unchecked.png"), QSize(), QIcon::Normal, QIcon::Off);
    activeCheckBoxIcon.addFile(QString::fromUtf8(":/icons/light/checkbox-checked.png"), QSize(), QIcon::Normal, QIcon::On);

    QIcon unactiveCheckBoxIcon;
    unactiveCheckBoxIcon.addFile(QString::fromUtf8(":/icons/light/checkbox-flat-unchecked.png"), QSize(), QIcon::Normal, QIcon::Off);
    unactiveCheckBoxIcon.addFile(QString::fromUtf8(":/icons/light/checkbox-flat-checked.png"), QSize(), QIcon::Normal, QIcon::On);

    initModels();
    ui->comboBoxWasEarly->setModel(cartridgeRepeatReason);
    ui->comboBoxWasEarly->setCurrentIndex(-1);
    ui->comboBoxWasEarly->disableWheelEvent(true);
    ui->comboBoxWasEarly->view()->setMinimumWidth(230);
    ui->comboBoxPlace->setButtons("Clear");
    ui->comboBoxPlace->setModel(repairBoxesModel);
    ui->comboBoxPlace->setCurrentIndex(-1);
    ui->comboBoxPlace->disableWheelEvent(true);
    ui->comboBoxPlace->view()->setMinimumWidth(150);
    connect(ui->comboBoxPlace, qOverload<int>(&QComboBox::currentIndexChanged), this, &SCartridgeForm::savePlace);
    connect(ui->comboBoxPlace, &SComboBox::buttonClicked, this, &SCartridgeForm::comboBoxPlaceButtonClickHandler);
    ui->labelLimitReached->setVisible(prevRepairsCount() >= m_cartridgeCard->resource());
    connect(ui->toolButtonRemove, &QToolButton::clicked, this, &SCartridgeForm::removeWidget);
    ui->comboBoxState->view()->setMinimumWidth(230);
    ui->labelRepeatWarranty->setVisible(false);

    if(m_repairId)
    {
        m_lineEditStyleSheet.replace("background: #FFFFFF;", "background-color: rgba(255, 255, 255, 0);");
        m_spinBoxStyleSheet.replace("background: #FFFFFF;", "background-color: rgba(255, 255, 255, 0);");
        ui->lineEditSerial->setStyleSheet(m_lineEditStyleSheet);
        ui->spinBoxRefillWeight->setStyleSheet(m_spinBoxStyleSheet);
        ui->doubleSpinBoxPreagreedAmount->setStyleSheet(m_spinBoxStyleSheet);

        initCheckBoxWidgets(unactiveCheckBoxIcon, ui->pushButtonPreagreedRefill, true, true);
        initCheckBoxWidgets(unactiveCheckBoxIcon, ui->pushButtonPreagreedChipReplace, true, true);
        initCheckBoxWidgets(unactiveCheckBoxIcon, ui->pushButtonPreagreedDrumReplace, true, true);
        initCheckBoxWidgets(unactiveCheckBoxIcon, ui->pushButtonPreagreedBladeReplace, true, true);
        ui->comboBoxEngineer->setModel(engineersModel);
        ui->comboBoxEngineer->disableWheelEvent(true);
        connect(ui->comboBoxEngineer, qOverload<int>(&QComboBox::currentIndexChanged), this, &SCartridgeForm::comboBoxEngineerChanged);
        ui->comboBoxState->setModel(statusesProxyModel);
        ui->comboBoxState->setCurrentIndex(-1);
        ui->comboBoxState->disableWheelEvent(true);
        connect(ui->comboBoxState, qOverload<int>(&QComboBox::currentIndexChanged), this, &SCartridgeForm::stateIndexChanged);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonRefill, false, true);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonChipReplace, false, true);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonDrumReplace, false, true);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonBladeReplace, false, true);
        connect(ui->pushButtonClientCard, &QPushButton::clicked, this, &SCartridgeForm::buttonClientCardClicked);
        connect(ui->toolButtonClassicTab, &QPushButton::clicked, this, &SCartridgeForm::buttonClassicTabClicked);
        connect(ui->toolButtonCartridgeCard, &QPushButton::clicked, this, &SCartridgeForm::buttonCartridgeCardClicked);
        connect(ui->lineEditComment, &QLineEdit::editingFinished, this, &SCartridgeForm::updateComment);
        ui->labelWasEarly->hide();
        ui->comboBoxWasEarly->hide();

        updateHeader();
        updateWidgets();
    }
    else
    {
        hideWidgetsOnReceiptForm();
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonPreagreedRefill);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonPreagreedChipReplace);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonPreagreedDrumReplace);
        initCheckBoxWidgets(activeCheckBoxIcon, ui->pushButtonPreagreedBladeReplace);
        ui->comboBoxWasEarly->setCurrentIndex(m_isRepeat?1:-1);

        connect(ui->pushButtonPreagreedRefill, &QPushButton::toggled, this, &SCartridgeForm::setRefill);
        connect(ui->pushButtonPreagreedChipReplace, &QPushButton::toggled, this, &SCartridgeForm::setChipReplace);
        connect(ui->pushButtonPreagreedDrumReplace, &QPushButton::toggled, this, &SCartridgeForm::setDrumReplace);
        connect(ui->pushButtonPreagreedBladeReplace, &QPushButton::toggled, this, &SCartridgeForm::setBladeReplace);
        ui->pushButtonPreagreedRefill->setChecked(true);
        setRefill(true);    // почему-то при вызове метода setChecked() слот setRefill() не вызывается
    }
    m_initDone = 1;
}

/* Натсройка виджетов предварительно согласованных работ
 * isFlat: 0 - форма приёма картриджей; 1 - форма работы (заправки) с картриджем
*/
void SCartridgeForm::initCheckBoxWidgets(const QIcon &icon, QPushButton *button, bool isFlat, bool installEventFilter)
{
    button->setCheckable(true);
    button->setIcon(icon);
    button->setStyleSheet(QString::fromUtf8("QPushButton {"
                                            "  background-color: rgba(240, 240, 240, 0);"
                                            "}"));
    if(isFlat)
    {
        button->setFlat(true);
    }

    if(installEventFilter)
    {
        button->installEventFilter(this);
    }
}

void SCartridgeForm::updateHeader()
{
    ui->labelTitle->setText(QString("%1 [%2]").arg(m_repair->id()).arg(m_repair->title()));

    SClientModel *client = new SClientModel(m_repair->clientId());
    QString name = client->fullLongName();
    if(this->fontMetrics().horizontalAdvance(name) > 400)   // TODO: заменить жестко заданное значение
    {
        if(client->type() && !client->shortName().isEmpty() )
            name = client->shortName();
        else if(!client->type())
            name = client->fullShortName();
        else
            ui->pushButtonClientCard->setMaximumWidth(400);
    }
    ui->pushButtonClientCard->setText(name);
    delete client;  // TODO: модель нужна только для получения ФИО; подумать над более оптимальным способом.
}

void SCartridgeForm::updateWidgets()
{
    ui->labelRepeatWarranty->setVisible(false);
    if(m_repair->isRepeat() || m_prevRepairsCount)
        updateLabelRepeatWarranty(tr("Повтор"));

    if(m_repair->isWarranty())
        updateLabelRepeatWarranty(tr("Гарантия"));

    ui->lineEditSerial->setText(m_repair->serialNumber());
    ui->lineEditSerial->setReadOnly(true);
    ui->pushButtonPreagreedRefill->setChecked(m_repair->cartridge()->refill());
    ui->pushButtonPreagreedChipReplace->setChecked(m_repair->cartridge()->chip());
    ui->pushButtonPreagreedDrumReplace->setChecked(m_repair->cartridge()->drum());
    ui->pushButtonPreagreedBladeReplace->setChecked(m_repair->cartridge()->Blade());
    ui->spinBoxRefillWeight->setValue(m_cartridgeCard->tonerWeight());
    ui->doubleSpinBoxPreagreedAmount->setValue(m_repair->preAgreedAmount());
    ui->comboBoxWasEarly->setCurrentIndex(m_repair->isRepeat()?1:-1);
    ui->comboBoxWasEarly->setCurrentIndex(m_repair->isWarranty()?0:-1);
    ui->comboBoxWasEarly->setEnabled(false);

    setWidgetsParams(m_repair->state());

    ui->comboBoxEngineer->setEnabled(engineerComboBoxEn && (permissions->setRepairEngineer || permissions->beginUnengagedRepair));
    updateComboBoxEngineer(m_repair->engineer());
    ui->doubleSpinBoxTotalAmount->setValue(m_repair->realRepairCost());
    ui->comboBoxPlace->setCurrentIndex(m_repair->boxIndex());
    ui->comboBoxPlace->setEnabled(placeComboBoxEn);
    ui->lineEditComment->setText(m_repair->extNotes());
}

void SCartridgeForm::updateWorksCheckBoxes()
{
    if(m_repairId)
    {
        ui->pushButtonRefill->setEnabled(worksCheckboxesEn && m_cartridgeCard->isMaterialSet(SCartridgeMaterialModel::Toner));
        ui->pushButtonChipReplace->setEnabled(worksCheckboxesEn && m_cartridgeCard->isMaterialSet(SCartridgeMaterialModel::Chip));
        ui->pushButtonDrumReplace->setEnabled(worksCheckboxesEn && m_cartridgeCard->isMaterialSet(SCartridgeMaterialModel::Drum));
        ui->pushButtonBladeReplace->setEnabled(worksCheckboxesEn && m_cartridgeCard->isMaterialSet(SCartridgeMaterialModel::Blade));
        ui->toolButtonOtherWorksMenu->setEnabled(worksCheckboxesEn);
    }
}

/*  Скрытие виджетов, не используемых при приёме картриджа
*/
void SCartridgeForm::hideWidgetsOnReceiptForm()
{
    ui->labelRepeatWarranty->hide();
    ui->pushButtonClientCard->hide();
    ui->toolButtonClassicTab->hide();
    ui->toolButtonCartridgeCard->hide();

    ui->labelRefillWeight->hide();
    ui->spinBoxRefillWeight->hide();    // в АСЦ при приёме это поле отображается, но в нём нет смысла
    ui->labelEngineer->hide();
    ui->comboBoxEngineer->hide();
    ui->labelRefill->hide();
    ui->pushButtonRefill->hide();
    ui->labelRealRefillWeight->hide();
    ui->spinBoxRealRefillWeight->hide();
    ui->labelDrumReplace->hide();
    ui->pushButtonDrumReplace->hide();
    ui->labelChipReplace->hide();
    ui->pushButtonChipReplace->hide();
    ui->labelBladeReplace->hide();
    ui->pushButtonBladeReplace->hide();
    ui->labelState->hide();
    ui->comboBoxState->hide();
    ui->labelTotalAmount->hide();
    ui->doubleSpinBoxTotalAmount->hide();
    ui->labelOtherWorks->hide();
    ui->toolButtonOtherWorksMenu->hide();

    ui->listWidgetWorks->hide();
    ui->listWidgetParts->hide();
}

void SCartridgeForm::updateLabelRepeatWarranty(const QString text)
{
    ui->labelRepeatWarranty->setText(text);
    ui->labelRepeatWarranty->setVisible(true);
}

bool SCartridgeForm::eventFilter(QObject *watched, QEvent *event)
{
//    if(event->type() != QEvent::Paint)
//        qDebug().nospace() << "[" << this << "] eventFilter() | watched: " << m_repair->objectName() << "; event->type(): " << event->type();

    // TODO: выделение виджета (фокус):
    // QEvent::MouseButtonPress
    // любое взаимодействие с дочерними виджетами

    if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
    {
        if(!m_initDone)     // Если инициализация моделей и виджетов не выполнена, то обработка событий не должна выполняться
            return false;

        QPushButton *checkBoxWidget = dynamic_cast<QPushButton*>(watched);
        if(checkBoxWidget)
        {
            if(static_cast<QMouseEvent*>(event)->button() != Qt::MouseButton::LeftButton)   // Achtung! Щелчек любой кнопкой мыши, кроме левой, по любому QPushButton на форме будет проигнорирован
                return true;

            if( checkBoxWidget == ui->pushButtonPreagreedRefill ||
                checkBoxWidget == ui->pushButtonPreagreedDrumReplace ||
                checkBoxWidget == ui->pushButtonPreagreedChipReplace ||
                checkBoxWidget == ui->pushButtonPreagreedBladeReplace )
                return true;

            if(!checkBoxWidget->isEnabled())
                return true;

            int ret = 1;
            int nSt = checkBoxWidget->isChecked()?0:2;
            if(checkBoxWidget == ui->pushButtonRefill)
                ret = !workAndPartHandler(SWorkModel::Type::CartridgeRefill, nSt);
            else if(checkBoxWidget == ui->pushButtonChipReplace)
                ret = !workAndPartHandler(SWorkModel::Type::CartridgeChipReplace, nSt);
            else if(checkBoxWidget == ui->pushButtonDrumReplace)
                ret = !workAndPartHandler(SWorkModel::Type::CartridgeDrumReplace, nSt);
            else if(checkBoxWidget == ui->pushButtonBladeReplace)
                ret = !workAndPartHandler(SWorkModel::Type::CartridgeBladeReplace, nSt);
            return nSt?1:ret; // при добавлении работ нужно фильтровать событие, а при удалении нет
        }
    }

//    QAction *worksMenuAction = dynamic_cast<QAction*>(watched);
//    if(worksMenuAction)
//    {
//        workAndPartHandler(worksMenuAction->property("WorkType").toInt(), worksMenuAction->isChecked());
//    }

    return false;
}

SRepairModel *SCartridgeForm::model()
{
    return m_repair;
}

bool SCartridgeForm::createRepair()
{
    bool nErr = 1;
    m_repair = new SRepairModel(this);

    m_repair->initCartridgeRepairModel(m_cardId);

    QStringList faultList;
    if(ui->pushButtonPreagreedRefill->isChecked())
    {
        faultList.append(tr("Заправка"));
        m_repair->cartridge()->setRefill(1);
    }
    if(ui->pushButtonPreagreedChipReplace->isChecked())
    {
        faultList.append(tr("Чип"));
        m_repair->cartridge()->setChip(1);
    }
    if(ui->pushButtonPreagreedDrumReplace->isChecked())
    {
        faultList.append(tr("Фотовал"));
        m_repair->cartridge()->setDrum(1);
    }
    if(ui->pushButtonPreagreedBladeReplace->isChecked())
    {
        faultList.append(tr("Ракель"));
        m_repair->cartridge()->setBlade(1);
    }
    m_repair->setTitle(m_title);
    m_repair->setClassId(m_deviceClassId);
    m_repair->setVendorId(m_deviceVendorId);
    m_repair->setClientId(m_clientId);
    m_repair->setFault(""+faultList.join(", "));
    m_repair->setSerialNumber(ui->lineEditSerial->text());
    m_repair->setExtNotes(ui->lineEditComment->text());
    switch(ui->comboBoxWasEarly->currentIndex())
    {
        case -1: m_repair->setIsWarranty(0); m_repair->setIsRepeat(0); break;
        case 0: m_repair->setIsWarranty(1); m_repair->setIsRepeat(0); break;
        case 1: m_repair->setIsRepeat(1); m_repair->setIsWarranty(0); break;
    }
    m_repair->setPreAgreedAmount(ui->doubleSpinBoxPreagreedAmount->value());
    m_repair->setRejectReason("");
    m_repair->setCompanyIndex(m_companyIndex);
    m_repair->setOfficeIndex(m_officeIndex);
    m_repair->setStartOfficeIndex(m_officeIndex);
    m_repair->setManager(userDbData->id);
    m_repair->setCurrentManager(userDbData->id);
    m_repair->setEngineerIndex(m_engineerIndex);
    if(m_isHighPriority)
        m_repair->setExpressRepair(1);
    if( m_paymentSystemIndex >= 0)
    {
        m_repair->setPaymentSystemIndex(m_paymentSystemIndex);
        if(paymentSystemsModel->databaseIDByRow(m_paymentSystemIndex, "system_id") == -1)
            m_repair->setIsCardPayment(1);
    }

    nErr = m_repair->commit();

    if(ui->comboBoxPlace->currentIndex() >= 0)
    {
        m_repair->setBoxIndex(ui->comboBoxPlace->currentIndex());
        nErr = m_repair->commit();
    }

    return nErr;
}

bool SCartridgeForm::updateRepair()
{
    return 1;
}

int SCartridgeForm::repairId()
{
    return m_repairId;
}

const QString &SCartridgeForm::title() const
{
    return m_title;
}

void SCartridgeForm::setTitle(const QString &title)
{
    m_title = title;
    ui->labelTitle->setText(m_title);
}

bool SCartridgeForm::isRepeat() const
{
    return m_isRepeat;
}

void SCartridgeForm::setIsRepeat(bool isRepeat)
{
    m_isRepeat = isRepeat;
}

int SCartridgeForm::deviceClassId() const
{
    return m_deviceClassId;
}

void SCartridgeForm::setDeviceClassId(int id)
{
    m_deviceClassId = id;
}

int SCartridgeForm::deviceVendorId() const
{
    return m_deviceVendorId;
}

void SCartridgeForm::setDeviceVendorId(int id)
{
    m_deviceVendorId = id;
}

int SCartridgeForm::deviceModelId() const
{
    return m_deviceModelId;
}

void SCartridgeForm::setDeviceModelId(int id)
{
    m_deviceModelId = id;
}

const QString &SCartridgeForm::serialNumber() const
{
    return m_serialNumber;
}

void SCartridgeForm::setSerialNumber(const QString &serial)
{
    m_serialNumber = serial;
    ui->lineEditSerial->setText(m_serialNumber);
}

int SCartridgeForm::cardId() const
{
    return m_cardId;
}

void SCartridgeForm::setCardId(int id)
{
    m_cardId = id;
}

int SCartridgeForm::clientId() const
{
    return m_clientId;
}

void SCartridgeForm::setClientId(int clientId)
{
    m_clientId = clientId;
}

// индекс (номер строки) модели companiesModel
int SCartridgeForm::companyIndex() const
{
    return m_companyIndex;
}

// индекс (номер строки) модели companiesModel
void SCartridgeForm::setCompanyIndex(int index)
{
    m_companyIndex = index;
}

// индекс (номер строки) модели officesModel
int SCartridgeForm::officeIndex() const
{
    return m_officeIndex;
}

// индекс (номер строки) модели officesModel
void SCartridgeForm::setOfficeIndex(int index)
{
    m_officeIndex = index;
}

// индекс (номер строки) модели engineersModel
int SCartridgeForm::engineerIndex() const
{
    return m_engineerIndex;
}

// индекс (номер строки) модели engineersModel
void SCartridgeForm::setEngineerIndex(int engineer)
{
    m_engineerIndex = engineer;
}

bool SCartridgeForm::isHighPriority() const
{
    return m_isHighPriority;
}

void SCartridgeForm::setIsHighPriority(bool isHighPriority)
{
    m_isHighPriority = isHighPriority;
}

int SCartridgeForm::paymentSystemIndex() const
{
    return m_paymentSystemIndex;
}

void SCartridgeForm::setPaymentSystemIndex(int paymentSystemIndex)
{
    m_paymentSystemIndex = paymentSystemIndex;
}

double SCartridgeForm::preargeedAmount()
{
    return m_amount;
}

bool SCartridgeForm::checkData(const int stateId)
{
    setDefaultStyleSheets();
    if(stateId == Global::RepStateIds::Ready)
    {
        if(m_BOQModel->amountTotal() == 0)
        {
            ui->doubleSpinBoxTotalAmount->setStyleSheet(commonSpinBoxStyleSheetRed);
            throw Global::ThrowType::ConditionsError;
        }
    }
    return 1;
}

void SCartridgeForm::doStateActions(const int stateId)
{
    QList<int> stateActions = comSettings->repairStatuses[stateId].Actions;

    // подробное описание см. в методе tabRepair::doStateActions()
    if(stateId == Global::RepStateIds::InWork)
    {
        stateActions << Global::RepStateActions::SetEngineer;
    }

    // TODO: этот код скопирован из метода tabRepair::doStateActions(), его нужно вынести куда-то в общий доступ
    for(const int &action : qAsConst(stateActions))
        switch (action)
        {
            case Global::RepStateActions::NoPayDiag: setPricesToZero(); break;
            case Global::RepStateActions::ResetInformedStatus: setInformedStatus(0); break;
            case Global::RepStateActions::SetEngineer: initEngineer(); break;
            case Global::RepStateActions::InformManager:; break;
            case Global::RepStateActions::InformEngineer:; break;
        }
}

void SCartridgeForm::updatePreagreedAmount(SCartridgeMaterialModel *material, const bool state)
{
    m_amount = 0;
    preagreedAmounts.insert(material->type(), (material->price()+material->worksPrice())*(state?1:0));
    QMap<int, double>::const_iterator i = preagreedAmounts.constBegin();
    while(i != preagreedAmounts.constEnd())
    {
        m_amount += i.value();
        i++;
    }
    ui->doubleSpinBoxPreagreedAmount->setValue(m_amount);
    emit updateTotalPreagreedAmount();
}

void SCartridgeForm::setDefaultStyleSheets()
{
    ui->lineEditSerial->setStyleSheet(m_lineEditStyleSheet);
    ui->spinBoxRefillWeight->setStyleSheet(m_spinBoxStyleSheet);
    ui->doubleSpinBoxPreagreedAmount->setStyleSheet(m_spinBoxStyleSheet);
    ui->doubleSpinBoxTotalAmount->setStyleSheet(commonSpinBoxStyleSheet);
}

void SCartridgeForm::setWidgetsParams(const int stateId)
{
    worksCheckboxesEn = comSettings->useSimplifiedCartridgeRepair;
    engineerComboBoxEn = 0;
    placeComboBoxEn = 0;

    switch(stateId)
    {
        case Global::RepStateIds::GetIn: engineerComboBoxEn = 1; placeComboBoxEn = 1; break;
        case Global::RepStateIds::InWork: worksCheckboxesEn = 1; engineerComboBoxEn = 1; placeComboBoxEn = 1; break;
        case Global::RepStateIds::Ready:
        case Global::RepStateIds::ReadyNoRepair: worksCheckboxesEn = 0; engineerComboBoxEn = 1; placeComboBoxEn = 1; break;
        case Global::RepStateIds::Returned:
        case Global::RepStateIds::ReturnedNoRepair:
        case Global::RepStateIds::ReturnedInCredit: worksCheckboxesEn = 0; break;
        default: ;
    }
    updateStateWidget(stateId);
}

bool SCartridgeForm::checkStateAcl(const int stateId)
{
    if(m_repair->state() == Global::RepStateIds::GetIn && !m_repair->engineer() && !permissions->beginUnengagedRepair)
        return 0;

    const QString allowedForRoles = comSettings->repairStatuses[stateId].Roles.join('|');

    if(userDbData->roles.contains(QRegularExpression(QString("\\b(%1)\\b").arg(allowedForRoles))))
    {
        return 1;
    }
    return 0;
}

void SCartridgeForm::setPricesToZero()
{
//    tableWorksParts->setPricesToZero();
    m_repair->setRepairCost(0);
}

void SCartridgeForm::setInformedStatus(int status)
{
    if(m_repair->informedStatusIndex())
        return;

    m_repair->setInformedStatusIndex(status);

    if(m_groupUpdate)
        return;

    commit(tr("Успешно"), tr("Статус информирования клиента обновлён"));
}

void SCartridgeForm::initEngineer()
{
    if(m_repair->engineer())
        return;

    m_repair->setEngineer(userDbData->id);
}

void SCartridgeForm::updateComboBoxEngineer(const int id)
{
    ui->comboBoxEngineer->blockSignals(true);
    ui->comboBoxEngineer->setCurrentIndex(-1);
    ui->comboBoxEngineer->setPlaceholderText(usersModel->getDisplayRole(id));
    ui->comboBoxEngineer->blockSignals(false);
}

void SCartridgeForm::updateLists()
{
    QString recName;
    QString recSumm;
    int itemId = 0;
    int workType;
    QListWidget *list;
    QListWidgetItem *listItem;

    ui->listWidgetWorks->clear();
    ui->listWidgetParts->clear();

    for(int i = 0; i < m_BOQModel->rowCount(); i++)
    {
        recName = m_BOQModel->index(i, SStoreItemModel::SaleOpColumns::ColName).data().toString();
        recSumm = m_BOQModel->index(i, SStoreItemModel::SaleOpColumns::ColSumm).data().toString();
        listItem = new QListWidgetItem(QString("%1 %2%3").arg(recName).arg(recSumm).arg(sysLocale.currencySymbol()));    // TODO: мультивалютность
        if(m_BOQModel->recordType(i) == SSaleTableModel::RecordType::Work)
        {
            list = ui->listWidgetWorks;
            workType = m_BOQModel->index(i, SStoreItemModel::SaleOpColumns::ColWorkType).data().toInt();
            setWorkCheckBoxChecked(workType);
        }
        else
        {
            list = ui->listWidgetParts;
            itemId = m_BOQModel->index(i, SStoreItemModel::SaleOpColumns::ColItemId).data().toInt();
            listItem->setData(Qt::UserRole, itemId);    // для открытия карточки товара при двойном клике
        }

        list->addItem(listItem);
    }
}

bool SCartridgeForm::commit(const QString &notificationCaption, const QString &notificationText)
{
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connThird"));
    i_queryLog = new SQueryLog();
    bool nErr = 1;
    QUERY_LOG_START(metaObject()->className());
    try
    {
        QUERY_EXEC(query,nErr)(QUERY_BEGIN);
        m_repair->updateLastSave();
        nErr = m_repair->commit();
        shortlivedNotification *newPopup = new shortlivedNotification(this, notificationCaption, notificationText, QColor(214,239,220), QColor(229,245,234));

#ifdef QT_DEBUG
//        throw Global::ThrowType::Debug; // это для отладки (чтобы сессия всегда завершалась ROLLBACK'OM)
#endif
        QUERY_COMMIT_ROLLBACK(query,nErr);
    }
    catch (Global::ThrowType type)
    {
        nErr = 0;
        if(type == Global::ThrowType::Debug)
        {
            QString err = "DEBUG ROLLBACK";
            QUERY_ROLLBACK_MSG(query, err);
        }
        else if (type == Global::ThrowType::QueryError)
        {
            QUERY_COMMIT_ROLLBACK_MSG(query, nErr);
        }
        else
            QUERY_COMMIT_ROLLBACK(query, nErr);
    }
    QUERY_LOG_STOP;

    delete query;

    return nErr;
}

// Добавление работы и товара.
// Возвращает 0 в случае ошибки.
bool SCartridgeForm::addWorkAndPart(const int workType)
{
    int rowWork = m_BOQModel->rowCount();
    int rowItem = -1;
    int itemId = 0;
    SCartridgeMaterialModel *material;
    QSqlQuery query(QSqlDatabase::database("connThird"));

    material =  m_cartridgeCard->material((SWorkModel::Type)workType);
    if(material->articul()) // если артикул не задан, подразумевается работа без склада
    {
        query.exec(QUERY_SEL_CARTRIDGE_MATERIAL(material->articul(), material->count()));
        if(!query.first())
        {
            shortlivedNotification *newPopup = new shortlivedNotification(this,
                                                                          tr("Ошибка"),
                                                                          tr("Кол-во больше наличия, списание не возможно"),
                                                                          QColor(255,164,119),
                                                                          QColor(255,199,173));
            return 0;
        }

        itemId = query.value(0).toInt();
    }

    m_BOQModel->addCustomWork();
    m_BOQModel->setData(m_BOQModel->index(rowWork, SStoreItemModel::SaleOpColumns::ColName), material->workName());
    m_BOQModel->setData(m_BOQModel->index(rowWork, SStoreItemModel::SaleOpColumns::ColCount), 1);
    m_BOQModel->setData(m_BOQModel->index(rowWork, SStoreItemModel::SaleOpColumns::ColPrice), material->worksPrice());
    m_BOQModel->setData(m_BOQModel->index(rowWork, SStoreItemModel::SaleOpColumns::ColWorkType), workType);
//    m_BOQModel->setData(m_BOQModel->index(rowWork, SStoreItemModel::SaleOpColumns::ColSumm), material->worksPrice());

    rowItem = m_BOQModel->rowCount();
    if(material->articul())
    {
        m_BOQModel->addItemByUID(itemId, material->count());
#ifdef QT_DEBUG // в методе SSaleTableModel::insertRecord() для удобства отладки устанавливается случайное кол-во; здесь это не нужно
        m_BOQModel->setData(m_BOQModel->index(rowItem, SStoreItemModel::SaleOpColumns::ColCount), material->count());
#endif
        m_BOQModel->setData(m_BOQModel->index(rowItem, SStoreItemModel::SaleOpColumns::ColPrice), material->price()/material->count());
        //    m_BOQModel->setData(m_BOQModel->index(rowItem, SStoreItemModel::SaleOpColumns::ColSumm), material->count()*material->worksPrice());
    }

    return m_BOQModel->repair_saveTablesStandalone();
}

bool SCartridgeForm::removeWorkAndPart(const int workType)
{
    int nErr = 1;
    int row = 0;
    for(; row < m_BOQModel->rowCount(); row++)
    {
        if(m_BOQModel->index(row, SStoreItemModel::SaleOpColumns::ColWorkType).data().toInt() == workType)
            break;
    }
    m_BOQModel->removeRow(row);
    nErr = m_BOQModel->repair_saveTablesStandalone();

    return nErr;
}

bool SCartridgeForm::workAndPartHandler(const int workType, const int checkboxState)
{
    int ret;
    if(comSettings->useSimplifiedCartridgeRepair && m_repair->state() == Global::RepStateIds::GetIn)
        saveState(Global::RepStateIds::InWork);

    if(checkboxState)
    {
        ret = addWorkAndPart(workType);

        // при добавлении работ и деталей сигнал modelReset не эмитируется
        updateLists();
        updateWorksActionsCheckedState();
    }
    else
    {
        ret = removeWorkAndPart(workType);  // при удалении сигнал modelReset емитируется
    }
    return ret;
}

/* Запрос кол-ва предыдущих заправок
 * Презназначен для включения индикатора о превышении ресурса и индикатора о повторной заправке, если об этом не было указано при приёмке
 * Cм. описание метода SCartridgeCardModel::resource()
*/
int SCartridgeForm::prevRepairsCount()
{
    QSqlQuery *query = new QSqlQuery(QSqlDatabase::database("connMain"));

    query->exec(QUERY_SEL_CARTRIDGE_RESOURCE(m_repairId, m_serialNumber, SWorkModel::Type::CartridgeRefill));
    if(query->first())
    {
        m_prevRepairsCount = query->value(0).toInt();
    }

    delete query;

    return m_prevRepairsCount;
}

int SCartridgeForm::checkInput()
{
    int error = 0;
    setDefaultStyleSheets();

    if (ui->lineEditSerial->text() == "" && comSettings->isCartridgeSerialNumberRequired)
    {
        ui->lineEditSerial->setStyleSheet(commonLineEditStyleSheetRed);
        error = 1;
    }
//    if(ui->doubleSpinBoxPreagreedAmount->value() == 0)
//    {
//        ui->doubleSpinBoxPreagreedAmount->setStyleSheet(commonSpinBoxStyleSheetRed);
//        error = 2;
//    }
    if (error)
    {
        qDebug() << "Ошибка создания карточки заправки: не все обязательные поля заполнены (error " << error << ")";
        return error;
    }

    return false;
}

int SCartridgeForm::isReady()
{
    switch(m_repair->state())
    {
        case Global::RepStateIds::InWork: return (comSettings->useSimplifiedCartridgeRepair && m_BOQModel->rowCount());
        case Global::RepStateIds::Ready:
        case Global::RepStateIds::ReadyNoRepair: return 1;
    }

    return 0;
}

void SCartridgeForm::updateStatesModel(const int statusId)
{

    QString allowedStates;
    switch(statusId)
    {
        case Global::RepStateIds::GetIn: allowedStates = QString::number(Global::RepStateIds::InWork); break;
        case Global::RepStateIds::InWork: allowedStates = QString("%1|%2").arg(Global::RepStateIds::Ready).arg(Global::RepStateIds::ReadyNoRepair); break;
        case Global::RepStateIds::Ready: allowedStates = "---"; break;
        case Global::RepStateIds::ReadyNoRepair: allowedStates = "---"; break;
        default: allowedStates = "---";
    }
    statusesProxyModel->setFilterRegularExpression(QString("\\b(%1)\\b").arg(allowedStates));

    emit updateParentTab();
}

void SCartridgeForm::updateStateWidget(const int statusId)
{
    ui->comboBoxState->blockSignals(true);
    ui->comboBoxState->setCurrentIndex(-1);
    // QComboBox::setPlaceholderText(const QString&) https://bugreports.qt.io/browse/QTBUG-90595
    ui->comboBoxState->setPlaceholderText(comSettings->repairStatuses[statusId].Name);
    updateStatesModel(statusId);
    ui->comboBoxState->blockSignals(false);
}

void SCartridgeForm::updateTotalSumms(const double, const double, const double)
{
    ui->doubleSpinBoxTotalAmount->setValue(m_BOQModel->amountTotal());
}

void SCartridgeForm::saveTotalSumms()
{
    m_repair->setRealRepairCost(m_BOQModel->amountTotal());
    m_repair->setPartsCost(m_BOQModel->amountItems());
    m_repair->commit();
}

void SCartridgeForm::setRefill(bool state)
{
    SCartridgeMaterialModel *material = m_cartridgeCard->material(SCartridgeMaterialModel::Toner);
    if(material)
        updatePreagreedAmount(material, state);
}

void SCartridgeForm::setChipReplace(bool state)
{
    SCartridgeMaterialModel *material = m_cartridgeCard->material(SCartridgeMaterialModel::Chip);
    if(material)
        updatePreagreedAmount(material, state);
}

void SCartridgeForm::setDrumReplace(bool state)
{
    SCartridgeMaterialModel *material = m_cartridgeCard->material(SCartridgeMaterialModel::Drum);
    if(material)
        updatePreagreedAmount(material, state);
}

void SCartridgeForm::setBladeReplace(bool state)
{
    SCartridgeMaterialModel *material = m_cartridgeCard->material(SCartridgeMaterialModel::Blade);
    if(material)
        updatePreagreedAmount(material, state);
}

void SCartridgeForm::savePlace(int index)
{
    if(!m_repair)
        return;

    int currentPlace = m_repair->boxIndex();

    if(currentPlace == index)
        return;

    m_repair->setBoxIndex(index);
    if(!commit())
    {
        ui->comboBoxPlace->blockSignals(true);
        ui->comboBoxPlace->setCurrentIndex(currentPlace);
        ui->comboBoxPlace->blockSignals(false);
    }
}

void SCartridgeForm::comboBoxPlaceButtonClickHandler(int id)
{
    if(id == SLineEdit::Clear)
        ui->comboBoxPlace->setCurrentIndex(-1);
}

void SCartridgeForm::comboBoxEngineerChanged(int index)
{
    m_repair->setEngineerIndex(index);
    commit();
}

void SCartridgeForm::stateIndexChanged(int index)
{
    if (index < 0)
        return;

    saveState(statusesProxyModel->databaseIDByRow(index));
}

void SCartridgeForm::saveState(int stateId)
{
    if (stateId < 0)
        return;

    m_groupUpdate = 1;

    try
    {
        if(!checkStateAcl(stateId))
        {
            shortlivedNotification *newPopup = new shortlivedNotification(this, tr("Информация"), tr("Проверьте права доступа или обратитесь к администратору"), QColor(212,237,242), QColor(229,244,247));
            throw Global::ThrowType::ConditionsError;
        }
        checkData(stateId);
        doStateActions(stateId);
        m_repair->setState(stateId);

        if(!commit())
            throw Global::ThrowType::QueryError;
    }
    catch (Global::ThrowType type)
    {
        ui->comboBoxState->blockSignals(true);
        ui->comboBoxState->setCurrentIndex(-1);
        ui->comboBoxState->blockSignals(false);
        return;
    }
    updateWidgets();
    emit updateParentTab();
    tabRepairs::refreshIfTabExists();

    m_groupUpdate = 0;
}

void SCartridgeForm::removeWidget()
{
    updateComment();
    this->deleteLater();
}

void SCartridgeForm::buttonClientCardClicked()
{
    emit createTabClient(m_repair->clientId());
}

void SCartridgeForm::buttonClassicTabClicked()
{
    emit createTabRepair(m_repair->id());
    this->deleteLater();    // TODO: когда будет настроена синхронизация модели данных таблицы работ и деталей, тогда можно будет не удалять форму из списка
}

void SCartridgeForm::buttonCartridgeCardClicked()
{
    emit createCartridgeCardForm(m_cardId);
}

void SCartridgeForm::updateComment()
{
    if(!m_repairId)
        return;

    QString text = ui->lineEditComment->text();

    if(text.compare(m_repair->extNotes()) == 0)
        return;

    m_repair->setExtNotes(text);
    if(m_repair->commit())
        shortlivedNotification *newPopup = new shortlivedNotification(this, tr("Успешно"), tr("Примечание сохранено"), QColor(214,239,220), QColor(229,245,234));
}

void SCartridgeForm::initWorksMenu()
{
    QMenu *works_menu = new QMenu();
    QMetaEnum types = SWorkModel::staticMetaObject.enumerator(SWorkModel::staticMetaObject.indexOfEnumerator("Type"));
    SWorkModel::Type type;
    SCartridgeMaterialModel *material;

    for(int i = SWorkModel::Type::CartridgeReplaceOfWorn; i < types.keyCount(); i++)
    {
        type = (SWorkModel::Type)types.value(i);
        material = m_cartridgeCard->material(type);
        if(material)
        {
            QAction *item = new QAction(material->workName(), this);
            item->setCheckable(true);
            item->setProperty("WorkType", type);
            works_menu->addAction(item);
//            item->installEventFilter(this);
            connect(item, &QAction::triggered, [=](){workAndPartHandler(item->property("WorkType").toInt(), item->isChecked());});
        }
    }
    if(works_menu->actions().count())
    {
        ui->toolButtonOtherWorksMenu->setMenu(works_menu);

        updateWorksActionsCheckedState();
    }
    else
        ui->toolButtonOtherWorksMenu->setDisabled(true);
}

void SCartridgeForm::updateWorksMenu()
{
    if(!m_repairId)
        return;

    QMenu *menu = ui->toolButtonOtherWorksMenu->menu();

    initWorksMenu();

    if(menu)
        delete menu;
}

void SCartridgeForm::setWorkCheckBoxChecked(const int workType)
{
    switch(workType)
    {
        case SWorkModel::Type::CartridgeRefill: ui->pushButtonRefill->setChecked(true); break;
        case SWorkModel::Type::CartridgeDrumReplace: ui->pushButtonDrumReplace->setChecked(true); break;
        case SWorkModel::Type::CartridgeChipReplace: ui->pushButtonChipReplace->setChecked(true); break;
        case SWorkModel::Type::CartridgeBladeReplace: ui->pushButtonBladeReplace->setChecked(true); break;
        default: ;
    }
}

void SCartridgeForm::updateWorksActionsCheckedState()
{
    if(!ui->toolButtonOtherWorksMenu->menu())
        return;

    QList<QAction*> actions = ui->toolButtonOtherWorksMenu->menu()->actions();
    if(actions.isEmpty())
        return;

    QList<QAction*>::const_iterator action = actions.constBegin();
    while(action != actions.constEnd())
    {
        (*action)->setChecked(false);
        for(int row = 0; row < m_BOQModel->rowCount(); row++)
        {
            if(m_BOQModel->recordType(row) != SSaleTableModel::RecordType::Work)
                continue;

            if((*action)->property("WorkType").toInt() == m_BOQModel->workType(row))
            {
                (*action)->setChecked(true);
                return;
            }
        }
        action++;
    }
}

void SCartridgeForm::guiFontChanged()
{
    QFont font;
//    font.setFamily(userLocalData->FontFamily.value);
    font.setPixelSize(userDbData->fontSize);

    QFont font1(font);
    font1.setBold(true);
    font1.setWeight(75);

    QFont font2(font1);
    font2.setPixelSize(userDbData->fontSize+1);

    ui->labelTitle->setFont(font);
    ui->labelLimitReached->setFont(font1);
    ui->labelLimitReached->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 175, 176);\n"
                                                           "color: rgb(197, 255, 172);"));
    ui->labelLimitReached->setMargin(5);
    ui->lineEditSerial->setFont(font2);

}
