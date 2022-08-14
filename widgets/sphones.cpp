#include "sphones.h"
#include "ui_sphones.h"

SPhones::SPhones(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SPhones)
{
    ui->setupUi(this);
}

SPhones::SPhones(SPhonesModel *model, QWidget *parent) :
    SPhones(parent)
{
    setModel(model);
}

SPhones::~SPhones()
{
    clear();
    delete ui;
}

SPhoneModel *SPhones::phone(int item)
{
    return m_phonesModel->phonesList().at(item);
}

void SPhones::setModel(SPhonesModel *model)
{
    deleteAllForms();

    SPhoneModel *phoneModel;
    m_phonesModel = model;
    m_phoneModelsList = m_phonesModel->phonesList();
    foreach(phoneModel, m_phoneModelsList)
    {
        addForm(phoneModel);
    }
    if(m_phoneModelsList.isEmpty())
        addForm(m_phonesModel->primary());
}

void SPhones::addForm(SPhoneModel *model)
{
    SPhone *phoneForm = new SPhone(model, this);
    connect(phoneForm,SIGNAL(addPhone()),this,SLOT(addPhone()));
    connect(phoneForm,SIGNAL(delPhone(SPhone*)),this,SLOT(delPhone(SPhone*)));
    connect(phoneForm,SIGNAL(markedPrimary()),model,SLOT(setPrimaryUi()));

    m_isPrimary = model->type();
    if(m_isPrimary)
        m_primaryForm = phoneForm;  // форма с основным телефоном только одна; задача определения типа лежит на методах класса SPhonesModel

    if(!m_phoneFormsList.isEmpty())
        m_phoneFormsList.last()->setButtonVisible(SPhone::Add, true);
    m_phoneFormsList.append(phoneForm);
    ui->gridLayout_2->addWidget(phoneForm);
}

void SPhones::removeForm()
{

}

void SPhones::updateFormsButtons()
{
    QList<SPhone*>::const_iterator i = m_phoneFormsList.constBegin();
    while(i != m_phoneFormsList.constEnd()-1)
    {
        (*i)->setButtonVisible(SPhone::Add, false);
        i++;
    }
    (*i)->setButtonVisible(SPhone::Add, true);
}

void SPhones::clear()
{
    deleteAllForms();
}

void SPhones::deleteAllForms()
{
    SPhone *phoneForm;
    while( !m_phoneFormsList.isEmpty() )
    {
        phoneForm = m_phoneFormsList.last();
        m_phoneFormsList.removeLast();
        delete phoneForm;
    }
}

bool SPhones::isValid()
{
    return m_primaryForm->isValid();
}

SPhone *SPhones::primary()
{
    return m_primaryForm;
}

/*  SLOT
 */
void SPhones::addPhone()
{
    QList<SPhoneModel*> list;
    m_phonesModel->add(new SPhoneModel(m_phonesModel));
    list = m_phonesModel->phonesList();
    addForm(list.last());
    updateFormsButtons();
}

/*  SLOT
 */
void SPhones::delPhone(SPhone *form)
{
    m_phonesModel->remove(form->model());
    int formIndex = m_phoneFormsList.indexOf(form);
    m_phoneFormsList.removeAt(formIndex);
    delete form;
    updateFormsButtons();
}

