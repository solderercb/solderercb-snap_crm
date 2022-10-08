#ifndef SREPAIRMODEL_H
#define SREPAIRMODEL_H

#include "scomrecord.h"
#include <QObject>
#include <QString>
#include <QDate>
#include "sdevmdlmodel.h"
#include "sclientmodel.h"
#include "saletablemodel.h"

class SRepairModel : public SComRecord
{
    Q_OBJECT
public:
    explicit SRepairModel(QObject *parent = nullptr);
    explicit SRepairModel(const int, QObject *parent = nullptr);
    ~SRepairModel();
    int id();
    void setId(const int);
    void load(const int);
    bool isHidden();
    void setHidden(const bool);
    QString title();
    void setTitle(const QString);
    int clientId();
    void setClientId(const int);
    SClientModel *clientModel();
    int classId();
    void setClassId(const int);
    int vendorId();
    void setVendorId(const int);
    int deviceId();
    void setDeviceId(const int);
    QString serialNumber();
    void setSerialNumber(const QString);
    int companyIndex();
    void setCompanyIndex(const int);
    int officeIndex();
    void setOffice(const int);
    void setOfficeIndex(const int);
    int startOfficeIndex();
    void setStartOffice(const int);
    void setStartOfficeIndex(const int);
    int managerIndex();
    void setManager(const int);
    void setManagerIndex(const int);
    int currentManagerIndex();
    void setCurrentManager(const int);
    void setCurrentManagerIndex(const int);
    int masterIndex();
    void setMasterIndex(const int);
    QString diagnosticResult();
    void setDiagnosticResult(const QString);
    QDateTime inDate();
    void setInDate(const QDateTime);
    QDateTime outDate();
    void setOutDate(const QDateTime);
    int stateIndex();
    void setStateIndex(const int);
    int newStateIndex();
    void setNewStateIndex(const int);
    int userLock();
    void setUserLock(const int);
    QDateTime lockDatetime();
    void setLockDatetime(const QDateTime);
    bool expressRepair();
    void setExpressRepair(const bool);
    bool quickRepair();
    void setQuickRepair(const bool);
    bool isWarranty();
    void setIsWarranty(const bool);
    bool isRepeat();
    void setIsRepeat(const bool);
    int paymentSystemIndex();
    void setPaymentSystem(const int);
    void setPaymentSystemIndex(const int);
    bool isCardPayment();
    void setIsCardPayment(const bool);
    bool canFormat();
    void setCanFormat(const bool);
    bool printCheck();
    void setPrintCheck(const bool);
    int boxIndex();
    void setBoxIndex(const int);
    QString warrantyLabel();
    void setWarrantyLabel(const QString);
    QString extNotes();
    void setExtNotes(const QString);
    bool isPrepaid();
    void setIsPrepaid(const bool state = true);
    int prepaidType();
    void setPrepaidType(const int);
    float prepaidSumm();
    void setPrepaidSumm(const float);
    int prepaidOrder();
    void setPrepaidOrder(const int);
    void addPrepay(float amount, QString reason = QString());
    bool isPreAgreed();
    void setIsPreAgreed(const bool);
    bool isDebt();
    void setIsDebt(const bool);
    float preAgreedAmount();
    void setPreAgreedAmount(const float);
    float repairCost();
    void setRepairCost(const float);
    float realRepairCost();
    void setRealRepairCost(const float);
    float partsCost();
    void setPartsCost(const float);
    QString fault();
    void setFault(const QString);
    QString complect();
    void setComplect(const QString);
    QString look();
    void setLook(const QString);
    bool thirsPartySc();
    void setThirsPartySc(const bool);
    QDateTime lastSave();
    void setLastSave(const QDateTime);
    QDateTime lastStatusChanged();
    void setLastStatusChanged(const QDateTime);
    int warrantyDays();
    void setWarrantyDays(const int);
    QString barcode();
    void setBarcode(const QString);
    QString rejectReason();
    void setRejectReason(const QString);
    int informedStatusIndex();
    void setInformedStatusIndex(const int);
    QString imageIds();
    void setImageIds(const QString);
    QColor color();
    void setColor(const QColor);
    QString orderMoving();
    void setOrderMoving(const QString);
    int early();
    void setEarly(const int);
    QString extEarly();
    void setExtEarly(const QString);
    QString issuedMsg();
    void setIssuedMsg(const QString);
    bool smsInform();
    void setSmsInform(const bool);
    int invoice();
    void setInvoice(const int);
    int cartridge();
    void setCartridge(const int);
    bool termsControl();
    void setTermsControl(const bool);
    bool commit();
    bool lock(bool state = 1);

private:
    SClientModel *m_clientModel;
    bool m_isHidden;
    QString m_title;
    int m_clientId;
    int m_type;
    int m_maker;
    int m_model;
    QString m_serialNumber;
    int m_company;
    int m_office;
    int m_startOffice;
    int m_manager;
    int m_currentManager;
    int m_master;
    QString m_diagnosticResult;
    QDateTime m_inDate;
    QDateTime m_outDate;
    int m_state;
    int m_newState;
    int m_userLock;
    QDateTime m_lockDatetime;
    bool m_expressRepair;
    bool m_quickRepair;
    bool m_isWarranty;
    bool m_isRepeat;
    int m_paymentSystem;
    bool m_isCardPayment;
    bool m_canFormat;
    bool m_printCheck;
    int m_box;
    QString m_warrantyLabel;
    QString m_extNotes;
    bool m_isPrepaid;
    int m_prepaidType;
    float m_prepaidSumm = 0;
    int m_prepaidOrder;
    bool m_isPreAgreed;
    bool m_isDebt;
    float m_preAgreedAmount;
    float m_repairCost;
    float m_realRepairCost;
    float m_partsCost;
    QString m_fault;
    QString m_complect;
    QString m_look;
    bool m_thirsPartySc;
    QDateTime m_lastSave;
    QDateTime m_lastStatusChanged;
    int m_warrantyDays;
    QString m_barcode;
    QString m_rejectReason;
    int m_informedStatus;
    QString m_imageIds;
    QColor m_color;
    QString m_orderMoving;
    int m_early;
    QString m_extEarly;
    QString m_issuedMsg;
    bool m_smsInform;
    int m_invoice;
    int m_cartridge;
    int m_vendorId;
    bool m_termsControl;
};

#endif // SREPAIRMODEL_H
