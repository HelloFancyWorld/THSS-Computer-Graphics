#include <QApplication>
#include <QWidget>
#include <QScreen>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class mPoint
{
public:
    QPoint point;
    int type; // 0:入点 1:出点 2：非交点主多边形顶点 3：非交点裁剪多边形顶点 4：交点
    mPoint *next = nullptr;
    bool visited = false;
    mPoint *brother = nullptr; // 用于双向指针
    mPoint(QPoint p, int type) : point(p), type(type) {}
};

class PolygonWidget : public QWidget
{
    QVector<QVector<QPoint>> mainPolygon;   // 主多边形
    QVector<QVector<QPoint>> clipPolygon;   // 裁剪多边形
    QList<mPoint> completeMainPolygon;      // 用于计算的完整主多边形、
    QList<mPoint> completeClipPolygon;      // 用于计算的完整裁剪多边形
    QVector<QPoint> currentPolygon;         // 当前正在绘制的多边形
    QVector<QVector<QPoint>> resultPolygon; // 裁剪后的结果
    bool isMainActive = true;               // 默认主多边形为激活状态
    QPushButton *toggleButton;              // 切换多边形类型的按钮
    QLabel *modeLabel;                      // 当前模式标签

public:
    PolygonWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
    }

    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制主多边形
        painter.setPen(Qt::blue);
        painter.setBrush(Qt::blue);
        for (const auto &polygon : mainPolygon)
        {
            for (const auto &point : polygon)
            {
                painter.drawEllipse(point, 3, 3);
            }
            if (polygon.size() > 1)
            {
                for (int i = 0; i < polygon.size(); ++i)
                {
                    if (i == 0)
                        continue;
                    painter.drawLine(polygon[i - 1], polygon[i]);
                }
                painter.drawLine(polygon.last(), polygon.first());
            }
        }

        // 绘制裁剪多边形
        painter.setPen(Qt::green);
        painter.setBrush(Qt::green);
        for (const auto &polygon : clipPolygon)
        {
            for (const auto &point : polygon)
            {
                painter.drawEllipse(point, 3, 3);
            }
            if (polygon.size() > 1)
            {
                for (int i = 0; i < polygon.size(); ++i)
                {
                    if (i == 0)
                        continue;
                    painter.drawLine(polygon[i - 1], polygon[i]);
                }
                painter.drawLine(polygon.last(), polygon.first());
            }
        }

        // 绘制当前多边形
        painter.setPen(isMainActive ? Qt::blue : Qt::green);
        painter.setBrush(isMainActive ? Qt::blue : Qt::green);
        for (const auto &point : currentPolygon)
        {
            painter.drawEllipse(point, 3, 3);
        }

        // 绘制裁剪结果
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::red);
        for (const auto &polygon : resultPolygon)
        {
            for (const auto &point : polygon)
            {
                painter.drawEllipse(point, 3, 3);
            }
            if (polygon.size() > 1)
            {
                for (int i = 0; i < polygon.size(); ++i)
                {
                    if (i == 0)
                        continue;
                    painter.drawLine(polygon[i - 1], polygon[i]);
                }
                painter.drawLine(polygon.last(), polygon.first());
            }
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            // 左键点击，添加点
            currentPolygon.append(event->pos());
            update();
        }
        else if (event->button() == Qt::RightButton)
        {
            // 右键点击，闭合多边形
            if (currentPolygon.size() >= 3)
            {
                if (isMainActive)
                {
                    mainPolygon.append(currentPolygon);
                }
                else
                {
                    clipPolygon.append(currentPolygon);
                }
                currentPolygon.clear();
                update(); // 重新绘制以显示闭合边
            }
            else
            {
                QMessageBox::warning(this, "Warning", "多边形至少有三个点才能闭合");
            }
        }
    }

    void clearPolygons()
    {
        if (isMainActive)
            mainPolygon.clear();
        else
            clipPolygon.clear();
        currentPolygon.clear();
        update();
    }

    void undoLastPoint()
    {
        if (!currentPolygon.isEmpty())
        {
            currentPolygon.removeLast();
            update();
        }
    }

    void togglePolygon()
    {
        isMainActive = !isMainActive;
        toggleButton->setText(isMainActive ? "切换为裁剪多边形" : "切换为主多边形");
        modeLabel->setText(isMainActive ? "当前模式: 主多边形" : "当前模式: 裁剪多边形");
    }

    void clipPolygons()
    {
        resultPolygon.clear();
        // 预处理
        prepareClip();
        // 按顺序添加交点到顶点表
        addInters();
        weilerAthertonClip(completeMainPolygon, completeClipPolygon);
        update();
    }

    void prepareClip()
    {
        // 将处理后的多边形按顺序加入新的 vector 中
        completeMainPolygon.clear();
        completeClipPolygon.clear();

        // 处理主多边形
        if (!mainPolygon.isEmpty())
        {
            int maxAreaIndex = 0;
            double maxArea = calculateArea(mainPolygon[0]);

            for (int i = 1; i < mainPolygon.size(); ++i)
            {
                double area = calculateArea(mainPolygon[i]);
                if (std::abs(area) > std::abs(maxArea))
                {
                    maxArea = area;
                    maxAreaIndex = i;
                }
            }

            // 将最大面积的多边形作为外环
            QVector<QPoint> outerRing = mainPolygon[maxAreaIndex];
            if (maxArea < 0) // 顺时针，反转
            {
                std::reverse(outerRing.begin(), outerRing.end());
            }
            for (auto &point : outerRing)
            {
                mPoint p(point, 2);
                completeMainPolygon.push_back(p);
            }
            mPoint p(outerRing.first(), 2);
            completeMainPolygon.push_back(p);

            // 处理内环
            for (int i = 0; i < mainPolygon.size(); ++i)
            {
                if (i == maxAreaIndex)
                    continue;

                QVector<QPoint> innerRing = mainPolygon[i];
                double area = calculateArea(innerRing);
                if (area > 0) // 逆时针，反转
                {
                    std::reverse(innerRing.begin(), innerRing.end());
                }
                for (auto &point : innerRing)
                {
                    mPoint p(point, 2);
                    completeMainPolygon.push_back(p);
                }
                mPoint p(innerRing.first(), 2);
                completeMainPolygon.push_back(p);
            }
        }

        // 处理裁剪多边形
        if (!clipPolygon.isEmpty())
        {
            int maxAreaIndex = 0;
            double maxArea = calculateArea(clipPolygon[0]);

            for (int i = 1; i < clipPolygon.size(); ++i)
            {
                double area = calculateArea(clipPolygon[i]);
                if (std::abs(area) > std::abs(maxArea))
                {
                    maxArea = area;
                    maxAreaIndex = i;
                }
            }

            // 将最大面积的多边形作为外环
            QVector<QPoint> outerRing = clipPolygon[maxAreaIndex];
            if (maxArea < 0) // 顺时针，反转
            {
                std::reverse(outerRing.begin(), outerRing.end());
            }
            for (auto &point : outerRing)
            {
                mPoint p(point, 3);
                completeClipPolygon.push_back(p);
            }
            mPoint p(outerRing.first(), 3);
            completeClipPolygon.push_back(p);
            // 处理内环
            for (int i = 0; i < clipPolygon.size(); ++i)
            {
                if (i == maxAreaIndex)
                    continue;

                QVector<QPoint> innerRing = clipPolygon[i];
                double area = calculateArea(innerRing);
                if (area > 0) // 逆时针，反转
                {
                    std::reverse(innerRing.begin(), innerRing.end());
                }

                for (auto &point : innerRing)
                {
                    mPoint p(point, 3);
                    completeClipPolygon.push_back(p);
                }
                mPoint p(innerRing.first(), 3);
                completeClipPolygon.push_back(p);
            }
        }
    }

    void addInters()
    {
        QVector<mPoint *> temp_inters;                                               // 当前主多边形边上的交点
        QVector<QVector<mPoint *>> temp_inters_for_clip(completeClipPolygon.size()); // 裁剪多边形边上的交点

        QList<mPoint *> tempCompleteMainPolygon;

        QPoint mainStart = completeMainPolygon.first().point;
        int mainStartIndex = 0;
        QPoint clipStart = completeClipPolygon.first().point;
        int clipStartIndex = 0;

        for (int i = 0; i < completeMainPolygon.size() - 1; ++i)
        {
            temp_inters.clear();
            const QPoint &S = completeMainPolygon[i].point;
            const QPoint &E = completeMainPolygon[i + 1].point;

            if (S == mainStart && i != mainStartIndex)
            {
                mainStartIndex = i + 1;
                mainStart = completeMainPolygon[mainStartIndex].point;
                mPoint *p1 = new mPoint(S, completeMainPolygon[i].type);
                tempCompleteMainPolygon.push_back(p1);
                continue;
            }

            clipStartIndex = 0;
            clipStart = completeClipPolygon.first().point;
            for (int j = 0; j < completeClipPolygon.size() - 1; ++j)
            {
                const QPoint &clipS = completeClipPolygon[j].point;
                const QPoint &clipE = completeClipPolygon[j + 1].point;

                if (clipS == clipStart && j != clipStartIndex)
                {
                    clipStartIndex = j + 1;
                    clipStart = completeClipPolygon[clipStartIndex].point;
                    continue;
                }

                // 计算交点
                QPoint intersection = computeIntersection(S, E, clipS, clipE);

                // 检查交点是否在两条边的端点之间
                if (isInside(S, E, intersection) && isInside(clipS, clipE, intersection))
                {
                    // 添加交点
                    int temp_type = determineIntersectionType(S, E, clipS);
                    mPoint *p1 = new mPoint(intersection, temp_type);
                    mPoint *p2 = new mPoint(intersection, temp_type);
                    temp_inters.push_back(p1);
                    temp_inters_for_clip[j].push_back(p2);
                }
            }
            // 对交点排序
            if (!temp_inters.isEmpty())
            {
                if (S.x() < E.x())
                {
                    std::sort(temp_inters.begin(), temp_inters.end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.x() < b->point.x(); });
                }
                else if (S.x() > E.x())
                {
                    std::sort(temp_inters.begin(), temp_inters.end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.x() > b->point.x(); });
                }
                else if (S.y() < E.y())
                {
                    std::sort(temp_inters.begin(), temp_inters.end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.y() < b->point.y(); });
                }
                else if (S.y() > E.y())
                {
                    std::sort(temp_inters.begin(), temp_inters.end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.y() > b->point.y(); });
                }
            }
            // 将交点加入顶点表中
            mPoint *p1 = new mPoint(S, completeMainPolygon[i].type);
            tempCompleteMainPolygon.push_back(p1);
            for (auto &p : temp_inters)
            {
                tempCompleteMainPolygon.push_back(p);
            }
        }
        mPoint *p1 = new mPoint(completeMainPolygon.last().point, completeMainPolygon.last().type);
        tempCompleteMainPolygon.push_back(p1);
        completeMainPolygon.clear();
        for (auto &p : tempCompleteMainPolygon)
        {
            completeMainPolygon.push_back(*p);
        }
        // 处理裁剪多边形
        QList<mPoint *> tempCompleteClipPolygon;
        for (int i = 0; i < completeClipPolygon.size() - 1; ++i)
        {
            if (!temp_inters_for_clip[i].isEmpty())
            {
                if (completeClipPolygon[i].point.x() < completeClipPolygon[i + 1].point.x())
                {
                    std::sort(temp_inters_for_clip[i].begin(), temp_inters_for_clip[i].end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.x() < b->point.x(); });
                }
                else if (completeClipPolygon[i].point.x() > completeClipPolygon[i + 1].point.x())
                {
                    std::sort(temp_inters_for_clip[i].begin(), temp_inters_for_clip[i].end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.x() > b->point.x(); });
                }
                else if (completeClipPolygon[i].point.y() < completeClipPolygon[i + 1].point.y())
                {
                    std::sort(temp_inters_for_clip[i].begin(), temp_inters_for_clip[i].end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.y() < b->point.y(); });
                }
                else if (completeClipPolygon[i].point.y() > completeClipPolygon[i + 1].point.y())
                {
                    std::sort(temp_inters_for_clip[i].begin(), temp_inters_for_clip[i].end(), [](const mPoint *a, const mPoint *b)
                              { return a->point.y() > b->point.y(); });
                }
            }
            mPoint *p1 = new mPoint(completeClipPolygon[i].point, completeClipPolygon[i].type);
            tempCompleteClipPolygon.push_back(p1);
            for (auto &p : temp_inters_for_clip[i])
            {
                tempCompleteClipPolygon.push_back(p);
            }
        }
        mPoint *p2 = new mPoint(completeClipPolygon.last().point, completeClipPolygon.last().type);
        tempCompleteClipPolygon.push_back(p2);
        completeClipPolygon.clear();
        for (auto &p : tempCompleteClipPolygon)
        {
            completeClipPolygon.push_back(*p);
        }

        // 设置交点的 brother 指针
        for (int i = 0; i < completeMainPolygon.size(); ++i)
        {
            if (completeMainPolygon[i].type == 0 || completeMainPolygon[i].type == 1)
            {
                for (int j = 0; j < completeClipPolygon.size(); ++j)
                {
                    if (completeMainPolygon[i].point == completeClipPolygon[j].point)
                    {
                        completeMainPolygon[i].brother = &completeClipPolygon[j];
                        completeClipPolygon[j].brother = &completeMainPolygon[i];
                        break;
                    }
                }
            }
        }
        int tmp_type = 0; // 0:入点 1:出点
        int startNextIndex = 0;
        QPoint startNextPoint = completeMainPolygon.first().point;
        for (int i = 0; i < completeMainPolygon.size(); ++i)
        {
            // if (completeMainPolygon[i].type == 4)
            // {
            //     completeMainPolygon[i].type = tmp_type;
            //     completeMainPolygon[i].brother->type = tmp_type;
            //     tmp_type = 1 - tmp_type;
            // }
            if (completeMainPolygon[i].point == startNextPoint && i != startNextIndex)
            {
                completeMainPolygon[i].next = &completeMainPolygon[startNextIndex];
                startNextIndex = i + 1;
                if (startNextIndex == completeMainPolygon.size())
                    break;
                startNextPoint = completeMainPolygon[startNextIndex].point;
                continue;
            }
            else
            {
                completeMainPolygon[i].next = &completeMainPolygon[i + 1];
            }
        }
        startNextIndex = 0;
        startNextPoint = completeClipPolygon.first().point;
        for (int i = 0; i < completeClipPolygon.size(); ++i)
        {
            if (completeClipPolygon[i].point == startNextPoint && i != startNextIndex)
            {
                completeClipPolygon[i].next = &completeClipPolygon[startNextIndex];
                startNextIndex = i + 1;
                if (startNextIndex == completeClipPolygon.size())
                    break;
                startNextPoint = completeClipPolygon[startNextIndex].point;
                continue;
            }
            else
            {
                completeClipPolygon[i].next = &completeClipPolygon[i + 1];
            }
        }

        // 处理交点集为空的情况
        handleEmptyIntersections();
    }

    bool isLeftTurn(const QPoint &a, const QPoint &b, const QPoint &c)
    {
        return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x()) > 0;
    }

    int determineIntersectionType(const QPoint &mainStart, const QPoint &mainEnd, const QPoint &clipStart)
    {
        bool isLeft = isLeftTurn(mainStart, mainEnd, clipStart);

        if (isLeft)
        {
            return 0; // 入点
        }
        else
        {
            return 1; // 出点
        }
    }

    void handleEmptyIntersections()
    {
        // 处理主多边形的每个环
        QVector<QPoint> polygon;
        bool noIntersections = true;
        auto startPoint = completeMainPolygon.first();
        auto startPointIndex = 0;
        for (int i = 0; i < completeMainPolygon.size(); ++i)
        {
            if (completeMainPolygon[i].point == startPoint.point && i != startPointIndex)
            {
                if (noIntersections)
                {
                    // 使用射线法判断环中的一个点是否在剪裁多边形内部
                    if (isPointInPolygon(polygon.first(), clipPolygon))
                    {
                        resultPolygon.push_back(polygon);
                    }
                }
                startPointIndex = i + 1;
                if (startPointIndex == completeMainPolygon.size())
                    break;
                startPoint = completeMainPolygon[startPointIndex];
                noIntersections = true;
                polygon.clear();
                continue;
            }
            if (completeMainPolygon[i].type == 4 || completeMainPolygon[i].type == 1 ||
                completeMainPolygon[i].type == 0) // 交点或出点或入点
            {
                noIntersections = false;
            }
            else // 非交点
            {
                polygon.push_back(completeMainPolygon[i].point);
            }
        }

        // 处理裁剪多边形的每个环
        polygon.clear();
        noIntersections = true;
        startPoint = completeClipPolygon.first();
        startPointIndex = 0;
        for (int i = 0; i < completeClipPolygon.size(); ++i)
        {
            if (completeClipPolygon[i].point == startPoint.point && i != startPointIndex)
            {
                if (noIntersections)
                {
                    // 使用射线法判断环中的一个点是否在主多边形内部
                    if (isPointInPolygon(polygon.first(), mainPolygon))
                    {
                        resultPolygon.push_back(polygon);
                    }
                }
                startPointIndex = i + 1;
                if (startPointIndex == completeClipPolygon.size())
                    break;
                startPoint = completeClipPolygon[startPointIndex];
                noIntersections = true;
                polygon.clear();
                continue;
            }
            if (completeClipPolygon[i].type == 4 || completeClipPolygon[i].type == 1 ||
                completeClipPolygon[i].type == 0) // 交点或出点或入点
            {
                noIntersections = false;
            }
            else // 非交点
            {
                polygon.push_back(completeClipPolygon[i].point);
            }
        }
    }

    bool isPointInPolygon(const QPoint &point, const QVector<QVector<QPoint>> &polygon)
    {
        int intersections = 0;
        for (const auto &ring : polygon)
        {
            for (int i = 0; i < ring.size(); ++i)
            {
                QPoint p1 = ring[i];
                QPoint p2 = ring[(i + 1) % ring.size()];

                if ((p1.y() > point.y()) != (p2.y() > point.y()) &&
                    point.x() < (p2.x() - p1.x()) * (point.y() - p1.y()) / (p2.y() - p1.y()) + p1.x())
                {
                    intersections++;
                }
            }
        }
        return (intersections % 2) != 0;
    }

    void weilerAthertonClip(const QList<mPoint> &subjectPolygon, const QList<mPoint> &clipPolygon)
    {

        // 查找未访问的交点
        auto findUnvisitedIntersection = [](QList<mPoint> &polygon) -> mPoint *
        {
            for (auto &mpoint : polygon)
            {
                if ((mpoint.type == 0 /*|| mpoint.type == 1 */) && !mpoint.visited)
                {
                    return &mpoint;
                }
            }
            return nullptr;
        };

        mPoint *startPoint = findUnvisitedIntersection(completeMainPolygon);
        while (startPoint != nullptr)
        {
            QVector<QPoint> currentResultPolygon;
            mPoint *currentPoint = startPoint;

            currentResultPolygon.append(currentPoint->point);
            do
            {
                //                MODIFIED
                //                currentResultPolygon.append(currentPoint->point);
                //                currentPoint->visited = true;
                //                currentPoint->brother->visited = true;

                if (currentPoint->type == 0) // 入点
                {
                    // 在主多边形顶点表内跟踪
                    do
                    {
                        currentPoint = currentPoint->next;
                        currentResultPolygon.append(currentPoint->point);
                    } while (currentPoint->type != 1); // 直到遇到出点
                }
                else if (currentPoint->type == 1) // 出点
                {
                    // 在裁剪多边形顶点表内跟踪
                    do
                    {
                        currentPoint = currentPoint->next;
                        currentResultPolygon.append(currentPoint->point);
                    } while (currentPoint->type != 0); // 直到遇到入点
                }

                currentPoint = currentPoint->brother; // 切换到对应的交点
                currentPoint->visited = true;
                currentPoint->brother->visited = true;

                // if (currentPoint)
                // {
                //     currentPoint->visited = true;
                //     currentResultPolygon.append(currentPoint->point); // 添加交点
                //     currentPoint = currentPoint->next;          // 找到下一个交点
                // }
            } while (currentPoint && currentPoint->point != startPoint->point);

            resultPolygon.push_back(currentResultPolygon);
            startPoint = findUnvisitedIntersection(completeMainPolygon);
        }
    }

    // bool isInside(const QPoint &clipEdgeStart, const QPoint &clipEdgeEnd, const QPoint &point)
    // {
    //     return (clipEdgeEnd.x() - clipEdgeStart.x()) * (point.y() - clipEdgeStart.y()) > (clipEdgeEnd.y() - clipEdgeStart.y()) * (point.x() - clipEdgeStart.x());
    // }

    bool isInside(const QPoint &edgeStart, const QPoint &edgeEnd, const QPoint &point)
    {
        // 判断点是否在线段的两端点之间
        bool isBetweenX = (point.x() >= std::min(edgeStart.x(), edgeEnd.x())) && (point.x() <= std::max(edgeStart.x(), edgeEnd.x()));
        bool isBetweenY = (point.y() >= std::min(edgeStart.y(), edgeEnd.y())) && (point.y() <= std::max(edgeStart.y(), edgeEnd.y()));

        return isBetweenX && isBetweenY;
    }

    QPoint computeIntersection(const QPoint &S, const QPoint &E, const QPoint &clipEdgeStart, const QPoint &clipEdgeEnd)
    {
        int x1 = S.x(), y1 = S.y();
        int x2 = E.x(), y2 = E.y();
        int x3 = clipEdgeStart.x(), y3 = clipEdgeStart.y();
        int x4 = clipEdgeEnd.x(), y4 = clipEdgeEnd.y();

        int denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

        if (denom == 0) // 处理竖直线段的情况
        {
            if (x1 == x2)
            {
                int x = x1;
                int y = y3 + (y4 - y3) * (x1 - x3) / (x4 - x3);
                return QPoint(x, y);
            }
            else if (x3 == x4)
            {
                int x = x3;
                int y = y1 + (y2 - y1) * (x3 - x1) / (x2 - x1);
                return QPoint(x, y);
            }
        }

        int nume_a = (x1 * y2 - y1 * x2);
        int nume_b = (x3 * y4 - y3 * x4);

        int x = (nume_a * (x3 - x4) - (x1 - x2) * nume_b) / denom;
        int y = (nume_a * (y3 - y4) - (y1 - y2) * nume_b) / denom;

        return QPoint(x, y);
    }

    // 鞋带公式计算面积，顺便给出方向
    double calculateArea(const QVector<QPoint> &polygon)
    {
        int n = polygon.size();
        double area = 0.0;
        for (int i = 0; i < n; ++i)
        {
            int j = (i + 1) % n;
            area += polygon[i].x() * polygon[j].y();
            area -= polygon[j].x() * polygon[i].y();
        }
        area /= 2.0;
        return area;
    }

    void setToggleButton(QPushButton *button)
    {
        toggleButton = button;
    }
    void setModeLabel(QLabel *label) { modeLabel = label; }

    void restart()
    {
        mainPolygon.clear();
        clipPolygon.clear();
        currentPolygon.clear();
        resultPolygon.clear();
        isMainActive = true;
        toggleButton->setText("切换为裁剪多边形");
        modeLabel->setText("当前模式: 主多边形");
        update();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    PolygonWidget *polygonWidget = new PolygonWidget(&window);

    QPushButton *toggleButton = new QPushButton("切换为裁剪多边形", &window);
    QPushButton *clearButton = new QPushButton("清除", &window);
    QPushButton *undoButton = new QPushButton("撤销", &window);
    QPushButton *clipButtonAction = new QPushButton("裁剪", &window);
    QPushButton *restartButton = new QPushButton("重新开始", &window);

    QLabel *modeLabel = new QLabel("当前模式: 主多边形", &window); // 初始模式标签

    polygonWidget->setToggleButton(toggleButton);
    polygonWidget->setModeLabel(modeLabel);

    QObject::connect(toggleButton, &QPushButton::clicked, polygonWidget, &PolygonWidget::togglePolygon);
    QObject::connect(clearButton, &QPushButton::clicked, polygonWidget, &PolygonWidget::clearPolygons);
    QObject::connect(undoButton, &QPushButton::clicked, polygonWidget, &PolygonWidget::undoLastPoint);
    QObject::connect(clipButtonAction, &QPushButton::clicked, polygonWidget, &PolygonWidget::clipPolygons);
    QObject::connect(restartButton, &QPushButton::clicked, polygonWidget, &PolygonWidget::restart);

    // 布局设置
    QVBoxLayout *mainLayout = new QVBoxLayout;

    // 创建水平布局放置按钮和文本
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(modeLabel); // 添加模式标签
    buttonLayout->addWidget(toggleButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(undoButton);
    buttonLayout->addWidget(clipButtonAction);
    buttonLayout->addWidget(restartButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(polygonWidget);

    window.setLayout(mainLayout);

    window.resize(1200, 800);
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int x = (screenGeometry.width() - window.width()) / 2;
    int y = (screenGeometry.height() - window.height()) / 2;
    window.move(x, y);
    window.show();

    return app.exec();
}
