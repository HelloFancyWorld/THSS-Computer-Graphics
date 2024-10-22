#pragma once

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QKeyEvent>
#include <QFile>
#include <QTextStream>
#include "Camera.h"


class CoreFunctionWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
public:
	explicit CoreFunctionWidget(QWidget* parent = nullptr);
	~CoreFunctionWidget();
signals:
	void filter_change();
	void collision_detected(bool dir, QVector3D color);
protected:
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();
	void keyPressEvent(QKeyEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	int mouse_x, mouse_y;
private:
	QOpenGLShaderProgram shaderProgram;

	GLuint VBO, VAO, EBO;
	GLuint quadVAO, quadVBO;
	std::unique_ptr<QOpenGLFramebufferObject> FBO;
	QOpenGLShaderProgram invertShader, screenShader;

	Camera cam;

	// TASK 1 - Skybox
	unsigned int skyboxVAO, skyboxVBO;
	unsigned int cubemapTexture;
	QOpenGLShaderProgram skyboxShader;
	unsigned int loadCubemap(std::vector<std::string> faces);

	// TASK 2 - Cube
	float speed = 0.05f;
	QVector3D middleCubeSpeed = QVector3D(0.1f, 0.0f, 0.0f);
	QVector3D leftCubePos = QVector3D(-2.0f, 0.0f, 4.0f);
	QVector3D leftCubeColor = QVector3D(0.0f, 1.0f, 0.0f);
	QVector3D rightCubePos = QVector3D(2.0f, 0.0f, 4.0f);
	QVector3D rightCubeColor = QVector3D(1.0f, 0.0f, 0.0f);
	QVector3D middleCubePos = QVector3D(0.0f, 0.0f, 4.0f);
	QVector3D middleCubeColor = QVector3D(0.0f, 0.0f, 1.0f);
	QVector3D dir;

public:
	bool use_invert = false; // TASK 5 ÂË¾µ
	bool startRendering = false;	// TASK 4 - ÅäÖÃÎÄ¼þ
	QString readConfigFile(const QString& filePath);
};

