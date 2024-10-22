#include "OpenGLWidget.h"
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QHash>
#include <QOpenGLFramebufferObject>

// qHash 重载
inline uint qHash(const QVector3D &key, uint seed)
{
    return qHash(key.x(), seed) ^ qHash(key.y(), seed) ^ qHash(key.z(), seed);
}

CoreFunctionWidget::CoreFunctionWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    this->setFocusPolicy(Qt::StrongFocus);
}

CoreFunctionWidget::~CoreFunctionWidget()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void CoreFunctionWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_A)
    {
        this->cam.translate_left(0.2);
    }
    else if (e->key() == Qt::Key_D)
    {
        this->cam.translate_left(-0.2);
    }
    else if (e->key() == Qt::Key_W)
    {
        this->cam.translate_up(0.2);
    }
    else if (e->key() == Qt::Key_S)
    {
        this->cam.translate_up(-0.2);
    }
    else if (e->key() == Qt::Key_F)
    {
        this->cam.translate_forward(0.2);
    }
    else if (e->key() == Qt::Key_B)
    {
        this->cam.translate_forward(-0.2);
        ;
    }
    else if (e->key() == Qt::Key_Z)
    {
        this->cam.zoom_near(0.1);
    }
    else if (e->key() == Qt::Key_X)
    {
        this->cam.zoom_near(-0.1);
    }
    else if (e->key() == Qt::Key_T)
    {
        this->use_invert = !this->use_invert;
        emit filter_change();
    }
    update();
}

// TASK 4 读取配置文件
QString CoreFunctionWidget::readConfigFile(const QString &filePath)
{
    QFile file(filePath);
    QString returnString;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        returnString = "Cannot open config file! Use default setup";
        startRendering = true;
        return returnString;
    }

    QTextStream in(&file);
    QVector3D tempLeftCubePos, tempRightCubePos;
    QVector3D tempLeftCubeColor, tempRightCubeColor, tempMiddleCubeColor;
    float tempSpeed = 0.05f;
    bool leftCubePosSet = false, rightCubePosSet = false;
    bool leftCubeColorSet = false, rightCubeColorSet = false, middleCubeColorSet = false;
    bool speedSet = false;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList parts = line.split(":");
        if (parts.size() != 2)
            continue;

        QString key = parts[0].trimmed();
        QStringList values = parts[1].trimmed().split(",");
        if (values.size() != 3)
        {
            speed = values[0].toFloat();
            speedSet = true;
            continue;
        }

        QVector3D vec(values[0].toFloat(), values[1].toFloat(), values[2].toFloat());
        if (key == "leftCubePos")
        {
            tempLeftCubePos = vec;
            leftCubePosSet = true;
        }
        else if (key == "leftCubeColor")
        {
            tempLeftCubeColor = vec;
            leftCubeColorSet = true;
        }
        else if (key == "rightCubePos")
        {
            tempRightCubePos = vec;
            rightCubePosSet = true;
        }
        else if (key == "rightCubeColor")
        {
            tempRightCubeColor = vec;
            rightCubeColorSet = true;
        }
        else if (key == "middleCubeColor")
        {
            tempMiddleCubeColor = vec;
            middleCubeColorSet = true;
        }
    }

    // 检查所有值是否存在
    if (!leftCubePosSet || !rightCubePosSet || !leftCubeColorSet || !rightCubeColorSet || !middleCubeColorSet || !speedSet)
    {
        startRendering = true;
        return "Invalid config: Missing required values! Use Default Setup!";
    }

    // 检查颜色是否重复
    QSet<QVector3D> colors = {tempLeftCubeColor, tempRightCubeColor, tempMiddleCubeColor};
    if (colors.size() != 3)
    {
        startRendering = true;
        return "Invalid config: Duplicate colors found! Use Default Setup!";
    }

    // 检查位置是否重叠
    auto distance = [](const QVector3D &a, const QVector3D &b)
    {
        return (a - b).length();
    };

    if (distance(tempLeftCubePos, tempRightCubePos) < 2.5f)
    {
        startRendering = true;
        return "Invalid config: Overlapping cube positions found! Use Default Setup!";
    }

    // 如果检查通过，更新成员变量
    leftCubePos = tempLeftCubePos;
    leftCubeColor = tempLeftCubeColor;
    rightCubePos = tempRightCubePos;
    rightCubeColor = tempRightCubeColor;
    middleCubePos = (leftCubePos + rightCubePos) / 2;
    middleCubeColor = tempMiddleCubeColor;
    speed = tempSpeed;

    // 计算方向向量并设置 middleCubeSpeed
    dir = rightCubePos - leftCubePos;
    dir.normalize();
    middleCubeSpeed = dir * speed;

    startRendering = true;
    return "Config file loaded successfully!";
}

void CoreFunctionWidget::initializeGL()
{
    this->initializeOpenGLFunctions();

    // TASK 5 滤镜
    // 创建帧缓冲对象
    FBO = std::make_unique<QOpenGLFramebufferObject>(this->size(), QOpenGLFramebufferObject::CombinedDepthStencil);

    // 初始化反相滤镜着色器
    invertShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "./invert.vert");
    invertShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "./invert.frag");
    invertShader.link();

    // 初始化屏幕着色器
    screenShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "./screen.vert");
    screenShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "./screen.frag");
    screenShader.link();

    // 初始化四边形VAO
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
    // End TASK 5

    // 计算方向向量并设置 middleCubeSpeed
    dir = rightCubePos - leftCubePos;
    dir.normalize();
    middleCubeSpeed = dir * speed;

    glEnable(GL_DEPTH_TEST);
    this->cam.set_initial_distance_ratio(3.0);
    bool success = shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, "./textures.vert");
    if (!success)
    {
        qDebug() << "shaderProgram addShaderFromSourceFile failed!" << shaderProgram.log();
        return;
    }

    success = shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, "./textures.frag");
    if (!success)
    {
        qDebug() << "shaderProgram addShaderFromSourceFile failed!" << shaderProgram.log();
        return;
    }

    success = shaderProgram.link();
    if (!success)
    {
        qDebug() << "shaderProgram link failed!" << shaderProgram.log();
    }

    // TASK 2 - Cube
    float cube_vertices[] = {
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f};

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 1, 5, 5, 4, 0,
        2, 3, 7, 7, 6, 2,
        0, 3, 7, 7, 4, 0,
        1, 2, 6, 6, 5, 1};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 设置定时器
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&CoreFunctionWidget::update));
    timer->start(16); // 60帧每秒

    // TASK 1 - Skybox
    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};
    // Skybox shader
    skyboxShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "./skybox.vert");
    skyboxShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "./skybox.frag");
    skyboxShader.link();

    // Skybox VAO and VBO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindVertexArray(0);

    // Load skybox textures
    std::vector<std::string> faces{
        "./posx.jpg",
        "./negx.jpg",
        "./posy.jpg",
        "./negy.jpg",
        "./posz.jpg",
        "./negz.jpg"};
    cubemapTexture = loadCubemap(faces);
}

unsigned int CoreFunctionWidget::loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        QImage img = QImage(faces[i].c_str()).convertToFormat(QImage::Format_RGB888);
        if (!img.isNull())
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, img.width(), img.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.bits());
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void CoreFunctionWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void CoreFunctionWidget::paintGL()
{
	// TASK 5 - 滤镜
    FBO->bind();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    
    // 绘制begin


    // TASK 4 - 文件读取

    if (!startRendering)// 只渲染天空盒
    {
        // TASK 1 - Skybox
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL); // 更改深度函数，以便天空盒可以在最远处渲染
        skyboxShader.bind();
        QMatrix4x4 view = this->cam.get_camera_matrix();
        ;
        view.setColumn(3, QVector4D(0, 0, 0, 1)); // 移除平移部分
        skyboxShader.setUniformValue("view", view);
        QMatrix4x4 projection_matrix;

        // 默认使用透视投影 TASK 0
        projection_matrix.perspective(90, 1.0, 0.01, 50.0);

        skyboxShader.setUniformValue("projection", projection_matrix);
        // 绑定天空盒 VAO 并绘制
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        skyboxShader.release();
        glDepthFunc(GL_LESS); // 重置深度函数      
    }
    else
    {
        // TASK 2 - Cube
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.bind();
        {
            QMatrix4x4 model;
            QMatrix4x4 view = this->cam.get_camera_matrix();
            QMatrix4x4 projection;

            // 默认使用透视投影 TASK 0
            projection.perspective(90, 1.0, 0.01, 50.0);

            shaderProgram.setUniformValue("view", view);
            shaderProgram.setUniformValue("projection", projection);

            glBindVertexArray(VAO);

            // 左
            model.setToIdentity();
            model.translate(leftCubePos);
            shaderProgram.setUniformValue("model", model);
            shaderProgram.setUniformValue("color", leftCubeColor);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            // 右
            model.setToIdentity();
            model.translate(rightCubePos);
            shaderProgram.setUniformValue("model", model);
            shaderProgram.setUniformValue("color", rightCubeColor);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            // 中间立方体
            model.setToIdentity();
            model.translate(middleCubePos);
            shaderProgram.setUniformValue("model", model);
            shaderProgram.setUniformValue("color", middleCubeColor);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            // qDebug() << middleCubeSpeed;
            //  更新中间立方体的位置
            middleCubePos += middleCubeSpeed;
            if ((middleCubePos + dir).x() >= rightCubePos.x() || (middleCubePos - dir).x() <= leftCubePos.x())
            {
                middleCubeSpeed = -middleCubeSpeed; // 反向运动
                bool direction = QVector3D::dotProduct(dir, middleCubeSpeed) > 0;
                QVector3D textColor = direction ? leftCubeColor : rightCubeColor;
                emit collision_detected(direction, textColor);
            }
        }

        shaderProgram.release();

        // TASK 1 - Skybox
        // Draw skybox as last
        glDepthFunc(GL_LEQUAL); // 更改深度函数，以便天空盒可以在最远处渲染
        skyboxShader.bind();
        QMatrix4x4 view = this->cam.get_camera_matrix();
        ;
        view.setColumn(3, QVector4D(0, 0, 0, 1)); // 移除平移部分
        skyboxShader.setUniformValue("view", view);
        QMatrix4x4 projection_matrix;

        // 默认使用透视投影 TASK 0
        projection_matrix.perspective(90, 1.0, 0.01, 50.0);

        skyboxShader.setUniformValue("projection", projection_matrix);
        // 绑定天空盒 VAO 并绘制
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        skyboxShader.release();
        glDepthFunc(GL_LESS); // 重置深度函数
    }
    

	// 绘制end

    FBO->release();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (use_invert)
    {
        invertShader.bind();
    }
    else
    {
        screenShader.bind();
    }

    glBindVertexArray(quadVAO);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, FBO->texture());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (use_invert)
    {
        invertShader.release();
    }
    else
    {
        screenShader.release();
    }
}

void CoreFunctionWidget::mousePressEvent(QMouseEvent *e)
{
    mouse_x = e->x();
    mouse_y = e->y();
}

void CoreFunctionWidget::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();

    if (abs(x - mouse_x) >= 3)
    {
        if (x > mouse_x)
        {
            this->cam.rotate_left(3.0);
        }
        else
        {
            this->cam.rotate_left(-3.0);
        }
        mouse_x = x;
    }

    if (abs(y - mouse_y) >= 3)
    {
        if (y > mouse_y)
        {
            this->cam.rotate_up(-3.0);
        }
        else
        {
            this->cam.rotate_up(3.0);
        }

        mouse_y = y;
    }

    update();
}
