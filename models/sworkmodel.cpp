#include "sworkmodel.h"
#include "ssaletablemodel.h"

SWorkModel::SWorkModel(QObject *parent) : SComRecord(parent)
{
    i_tableName = "works";
    i_obligatoryFields << "user" << "name" << "warranty";
    i_idColumnName = "id";
    i_logRecord->setType(SLogRecordModel::Repair);
}

SWorkModel::SWorkModel(const QList<QStandardItem *> &record, QObject *parent) :
    SWorkModel(parent)
{
    i_id = record.at(SStoreItemModel::SaleOpColumns::ColId)->data(Qt::DisplayRole).toInt();

    m_user = record.at(SStoreItemModel::SaleOpColumns::ColUser)->data(Qt::DisplayRole).toInt();
    m_repair = record.at(SStoreItemModel::SaleOpColumns::ColObjId)->data(Qt::DisplayRole).toInt();
    m_name = record.at(SStoreItemModel::SaleOpColumns::ColName)->data(Qt::DisplayRole).toString();
    m_price = record.at(SStoreItemModel::SaleOpColumns::ColPrice)->data(Qt::DisplayRole).toDouble();
    m_count = record.at(SStoreItemModel::SaleOpColumns::ColCount)->data(Qt::DisplayRole).toInt();
    m_warranty = record.at(SStoreItemModel::SaleOpColumns::ColWarranty)->data(Qt::DisplayRole).toInt();
    m_priceId = record.at(SStoreItemModel::SaleOpColumns::ColItemId)->data(Qt::DisplayRole).toInt();
    i_createdUtc = record.at(SStoreItemModel::SaleOpColumns::ColCreated)->data(Qt::DisplayRole).toDateTime();
//    m_documentId = record.at(SStoreItemModel::SaleOpColumns::ColObjId)->data(Qt::DisplayRole).toInt();
//    m_isPay = record.at(SStoreItemModel::SaleOpColumns::)->data(Qt::DisplayRole).toBool();
    m_type = record.at(SStoreItemModel::SaleOpColumns::ColWorkType)->data(Qt::DisplayRole).toInt();
    i_logRecord->setRepairId(m_repair);

    if(!i_id)
        appendLogText(tr("Добавлена работа \"%1\" стоимостью %2").arg(m_name, sysLocale.toCurrencyString(m_price)));

    initQueryFields(record);
}

int SWorkModel::id()
{
    return i_id;
}

void SWorkModel::load(const int)
{

}

int SWorkModel::user()
{
    return m_user;
}

void SWorkModel::setUser(const int id, const QVariant oldValue)
{
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText;

    if(oldValue.isValid())
        logText = tr("Исполнитель работы \"%1\" изменён с %2 на %3").arg(m_name, usersModel->getDisplayRole(oldValue.toInt()), usersModel->getDisplayRole(id));
    else
        logText = tr("Исполнителем работы \"%1\" назначен %2").arg(m_name, usersModel->getDisplayRole(id, 1));

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    appendLogText(logText);
    i_valuesMap.insert("user", id);
    setPayRepair(usersSalaryTaxesModel->value(id, "id", "pay_repair").toInt());
    setPayRepairQuick(usersSalaryTaxesModel->value(id, "id", "pay_repair_quick").toInt());
}

int SWorkModel::repair()
{
    return m_repair;
}

void SWorkModel::setRepair(const int repair)
{
    i_valuesMap.insert("repair", repair);
}

int SWorkModel::documentId()
{
    return m_documentId;
}

void SWorkModel::setDocumentId(const int document_id)
{
    i_valuesMap.insert("document_id", document_id);
}

QString SWorkModel::name()
{
    return m_name;
}

void SWorkModel::setName(const QString name, const QVariant oldValue)
{
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText;

    if(!oldValue.toString().isEmpty())
        logText = tr("Название работы изменено с \"%1\" на \"%2\"").arg(oldValue.toString(), name);
    else if(i_id)   // для новой произвольной работы или работы из прайс-листа запись в журнал не нужна
        logText = tr("Название работы изменено на \"%1\"").arg(name);

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    appendLogText(logText);
    i_valuesMap.insert("name", name);
}

double SWorkModel::price()
{
    return m_price;
}

void SWorkModel::setPrice(const double price, const QVariant oldValue)
{
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText;

    if(oldValue.isValid())
        logText = tr("Стоимость работы \"%1\" изменёна с %2 на %3").arg(m_name, sysLocale.toCurrencyString(oldValue.toDouble()), sysLocale.toCurrencyString(price));
    else if(i_id)
        logText = tr("Стоимость работы \"%1\" установлена %1").arg(m_name, sysLocale.toCurrencyString(price));

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    appendLogText(logText);
    i_valuesMap.insert("price", price);
}

int SWorkModel::count()
{
    return m_count;
}

void SWorkModel::setCount(const int count, const QVariant oldValue)
{
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText = tr("Кол-во \"%1\" изменёно с %2 на %3").arg(m_name).arg(oldValue.toInt()).arg(count);

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    if(oldValue.isValid())
        appendLogText(logText);

    i_valuesMap.insert("count", count);
}

int SWorkModel::warranty()
{
    return m_warranty;
}

void SWorkModel::setWarranty(const int warranty, const QVariant oldValue)
{
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText = tr("Удален товар \"%1\" стоимостью %2 в кол-ве %3ед.").arg(m_name, sysLocale.toCurrencyString(m_price)).arg(m_count);

    if(!oldValue.toString().isEmpty())
        logText = tr("Срок гарантии на работу \"%1\" изменён с \"%2\" на \"%3\"").arg(m_name, warrantyTermsModel->getDisplayRole(oldValue.toInt(), 1), warrantyTermsModel->getDisplayRole(warranty, 1));
    else if(i_id || (!i_id && warranty != 0) )   // первое изменение срока гарантии при автосохранении списка или установка срока гарантии до первого сохранения списка
        logText = tr("Срок гарантии на работу \"%1\" установлен \"%2\"").arg(m_name, warrantyTermsModel->getDisplayRole(warranty, 1));
    else
        logText = tr("Срок гарантии на работу \"%1\" установлен по умолчанию (\"%2\")").arg(m_name, warrantyTermsModel->getDisplayRole(warranty, 1));

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    appendLogText(logText);
    i_valuesMap.insert("warranty", warranty);
}

int SWorkModel::priceId()
{
    return m_priceId;
}

void SWorkModel::setPriceId(const int id)
{
    if(id)
        i_valuesMap.insert("price_id", id);
    else
        i_valuesMap.insert("price_id", QVariant());
}

bool SWorkModel::isPay()
{
    return m_isPay;
}

void SWorkModel::setIsPay(const bool is_pay)
{
    i_valuesMap.insert("is_pay", is_pay);
}

void SWorkModel::setCreated(const QDateTime added)
{
    i_valuesMap.insert("added", added);
}

int SWorkModel::type()
{
    return m_type;
}

void SWorkModel::setType(const int type)
{
    i_valuesMap.insert("type", type);
}

int SWorkModel::payRepair()
{
    return m_payRepair;
}

void SWorkModel::setPayRepair(const int pay_repair)
{
    i_valuesMap.insert("pay_repair", pay_repair?pay_repair:QVariant());
}

int SWorkModel::payRepairQuick()
{
    return m_payRepair_quick;
}

void SWorkModel::setPayRepairQuick(const int pay_repair_quick)
{
    i_valuesMap.insert("pay_repair_quick", pay_repair_quick?pay_repair_quick:QVariant());
}

bool SWorkModel::remove()
{
    bool ret = 1;
    SSaleTableModel *model = static_cast<SSaleTableModel*>(parent());
    QString logText = tr("Удалена работа \"%1\" стоимостью %2").arg(m_name, sysLocale.toCurrencyString(m_price));

    if(model->state() == SSaleTableModel::State::WorkshopAdm && !logText.isEmpty())
        logText.prepend("[A] ");

    i_logRecord->setText(logText);
    ret &= del();
    ret &= i_logRecord->commit();

    return ret;
}

void SWorkModel::setQueryField(const int fieldNum, const QVariant value, const QVariant oldValue)
{
    switch(fieldNum)
    {
        case SStoreItemModel::ColUser: setUser(value.toInt(), oldValue); break;
        case SStoreItemModel::ColObjId: setRepair(value.toInt()); break;
        case SStoreItemModel::ColName: setName(value.toString(), oldValue); break;
        case SStoreItemModel::ColPrice: setPrice(value.toDouble(), oldValue); break;
        case SStoreItemModel::ColCount: setCount(value.toInt(), oldValue); break;
        case SStoreItemModel::ColWarranty: setWarranty(value.toInt(), oldValue); break;
        case SStoreItemModel::ColItemId: setPriceId(value.toInt()); break;
        case SStoreItemModel::ColCreated: setCreated(value.toDateTime()); break;
        case SStoreItemModel::ColWorkType: setType(value.toInt()); break;
    }
}

bool SWorkModel::commit()
{
    if(i_id)
    {
        update();
    }
    else
    {
        setCreated(QDateTime::currentDateTime());
        insert();
    }
    commitLogs();

    return i_nErr;
}

double SWorkModel::salarySumm() const
{
    return m_salarySumm;
}

void SWorkModel::setSalarySumm(double salarySumm)
{
    m_salarySumm = salarySumm;
    i_valuesMap.insert("salary_summ", salarySumm);
}

