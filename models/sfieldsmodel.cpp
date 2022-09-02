#include "sfieldsmodel.h"

SFieldsModel::SFieldsModel(Type type, QObject *parent) :
    QObject(parent),
    m_isRepair(type)
{
    query = new QSqlQuery(QSqlDatabase::database("connMain"));
}

SFieldsModel::~SFieldsModel()
{
    qDebug().nospace() << "[SFieldsModel] ~SFieldsModel() | In";
    delete query;
    clear();
    qDebug().nospace() << "[SFieldsModel] ~SFieldsModel() | all widgets and fields deleted successfully";
}

void SFieldsModel::clear()
{
    qDebug().nospace() << "[SFieldsModel] deleteWidgets() | In";
    QWidget *w;
    SFieldValueModel *f;

    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    while(!widgetFieldMap.isEmpty())
    {
        i = widgetFieldMap.constBegin();
        w = i.key();
        f = i.value();
        qDebug().nospace() << "[SFieldsModel] deleteWidgets() | Step x.0";
        if(w->property("fieldType") == WidgetType::ComboBox)
        {
            QComboBox *cb = static_cast<QComboBox*>(w);
            QAbstractItemModel* widgetModel;
            widgetModel = cb->model();
            qDebug().nospace() << "[SFieldsModel] deleteWidgets() | Step x.1a";
            if(widgetModel)
                delete widgetModel;
        }
        qDebug().nospace() << "[SFieldsModel] deleteWidgets() | Step x.1";
        widgetFieldMap.remove(w);
        qDebug().nospace() << "[SFieldsModel] deleteWidgets() | Step x.2";
        delete w;
        delete f;
    }
    qDebug().nospace() << "[SFieldsModel] deleteWidgets() | Out";
}

QList<SFieldValueModel*> SFieldsModel::list()
{
    return widgetFieldMap.values();
}

QList<QWidget *> SFieldsModel::widgetsList()
{
    return widgetFieldMap.keys();
}

/*  Инициализация массива виджетов для нового товара или ремонта
 *  isRepair == 1 — инициализация виджетов для класса <id> устройств,
 *  иначе для <id> категории товаров
 */
bool SFieldsModel::initWidgets(const int id)
{
    qDebug().nospace() << "[SFieldsModel] initWidgets() | In";
    if(id == 0)
        return 0;

    qDebug().nospace() << "[SFieldsModel] initWidgets() | step 0";
    if( !widgetFieldMap.isEmpty() )
    {
        clear();
    }

    qDebug().nospace() << "[SFieldsModel] initWidgets() | step 1";
    QSizePolicy *sizePolicy = new QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QWidget *widget;
    SFieldValueModel *field;
    int type = 0;

    qDebug().nospace() << "[SFieldsModel] initWidgets() | step 2";
    query->exec(QUERY_SEL_ADDITIONAL_FIELDS_TYPES((m_isRepair?1:0), id));
    while(query->next())
    {
        type = query->value(2).toInt();
        field = new SFieldValueModel();

        switch (type)
        {
            case WidgetType::LineEdit: widget = createLineEdit(query->record(), field); break;
            case WidgetType::ComboBox: widget = createComboBox(query->record(), field); break;
            case WidgetType::DateEdit: widget = createDateTime(query->record(), field); break;
            default: widget = createDummyWidget(query->record(), field);
        }
        qDebug().nospace() << "[SFieldsModel] initWidgets() | step 3.x";
        widget->setSizePolicy(*sizePolicy);
        widgetFieldMap.insert(widget, field);
        field->setFieldId(query->value(3).toInt());
        widget->setProperty("fieldId", query->value(3).toInt());
        widget->setProperty("fieldType", type);
        widget->setProperty("fieldRequired", query->value(4).toBool());
        field->setProperty("fieldRequired", query->value(4).toBool());
        widget->setProperty("fieldPrintable", query->value(5).toBool());
    }
    delete sizePolicy;
    qDebug().nospace() << "[SFieldsModel] initWidgets() | Out";
    return 1;
}

QWidget *SFieldsModel::createLineEdit(const QSqlRecord &record, SFieldValueModel *field)
{
    QLineEdit *widget = new QLineEdit();
    widget->setPlaceholderText(record.value(0).toString());
    if(!record.value(1).toString().isEmpty())
    {
        // TODO: добавить Input mask для LineEdit
        widget->setText(record.value(1).toString());
        field->setValue(widget->text());
    }
    return widget;
}

QWidget *SFieldsModel::createComboBox(const QSqlRecord &record, SFieldValueModel *field)
{
    QComboBox *widget = new QComboBox();
    QStandardItemModel* widgetModel = new QStandardItemModel();
    QStringList widgetItems = record.value(1).toString().split('\n');
    QStandardItem *newRow;
    QFontMetrics *fm = new QFontMetrics(widget->font());
    int itemTextWidth, dropDownListWidth = 0;

    widget->setModel(widgetModel);
//            if (widgetItems.at(0) != "")   // Принудительно добавляем пустую строку
//            {
//                newRow = new QStandardItem();
//                newRow->setText("");
//                widgetModel->appendRow(newRow);
//            }
    for (int i=0; i<widgetItems.size(); i++)
    {
        newRow = new QStandardItem();
        newRow->setText(widgetItems.at(i));
        widgetModel->appendRow(newRow);

        // определяем наибольшую длину текста в списке элементов
        itemTextWidth = fm->size(Qt::TextSingleLine, widgetItems.at(i)).width() + 10;
        if (itemTextWidth > dropDownListWidth)
            dropDownListWidth = itemTextWidth;
    }
    delete fm;  // больше не нужен
    widget->setMinimumWidth(100);
    widget->view()->setMinimumWidth(dropDownListWidth);
    widget->setCurrentIndex(-1);
    widget->setEditable(true);
    // QComboBox::setPlaceholderText(const QString&) https://bugreports.qt.io/browse/QTBUG-90595
    widget->lineEdit()->setPlaceholderText(record.value(0).toString());

    return widget;
}

QWidget *SFieldsModel::createDateTime(const QSqlRecord &record, SFieldValueModel *field)
{
    QDateEdit *widget = new QDateEdit();
    if(record.value(1).toString().isEmpty())
    {
        widget->setDate(QDate::currentDate());
    }
    else
    {
        widget->setDate(record.value(1).toDate());
    }
    field->setValue(widget->text());
    widget->setCalendarPopup(true);

    return widget;
}

QWidget *SFieldsModel::createDummyWidget(const QSqlRecord &record, SFieldValueModel *field)
{
    // TODO: В АСЦ не реализовано, поэтому используем LineEdit
    QLineEdit *widget = new QLineEdit();
    widget->setPlaceholderText(record.value(0).toString());
    widget->setEnabled(false);

    return widget;
}

bool SFieldsModel::load(int id)
{
    if(m_repair == 0 && m_item == 0)
        return 0;

    if(m_repair)
        query->exec(QUERY_SEL_REPAIR_ADD_FIELDS(id));
    else
        query->exec(QUERY_SEL_ITEM_ADD_FIELDS(id));

    while(query->next())
    {
        itemHandler(query->record());
    }

    delete query;
    return 1;
}

void SFieldsModel::add(SFieldValueModel *item)
{
    QWidget *w = nullptr;
    widgetFieldMap.insert(w, item);
}

void SFieldsModel::remove(SFieldValueModel *item)
{
    m_removeList.append(item);
    QWidget *w = widgetFieldMap.key(item);
    widgetFieldMap.remove(w);
}

bool SFieldsModel::isEmpty()
{
    return widgetFieldMap.isEmpty();
}

void SFieldsModel::setRepair(const int id)
{
    m_repair = id;
    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    for (i = widgetFieldMap.constBegin(); i != widgetFieldMap.constEnd(); ++i)
    {
        i.value()->setRepairId(id);
    }
}

void SFieldsModel::setItem(const int id)
{
    m_item = id;
    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    for (i = widgetFieldMap.constBegin(); i != widgetFieldMap.constEnd(); ++i)
    {
        i.value()->setItemId(id);
    }
}

bool SFieldsModel::commit()
{
    SFieldValueModel *item;
    QWidget *widget;
    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    for (i = widgetFieldMap.constBegin(); i != widgetFieldMap.constEnd(); ++i)
    {
        widget = i.key();
        item = i.value();
        item->setFieldId(widget->property("fieldId").toInt());
        switch (widget->property("fieldType").toInt())
        {
            case WidgetType::LineEdit: item->setValue(static_cast<QLineEdit*>(widget)->text()); break;
            case WidgetType::ComboBox: item->setValue(static_cast<QComboBox*>(widget)->currentText()); break;
            case WidgetType::DateEdit: item->setValue(static_cast<QDateEdit*>(widget)->date().toString("yyyy-MM-dd")); break;
            default: item->setValue(static_cast<QLineEdit*>(widget)->text());
        }

        if(!item->value().isEmpty())
        {
            if(!item->commit())
                throw 1;
        }
    }

    while( !m_removeList.isEmpty() )
    {
        item = m_removeList.last();
        if(!item->delDBRecord())
            throw 1;

        m_removeList.removeLast();
        item->deleteLater();
    }

    return 1;
}

bool SFieldsModel::validate()
{
    QWidget *w;
    bool ret = 1;
    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    for (i = widgetFieldMap.constBegin(); i != widgetFieldMap.constEnd(); ++i)
    {
        w = i.key();
        if(w->property("fieldRequired").toBool())
        {
            switch (w->property("fieldType").toInt())
            {
                case WidgetType::LineEdit: if(static_cast<QLineEdit*>(w)->text().isEmpty()) {w->setStyleSheet(commonLineEditStyleSheetRed); ret = 0;}; break;
                case WidgetType::ComboBox: if(static_cast<QComboBox*>(w)->currentIndex() == -1) {w->setStyleSheet(commonComboBoxStyleSheetRed); ret = 0;}; break;
//                case WidgetType::DateEdit: if(static_cast<QDateEdit*>(w)->method()) {w->setStyleSheet(commonDateEditStyleSheetRed); ret = 0;}; break; // TODO: придумать критерии проверки DateEdit
                default: if(static_cast<QLineEdit*>(w)->text().isEmpty()) {w->setStyleSheet(commonLineEditStyleSheetRed); ret = 0;};
            }
        }
    }
    return ret;
}

void SFieldsModel::resetIds()
{
    SFieldValueModel *item;
    QMap<QWidget*, SFieldValueModel*>::ConstIterator i;
    for (i = widgetFieldMap.constBegin(); i != widgetFieldMap.constEnd(); ++i)
    {
        item = i.value();
        if(!item->value().isEmpty())
            item->setId(0);
    }
}

SFieldValueModel *SFieldsModel::itemHandler(const QSqlRecord &record)
{
    SFieldValueModel *item = new SFieldValueModel(this);
    item->load(record);
    add(item);
    return item;
}
