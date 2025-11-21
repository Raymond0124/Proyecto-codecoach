#ifndef PROBLEMSERVICE_H
#define PROBLEMSERVICE_H

#include <QObject>
#include <functional>
#include "HttpClient.h"
#include "ProblemModel.h"

class ProblemService : public QObject {
    Q_OBJECT
public:
    explicit ProblemService(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);

    // POST /problems
    void postProblem(const ProblemModel &problem,
                     std::function<void(const QByteArray&, int)> cb);

    // GET /problems/:category
    void getProblem(const QString &category,
                    std::function<void(const QByteArray&, int)> cb);

    // GET /problems/random?category=X
    void getRandomProblem(const QString &category,
                          std::function<void(const QByteArray&, int)> cb);

private:
    HttpClient *http;
    QString baseUrl;
};

#endif // PROBLEMSERVICE_H
