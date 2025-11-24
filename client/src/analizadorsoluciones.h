#ifndef ANALIZADORSOLUCIONES_H
#define ANALIZADORSOLUCIONES_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <functional>

struct AnalisisComplejidad {
    QString complejidadTemporal;
    QString complejidadEspacial;
    QString tipoAlgoritmo;
    QString confianza;
};

struct FeedbackLLM {
    QString mensaje;
    QVector<QString> sugerencias;
    QString nivelAyuda;
    bool esOptimo;
};

struct ResultadoAnalisis {
    AnalisisComplejidad complejidad;
    FeedbackLLM feedback;
    bool analisisExitoso;
    QString errorAnalisis;
};

class ClienteHttpLLM : public QObject {
    Q_OBJECT

public:
    explicit ClienteHttpLLM(const QString& apiKey, const QString& apiUrl, QObject* parent = nullptr);

    void enviarPeticion(const QString& prompt,
                        std::function<void(const QString&, bool)> callback,
                        const QString& modelo = "grok-2-latest",
                        double temperatura = 0.7,
                        int maxTokens = 1000);

private:
    QString apiKey_;
    QString apiUrl_;
    QNetworkAccessManager* manager_;

    QJsonObject construirJsonPeticion(const QString& prompt,
                                      const QString& modelo,
                                      double temperatura,
                                      int maxTokens);
};

class AnalizadorSoluciones : public QObject {
    Q_OBJECT

public:
    explicit AnalizadorSoluciones(const QString& apiKey,
                                  const QString& apiUrl,
                                  QObject* parent = nullptr);

    void analizarSolucion(
        const QString& codigoUsuario,
        const QString& estadoEjecucion,
        double tiempoPromedioMs,
        double memoriaMaxMB,
        const QString& enunciadoProblema,
        std::function<void(const ResultadoAnalisis&)> callback
        );

private:
    ClienteHttpLLM* clienteLLM_;

    void analizarComplejidad(const QString& codigo,
                             std::function<void(const AnalisisComplejidad&)> callback);

    void generarFeedback(
        const QString& codigo,
        const QString& estadoEjecucion,
        const AnalisisComplejidad& complejidad,
        const QString& enunciadoProblema,
        std::function<void(const FeedbackLLM&)> callback
        );

    QString construirPrompt(
        const QString& codigo,
        const QString& estadoEjecucion,
        const QString& enunciado
        );

    void parsearRespuestaLLM(const QString& respuestaJson, FeedbackLLM& feedback);
    void parsearAnalisisComplejidad(const QString& respuestaJson, AnalisisComplejidad& analisis);
};

#endif // ANALIZADORSOLUCIONES_H
