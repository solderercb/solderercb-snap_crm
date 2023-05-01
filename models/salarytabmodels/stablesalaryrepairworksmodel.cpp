#include "global.h"
#include "stablesalaryrepairworksmodel.h"

STableSalaryRepairWorksModel::STableSalaryRepairWorksModel(QObject *parent) : STableBaseModel(parent)
{

}

QVariant STableSalaryRepairWorksModel::data(const QModelIndex &item, int role) const
{
    if(role == Qt::DisplayRole)
    {
        switch (item.column())
        {
            case 3:
            case 4:
            case 7: return dataLocalizedFromDouble(item);
            case 5: return warrantyTermsModel->getDisplayRole(STableBaseModel::data(item).toInt(), 1);
            default: return STableBaseModel::data(item);
        }
    }
    return STableBaseModel::data(item, role);
}
