#include "ProblemService.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>

ProblemService::ProblemService(QObject *parent)
    : QObject(parent)
{
    http = new HttpClient(this);
    baseUrl = "http://localhost:3000";
}

void ProblemService::setBaseUrl(const QString &url) {
    baseUrl = url;
}

void ProblemService::postProblem(const ProblemModel &p,
                                 std::function<void(const QByteArray&, int)> callback)
{
    QString url = baseUrl + "/problems";

    // Convertir ProblemModel → JSON
    QJsonObject obj;
    obj["title"] = p.title;
    obj["category"] = p.category;
    obj["description"] = p.description;

    QJsonArray arr;
    for (auto &c : p.cases) {
        QJsonObject t;
        t["input"] = c.input;
        t["expected"] = c.expected;
        arr.append(t);
    }
    obj["cases"] = arr;

    QByteArray body = QJsonDocument(obj).toJson();

    // USAR TU HttpClient (NO manager)
    http->post(url, body, callback);
}

void ProblemService::getRandomProblem(const QString &category,
                                      std::function<void(const QByteArray&, int)> callback)
{
    QString url = baseUrl + "/problems/random?category=" +
                  QUrl::toPercentEncoding(category);

    http->get(url, callback);
}
