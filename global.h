#ifndef GLOBAL_H
#define GLOBAL_H

extern QVector<QSqlDatabase *> connections;    // массив указателей на соединения (для установки всем соединениям одинаковых параметров)
extern QMap<QString, QVariant> *userData;
extern QSqlQueryModel *userDataModel;
extern QMap<QString, QVariant> *userLocalData;
extern QMap<QString, bool> *permissions;
extern QMap<QString, QVariant> *comSettings;
extern QSqlQueryModel *companiesModel;
extern QSqlQueryModel *officesModel;
extern QSqlQueryModel *warehousesModel;
extern QSqlQueryModel *allUsersModel;
extern QMap<int, QString> *allUsersMap;
extern QSqlQueryModel *usersModel;
extern QSqlQueryModel *managersModel;
extern QSqlQueryModel *engineersModel;
extern QSqlQueryModel *itemBoxesModel;
extern QSqlQueryModel *repairBoxesModel;
extern QSqlQueryModel *paymentSystemsModel;
extern QStandardItemModel* clientsTypesList;
extern QSqlQueryModel* clientAdTypesList;
extern QStandardItemModel *statusesModel;
extern QStandardItemModel *notifyStatusesModel;
extern QStandardItemModel *warrantyTermsModel;
extern QMap<int, QString> *warrantyTermsMap;
extern QString commonComboBoxStyleSheet;

//extern QSqlQueryModel *Model;
//extern QSqlQueryModel *Model;

#endif // GLOBAL_H
