#ifndef PROBLEMMODEL_H
#define PROBLEMMODEL_H

#include <QString>
#include <QList>

struct TestCase {
    QString input;
    QString expected;
};

struct ProblemModel {
    QString title;
    QString description;
    QString category;
    QList<TestCase> cases;
};

#endif // PROBLEMMODEL_H
