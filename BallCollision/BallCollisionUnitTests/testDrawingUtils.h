#ifndef TEST_DRAWING_UTILS_H
#define TEST_DRAWING_UTILS_H

#include <QTest>

class TestDrawingUtils: public QObject
{
    Q_OBJECT

public:
    explicit TestDrawingUtils(QObject *parent = 0): QObject(parent){
    };

private slots:
    void testMoveBall01();
    void testMoveBall02();
    void testMoveBall03();
    void testMoveBall04();

    void testPartitioning01();
    void testPartitioning02();
    void testPartitioning03();
    void testPartitioning04();

};

#endif // TEST_DRAWING_UTILS_H

