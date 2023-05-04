#include "tabsalary.h"
#include "ui_tabsalary.h"
#include "global.h"

tabSalary* tabSalary::p_instance = nullptr;

tabSalary::tabSalary(MainWindow *parent) :
    tabCommon(parent),
    ui(new Ui::tabSalary)
{
    // Игициализация моделей до UI
    m_userModel = new SUserModel();
    m_repairs = new STableSalaryRepairsModel();
    m_repairWorks = new STableSalaryRepairWorksModel();
    m_repairParts = new STableSalaryRepairPartsModel();
    m_sales = new STableSalarySalesModel();
    m_saleParts = new STableSalarySalePartsModel();
    m_extraCharges = new STableSalaryExtraModel();
    m_items = new STableSalaryItemsModel();
    m_payments = new STableSalaryPaymentsModel();
    m_recepted = new STableSalaryReceptedIssued();
    m_issued = new STableSalaryReceptedIssued();

    ui->setupUi(this);
    usersModelF = new SSortFilterProxyModel();
    usersModelF->setSourceModel(usersModel);
    usersModelF->setFilterRegularExpression(QRegularExpression("^(?!(admin)).*$")); // без админа
    usersModelF->setFilterKeyColumn(0);
    ui->comboBoxEmployee->setModel(usersModelF);
    ui->comboBoxEmployee->setCurrentIndex(-1);

    ui->dateEditPeriod->setDate(QDate::currentDate());
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &tabSalary::tabChanged);

#ifdef QT_DEBUG
    ui->comboBoxEmployee->setCurrentIndex(0);
    m_periodBegin.setDate(QDate(2022,8,01));
    m_periodEnd.setDate(QDate(2022,9,1));
    loadButtonPressed();
#endif
    updateWidgets();
}

tabSalary *tabSalary::getInstance(MainWindow *parent)
{
    if( !p_instance )
      p_instance = new tabSalary(parent);
    return p_instance;
}

tabSalary::~tabSalary()
{
    delete ui;
    delete m_repairs;
    delete m_repairWorks;
    delete m_repairParts;
    delete m_sales;
    delete m_saleParts;
    delete m_extraCharges;
    delete m_items;
    delete m_payments;
    delete m_recepted;
    delete m_issued;
    delete m_userModel;
    p_instance = nullptr;
}

QString tabSalary::tabTitle()
{
    return "Заработная плата";
}

QString tabSalary::periodBegin()
{
    return m_periodBegin.toUTC().toString("yyyy-MM-dd hh:mm:ss");
}

QString tabSalary::periodEnd()
{
    return m_periodEnd.toUTC().toString("yyyy-MM-dd hh:mm:ss");
}

void tabSalary::updateModels()
{
    m_modelUpdatedFlags = 0;
    m_userModel->load(m_employeeId);
    emit updateDaughterTabsModels();
}

void tabSalary::modelsUpdated()
{
    qDebug().nospace() << "[" << this << "] modelsUpdated()";
}

void tabSalary::loadButtonPressed()
{
    if(ui->comboBoxEmployee->currentIndex() == -1)
    {
        ui->comboBoxEmployee->setStyleSheet(commonComboBoxStyleSheetRed);
        return;
    }

    ui->comboBoxEmployee->setStyleSheet(commonComboBoxStyleSheet);

    m_employeeId = usersModelF->databaseIDByRow(ui->comboBoxEmployee->currentIndex());

    updateModels();
}

void tabSalary::periodDateChanged(const QDate date)
{
    m_showSubsistanceGroup = 1;
    if(date.month() != QDate::currentDate().month())
        m_showSubsistanceGroup = 0;
    if(comSettings->value("classic_salary").toBool())
    {
        m_periodBegin.setDate(date.addDays(-date.day() + 1));
        m_periodEnd = m_periodBegin.addDays(date.daysInMonth());
    }
    else
    {
        m_periodBegin.setDate(date.addDays(-date.dayOfWeek() + 1));
        m_periodEnd = m_periodBegin.addDays(7);
    }
    updateWidgets();
}

void tabSalary::tabChanged(const int index)
{
    bool state = index==2;
    ui->buttonAddExtraCharge->setVisible(state);
    ui->buttonSaveExtraChargesList->setVisible(state);
}

void tabSalary::updateWidgets()
{
    ui->labelPeriod->setText(m_periodBegin.toString("dd.MM.yyyy") + " − " + m_periodEnd.addDays(-1).toString("dd.MM.yyyy"));
}

void tabSalary::setModelUpdatedFlag(const int pos)
{
    m_modelUpdatedFlags |= 1<<pos;
    if(m_modelUpdatedFlags == (
        1 << SPageSalaryBase::UserModel |
        1 << SPageSalaryBase::RepairsModel |
        1 << SPageSalaryBase::ExtraChargesModel |
        1 << SPageSalaryBase::SalesModel |
        1 << SPageSalaryBase::ItemsModel |
        1 << SPageSalaryBase::PaymentsModel |
        1 << SPageSalaryBase::ReceptedModel |
        1 << SPageSalaryBase::IssuedModel))
    {
        modelsUpdated();
        emit updateDaughterTabsWidgets();
    }
}
