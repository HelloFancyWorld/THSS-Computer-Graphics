#pragma once

#include <QtWidgets/QWidget>
#include "ui_QtOpenGLDemo.h"
#include "OpenGLWidget.h"
#include <QLineEdit>
#include <QFileDialog>

class QtOpenGLDemo : public QWidget
{
    Q_OBJECT

public:
    QtOpenGLDemo(QWidget *parent = nullptr);
    ~QtOpenGLDemo();

public slots:
    void set_invert();
    void set_original();
    void set_filter_button();
	void on_collision_detected(bool dir, QVector3D color);
    void onConfigButtonClicked();
	void onStartButtonClicked();

private:
    Ui::QtOpenGLDemoClass ui;
    CoreFunctionWidget* core_widget;
};
