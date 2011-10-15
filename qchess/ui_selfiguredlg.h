/********************************************************************************
** Form generated from reading ui file 'selfiguredlg.ui'
**
** Created: Sun 2. Oct 17:30:54 2011
**      by: Qt User Interface Compiler version 4.5.3
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SELFIGUREDLG_H
#define UI_SELFIGUREDLG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QRadioButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FigureDialog
{
public:
    QWidget *formLayoutWidget;
    QFormLayout *formLayout;
    QRadioButton *rbQueen_;
    QRadioButton *rbKnight_;
    QRadioButton *rbRook_;
    QRadioButton *rbBishop_;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *FigureDialog)
    {
        if (FigureDialog->objectName().isEmpty())
            FigureDialog->setObjectName(QString::fromUtf8("FigureDialog"));
        FigureDialog->setWindowModality(Qt::WindowModal);
        FigureDialog->resize(454, 174);
        formLayoutWidget = new QWidget(FigureDialog);
        formLayoutWidget->setObjectName(QString::fromUtf8("formLayoutWidget"));
        formLayoutWidget->setGeometry(QRect(30, 20, 168, 129));
        formLayout = new QFormLayout(formLayoutWidget);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        rbQueen_ = new QRadioButton(formLayoutWidget);
        rbQueen_->setObjectName(QString::fromUtf8("rbQueen_"));

        formLayout->setWidget(4, QFormLayout::LabelRole, rbQueen_);

        rbKnight_ = new QRadioButton(formLayoutWidget);
        rbKnight_->setObjectName(QString::fromUtf8("rbKnight_"));

        formLayout->setWidget(2, QFormLayout::LabelRole, rbKnight_);

        rbRook_ = new QRadioButton(formLayoutWidget);
        rbRook_->setObjectName(QString::fromUtf8("rbRook_"));

        formLayout->setWidget(1, QFormLayout::LabelRole, rbRook_);

        rbBishop_ = new QRadioButton(formLayoutWidget);
        rbBishop_->setObjectName(QString::fromUtf8("rbBishop_"));

        formLayout->setWidget(0, QFormLayout::LabelRole, rbBishop_);

        buttonBox = new QDialogButtonBox(formLayoutWidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        formLayout->setWidget(5, QFormLayout::LabelRole, buttonBox);


        retranslateUi(FigureDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), FigureDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), FigureDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(FigureDialog);
    } // setupUi

    void retranslateUi(QDialog *FigureDialog)
    {
        FigureDialog->setWindowTitle(QApplication::translate("FigureDialog", "What piece do you want to promote your pawn to?", 0, QApplication::UnicodeUTF8));
        rbQueen_->setText(QApplication::translate("FigureDialog", "Queen", 0, QApplication::UnicodeUTF8));
        rbKnight_->setText(QApplication::translate("FigureDialog", "Knight", 0, QApplication::UnicodeUTF8));
        rbRook_->setText(QApplication::translate("FigureDialog", "Rook", 0, QApplication::UnicodeUTF8));
        rbBishop_->setText(QApplication::translate("FigureDialog", "Bishop", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(FigureDialog);
    } // retranslateUi

};

namespace Ui {
    class FigureDialog: public Ui_FigureDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SELFIGUREDLG_H
