/********************************************************************************
** Form generated from reading UI file 'QtOpenGLDemo.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTOPENGLDEMO_H
#define UI_QTOPENGLDEMO_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QtOpenGLDemoClass
{
public:
    QGroupBox *groupBox;
    QRadioButton *originalButton;
    QRadioButton *invertButton;
    QTextEdit *textEdit;
    QLabel *label;
    QPushButton *configButton;
    QTextEdit *textEdit_2;
    QPushButton *startButton;

    void setupUi(QWidget *QtOpenGLDemoClass)
    {
        if (QtOpenGLDemoClass->objectName().isEmpty())
            QtOpenGLDemoClass->setObjectName("QtOpenGLDemoClass");
        QtOpenGLDemoClass->resize(900, 700);
        groupBox = new QGroupBox(QtOpenGLDemoClass);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(720, 30, 161, 101));
        originalButton = new QRadioButton(groupBox);
        originalButton->setObjectName("originalButton");
        originalButton->setGeometry(QRect(10, 30, 132, 22));
        originalButton->setChecked(true);
        invertButton = new QRadioButton(groupBox);
        invertButton->setObjectName("invertButton");
        invertButton->setGeometry(QRect(10, 60, 132, 22));
        textEdit = new QTextEdit(QtOpenGLDemoClass);
        textEdit->setObjectName("textEdit");
        textEdit->setGeometry(QRect(720, 390, 161, 71));
        label = new QLabel(QtOpenGLDemoClass);
        label->setObjectName("label");
        label->setGeometry(QRect(740, 360, 61, 21));
        configButton = new QPushButton(QtOpenGLDemoClass);
        configButton->setObjectName("configButton");
        configButton->setGeometry(QRect(740, 150, 111, 23));
        textEdit_2 = new QTextEdit(QtOpenGLDemoClass);
        textEdit_2->setObjectName("textEdit_2");
        textEdit_2->setGeometry(QRect(720, 220, 161, 71));
        startButton = new QPushButton(QtOpenGLDemoClass);
        startButton->setObjectName("startButton");
        startButton->setGeometry(QRect(740, 180, 111, 21));

        retranslateUi(QtOpenGLDemoClass);

        QMetaObject::connectSlotsByName(QtOpenGLDemoClass);
    } // setupUi

    void retranslateUi(QWidget *QtOpenGLDemoClass)
    {
        QtOpenGLDemoClass->setWindowTitle(QCoreApplication::translate("QtOpenGLDemoClass", "QtOpenGLDemo", nullptr));
        groupBox->setTitle(QCoreApplication::translate("QtOpenGLDemoClass", "Projection", nullptr));
        originalButton->setText(QCoreApplication::translate("QtOpenGLDemoClass", "Original", nullptr));
        invertButton->setText(QCoreApplication::translate("QtOpenGLDemoClass", "Invert Filter", nullptr));
        label->setText(QCoreApplication::translate("QtOpenGLDemoClass", "Collison", nullptr));
        configButton->setText(QCoreApplication::translate("QtOpenGLDemoClass", "Load Configure File", nullptr));
        startButton->setText(QCoreApplication::translate("QtOpenGLDemoClass", "Start Directly", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtOpenGLDemoClass: public Ui_QtOpenGLDemoClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTOPENGLDEMO_H
