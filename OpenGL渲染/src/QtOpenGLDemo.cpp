#include "QtOpenGLDemo.h"

QtOpenGLDemo::QtOpenGLDemo(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	core_widget = new CoreFunctionWidget(this);
	core_widget->setObjectName(QStringLiteral("OpenGLWidget"));
	core_widget->setGeometry(QRect(0, 0, 700, 700));

	connect(this->ui.originalButton, SIGNAL(clicked()), this, SLOT(set_original()));
	connect(this->ui.invertButton, SIGNAL(clicked()), this, SLOT(set_invert()));
	connect(this->core_widget, SIGNAL(filter_change()), this, SLOT(set_filter_button()));

	// TASK 3 Åö×²¼ì²â Ìí¼ÓÎÄ±¾¿ò
	connect(this->core_widget, SIGNAL(collision_detected(bool, QVector3D)), this, SLOT(on_collision_detected(bool, QVector3D)));
	// TASK 4 ÅäÖÃÎÄ¼ş
	connect(this->ui.configButton, SIGNAL(clicked()), this, SLOT(onConfigButtonClicked()));
	connect(this->ui.startButton, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()));
}

QtOpenGLDemo::~QtOpenGLDemo()
{}

void QtOpenGLDemo::onStartButtonClicked() {
	this->ui.textEdit_2->setText("Use Default Setup!");
	this->core_widget->startRendering = true;
	this->core_widget->update();
}

void QtOpenGLDemo::onConfigButtonClicked() {
	QString filePath = QFileDialog::getOpenFileName(this, tr("Open Config File"), "", tr("Text Files (*.txt)"));
	if (filePath.isEmpty()) {
		return;
	}
	QString result;
	result = this->core_widget->readConfigFile(filePath);
	this->ui.textEdit_2->setText(result);
	this->core_widget->update();
}

void QtOpenGLDemo::set_filter_button() {
	if (this->core_widget->use_invert) {
		this->ui.invertButton->setChecked(true);
	}
	else {
		this->ui.originalButton->setChecked(true);
	}
}

void QtOpenGLDemo::on_collision_detected(bool dir, QVector3D textColor) {
	QColor qcolor = QColor(textColor.x() * 255, textColor.y() * 255, textColor.z() * 255);
	this->ui.textEdit->setTextColor(qcolor);
	this->ui.textEdit->setText("Collision detected with this color!");
}

void QtOpenGLDemo::set_invert() {
	this->core_widget->use_invert = true;
	this->core_widget->update();
}

void QtOpenGLDemo::set_original() {
	this->core_widget->use_invert = false;
	this->core_widget->update();
}
