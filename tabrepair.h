#ifndef TABREPAIR_H
#define TABREPAIR_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QToolButton>
#include <QStyle>
#include <QTableWidget>
#include <QLabel>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlField>
#include <QScrollBar>
#include <QTimeZone>
#include <QDateTime>
#include <QLocale>
#include "tabcommon.h"
#include "getOutDialog.h"

namespace Ui {
class tabRepair;
}

class commentsTable : public QTableView
{
    Q_OBJECT

public:
    explicit commentsTable(QWidget *parent = nullptr);
    ~commentsTable();
    void resizeEvent(QResizeEvent*);
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>());
private:
};

class commentsDataModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    explicit commentsDataModel(QWidget *parent = nullptr);
    ~commentsDataModel();
    void update();
private:
    QVariant data(const QModelIndex &index, int role) const;
};

class worksAndSparePartsTable : public QTableView
{
    Q_OBJECT

public:
    explicit worksAndSparePartsTable(QWidget *parent = nullptr);
    ~worksAndSparePartsTable();
    void resizeEvent(QResizeEvent*);
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>());
private:
};

class worksAndSparePartsDataModel : public QSqlQueryModel
{
    Q_OBJECT

public:
    enum {actions, name, qty, price, summ, warranty, user, sn, id, work_id, item_rsrv_id};
    explicit worksAndSparePartsDataModel(QWidget *parent = nullptr);
    ~worksAndSparePartsDataModel();
    void update();
private:
    QVariant data(const QModelIndex &index, int role) const;
};

class tabRepair : public tabCommon
{
    Q_OBJECT

    friend class getOutDialog;

signals:
    void worksTreeDoubleClicked(int);

public:
    explicit tabRepair(int rep_id, MainWindow *parent = nullptr);
    static tabRepair* getInstance(int rep_id, MainWindow *parent = nullptr);
    ~tabRepair();

private:
    Ui::tabRepair *ui;
    static QMap<int, tabRepair*> p_instance;
    int repair_id;
    QSqlQueryModel *repairModel;
    QSqlQueryModel *clientModel;
    QSqlQueryModel *fieldsModel;
    QSortFilterProxyModel *statusesProxyModel;
    commentsDataModel *commentsModel;
    worksAndSparePartsDataModel *worksAndPartsModel;
    bool statusUpdateFlag;
    float works_sum, parts_sum, total_sum;
    int getFieldIdByName(const QString &, QSqlQueryModel *);
    QString getDisplayRoleById(int, QAbstractItemModel*, int column = 0);
    void eventResize(QResizeEvent *);
    void addItemToListViewExtraInfo(QString, QString);
    void setLock(bool);
    QString box_name;

    getOutDialog *modalWidget;
    QWidget *overlay;
    bool save_state_on_close = 0;
    bool worksAndPartsEditEnabled = 0;

private slots:
    void updateRepairData();
    void updateWidgets();
    void worksTreeDoubleClicked(QModelIndex);
    void saveStatus();
    void saveStatus(int);
    void comboBoxIndexChanged(int);
    void updateTotalSumms();
    void createGetOutDialog();
    void closeGetOutDialog();

};

#endif // TABREPAIR_H
