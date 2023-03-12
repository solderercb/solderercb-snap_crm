/*
 *
 */
#ifndef STABLEVIEWBASE_H
#define STABLEVIEWBASE_H

#include <QHeaderView>
#include <QRect>
#include <QScrollBar>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QSqlDriver>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>
#include "global.h"
#include "widgets/stableviewgridlayout.h"
#include "models/stablebasemodel.h"

typedef struct FilterList FilterList;

struct FilterField
{
    enum Op {Equals = 1<<0, Contains = 1<<1, StartsWith = 1<<2, EndsWith = 1<<3, RegExp = 1<<4, RegularExpression = 1<<5, Wildcard = 1<<6,
             NoOp = 1<<7, Null = 1<<8,
             NotMark = 1<<9 };
    QString column;
    Op operation;
    QVariant value;
    Qt::CaseSensitivity caseSensitivity;
};

struct FilterList
{
    enum Op {And, Or};
    QList<FilterField> fields;
    QList<FilterList> childs;
    bool op = And;
};

class STableViewBase : public QTableView
{
    Q_OBJECT
public:
    enum horizontalHeaderMenuActions{ToggleAutoWidth = 1, FitContent, BestFitAll, SetDefault, Hide, ColumnChooser};
    explicit STableViewBase(QWidget *parent = nullptr);
    ~STableViewBase();
    void resizeEvent(QResizeEvent*) override;
    void setModel(STableBaseModel *model);
    void setStoreItemsCategory(const int category);
    bool eventFilter(QObject *object, QEvent *event) override;

    // Часть кода взята из примера https://wiki.qt.io/Sort_and_Filter_a_QSqlQueryModel и доработана
    void setQuery(const QString &query, const QSqlDatabase &db = QSqlDatabase::database() );
    void setFilter(const FilterList &filter);
    void filter(const FilterList &filter);
    static FilterField initFilterField(const QString &column, FilterField::Op matchFlag, const QVariant &value, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);
    void setGrouping(const QStringList &grouping);
protected:
    int sizeHintForColumn(int column) const override;
    STableBaseModel *m_model = nullptr;
    QFontMetrics *m_fontMetrics;
    int m_modelRowCount = 0;
    int m_layoutVariant = SLocalSettings::RepairsGrid;
    XtraSerializer *i_gridLayout;
    QMap<int, int> i_defaultColumnsWidths;
    QList<int> i_defaultMarkedColumns;
    QStringList i_defaultHeaderLabels;
    QMap<int, int> m_autosizedColumns;
    int m_autosizedColumnsSummaryActualWidth = 0;
    int m_autosizedColumnsSummaryDefaultWidth = 0;
    QMenu *horizontalHeaderMenu = nullptr;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    void resizeColumnToContents(int column);
    int columnSizeByContents(int column);
    void resizeColumnsToContents();
    void applyGridlayout();
    void applySorting();
    void initAutosizedColumns();
    void adoptAutosizedColumns();
    void setColumnWidth(int column, int width);
    void setDefaultLayoutParams();
    void setDefaultColumnParams(const int column, const QString &label, const int width);
    void readLayout(SLocalSettings::SettingsVariant variant);
    void saveLayout(SLocalSettings::SettingsVariant variant);
    void initHorizontalHeaderMenu();
    void deleteHorizontalHeaderMenu();
private:
    int m_storeItemsCategory = 0;
    QFile m_layoutSettingsFileName;
    QSqlDatabase m_db;
    QString m_query;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    FilterList *m_filter = nullptr;
    QStringList *m_grouping = nullptr;
    void clearFilter();
    void clearGrouping();
    QString formatFilterGroup(const FilterList &filter);
    QString formatFilterField(const FilterField &field);
    int columnByName(const QString &name);
    void clearSorting();
    int visibleWidth();
public slots:
    void reset() override;
//    void applyLayoutForCategory(const int category);    // это для таблицы товаров, позже будет перенесено в наследующий класс
    void refresh();
protected slots:
    void columnResized(int column, int oldWidth, int newWidth);
private slots:
#if QT_VERSION >= 0x060000
    void dataChanged(const QModelIndex&, const QModelIndex&, const QList<int> &roles = QList<int>()) override;
#else
    void dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int> &roles = QVector<int>()) override;
#endif
    void orderChanged(int logicalIndex, Qt::SortOrder order);
    void horizontalHeaderMenuRequest(const QPoint &pos) const;
    void toggleAutoWidth(bool state);
    void fitContent();
    void bestFitAll();
    void setDefault();
    void hideColumn();
    void showColumnChooser();
};

#endif // STABLEVIEWBASE_H