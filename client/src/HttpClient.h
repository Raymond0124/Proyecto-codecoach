#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <functional>

class HttpClient : public QObject {
    Q_OBJECT
private:
    QNetworkAccessManager *mgr;

public:
    explicit HttpClient(QObject *parent = nullptr) : QObject(parent) {
        mgr = new QNetworkAccessManager(this);
    }

    // ------------------ GET ------------------
    void get(const QString &url, std::function<void(const QByteArray &, int)> callback) {
        QNetworkRequest req{ QUrl(url) };  // ✅ CORREGIDO
        QNetworkReply *rep = mgr->get(req);
        connect(rep, &QNetworkReply::finished, this, [rep, callback]() {
            QByteArray data = rep->readAll();
            int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            rep->deleteLater();
            callback(data, status);
        });
    }

    // ------------------ POST ------------------
    void post(const QString &url, const QByteArray &body, std::function<void(const QByteArray &, int)> callback) {
        QNetworkRequest req{ QUrl(url) };  // ✅ CORREGIDO
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *rep = mgr->post(req, body);
        connect(rep, &QNetworkReply::finished, this, [rep, callback]() {
            QByteArray data = rep->readAll();
            int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            rep->deleteLater();
            callback(data, status);
        });
    }

    // ------------------ PUT ------------------
    void put(const QString &url, const QByteArray &body, std::function<void(const QByteArray &, int)> callback) {
        QNetworkRequest req{ QUrl(url) };  // ✅ CORREGIDO
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *rep = mgr->put(req, body);
        connect(rep, &QNetworkReply::finished, this, [rep, callback]() {
            QByteArray data = rep->readAll();
            int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            rep->deleteLater();
            callback(data, status);
        });
    }

    // ------------------ DELETE ------------------
    void del(const QString &url, std::function<void(const QByteArray &, int)> callback) {
        QNetworkRequest req{ QUrl(url) };  // ✅ CORREGIDO
        QNetworkReply *rep = mgr->deleteResource(req);
        connect(rep, &QNetworkReply::finished, this, [rep, callback]() {
            QByteArray data = rep->readAll();
            int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            rep->deleteLater();
            callback(data, status);
        });
    }
};

#endif // HTTPCLIENT_H
