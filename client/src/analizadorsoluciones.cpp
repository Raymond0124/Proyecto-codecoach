#include "analizadorsoluciones.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

// ============================================================================
// ClienteHttpLLM - Implementacion con Qt + DEBUG
// ============================================================================

ClienteHttpLLM::ClienteHttpLLM(const QString& apiKey, const QString& apiUrl, QObject* parent)
    : QObject(parent), apiKey_(apiKey), apiUrl_(apiUrl) {
    manager_ = new QNetworkAccessManager(this);
    qDebug() << "[ClienteHttpLLM] Creado con URL:" << apiUrl_;
    qDebug() << "[ClienteHttpLLM] API Key presente:" << (!apiKey_.isEmpty());
    qDebug() << "[ClienteHttpLLM] API Key primeros 15 chars:" << apiKey_.left(15);
}

QJsonObject ClienteHttpLLM::construirJsonPeticion(const QString& prompt,
                                                  const QString& modelo,
                                                  double temperatura,
                                                  int maxTokens) {
    QJsonObject peticion;
    peticion["model"] = modelo;

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "Eres un asistente experto en algoritmos y estructuras de datos. "
                           "Tu rol es ayudar a estudiantes a mejorar sus soluciones de programacion competitiva "
                           "sin darles la respuesta completa. Proporciona pistas, identifica errores conceptuales "
                           "y sugiere mejoras.";
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);

    peticion["messages"] = messages;
    peticion["temperature"] = temperatura;
    peticion["max_tokens"] = maxTokens;

    return peticion;
}

void ClienteHttpLLM::enviarPeticion(const QString& prompt,
                                    std::function<void(const QString&, bool)> callback,
                                    const QString& modelo,
                                    double temperatura,
                                    int maxTokens) {

    qDebug() << "========================================";
    qDebug() << "[ClienteHttpLLM] ENVIANDO PETICION";
    qDebug() << "[ClienteHttpLLM] URL:" << apiUrl_;
    qDebug() << "[ClienteHttpLLM] Modelo:" << modelo;
    qDebug() << "[ClienteHttpLLM] Temperatura:" << temperatura;
    qDebug() << "[ClienteHttpLLM] Max tokens:" << maxTokens;
    qDebug() << "[ClienteHttpLLM] Prompt length:" << prompt.length() << "chars";
    qDebug() << "[ClienteHttpLLM] Prompt preview:" << prompt.left(100);

    QJsonObject jsonPeticion = construirJsonPeticion(prompt, modelo, temperatura, maxTokens);
    QByteArray data = QJsonDocument(jsonPeticion).toJson();

    qDebug() << "[ClienteHttpLLM] Payload size:" << data.size() << "bytes";

    QUrl url(apiUrl_);
    qDebug() << "[ClienteHttpLLM] QUrl creada:" << url.toString();
    qDebug() << "[ClienteHttpLLM] QUrl es valida:" << url.isValid();

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));

    QByteArray authHeader = "Bearer " + apiKey_.toUtf8();
    request.setRawHeader(QByteArray("Authorization"), authHeader);

    qDebug() << "[ClienteHttpLLM] Headers configurados";
    qDebug() << "[ClienteHttpLLM] Content-Type: application/json";
    qDebug() << "[ClienteHttpLLM] Authorization: Bearer [hidden]";

    QNetworkReply* reply = manager_->post(request, data);

    qDebug() << "[ClienteHttpLLM] Peticion POST enviada, esperando respuesta...";

    QObject::connect(reply, &QNetworkReply::finished, this,
                     [reply, callback]() {
                         qDebug() << "========================================";
                         qDebug() << "[ClienteHttpLLM] RESPUESTA RECIBIDA";

                         bool success = (reply->error() == QNetworkReply::NoError);
                         int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                         qDebug() << "[ClienteHttpLLM] Success:" << success;
                         qDebug() << "[ClienteHttpLLM] HTTP Status:" << statusCode;
                         qDebug() << "[ClienteHttpLLM] Error code:" << reply->error();
                         qDebug() << "[ClienteHttpLLM] Error string:" << reply->errorString();

                         QByteArray dataResponse = reply->readAll();
                         qDebug() << "[ClienteHttpLLM] Response size:" << dataResponse.size() << "bytes";

                         QString respuesta = QString::fromUtf8(dataResponse);

                         if (dataResponse.size() > 0) {
                             qDebug() << "[ClienteHttpLLM] Response preview (200 chars):" << respuesta.left(200);
                             qDebug() << "[ClienteHttpLLM] Response complete:" << respuesta;
                         } else {
                             qDebug() << "[ClienteHttpLLM] WARNING: Empty response!";
                         }

                         if (!success) {
                             qWarning() << "[ClienteHttpLLM] ERROR en peticion HTTP:" << reply->errorString();
                             QString errorMsg = reply->errorString();
                             respuesta = QString("{\"error\": \"%1\"}").arg(errorMsg);
                         }

                         qDebug() << "========================================";

                         reply->deleteLater();
                         callback(respuesta, success);
                     }
                     );
}

// ============================================================================
// AnalizadorSoluciones - Implementacion con Qt + DEBUG
// ============================================================================

AnalizadorSoluciones::AnalizadorSoluciones(const QString& apiKey,
                                           const QString& apiUrl,
                                           QObject* parent)
    : QObject(parent) {
    qDebug() << "[AnalizadorSoluciones] Creando analizador...";
    clienteLLM_ = new ClienteHttpLLM(apiKey, apiUrl, this);
    qDebug() << "[AnalizadorSoluciones] Cliente LLM creado exitosamente";
}

void AnalizadorSoluciones::analizarSolucion(
    const QString& codigoUsuario,
    const QString& estadoEjecucion,
    double tiempoPromedioMs,
    double memoriaMaxMB,
    const QString& enunciadoProblema,
    std::function<void(const ResultadoAnalisis&)> callback) {

    qDebug() << "========================================";
    qDebug() << "[AnalizadorSoluciones] INICIANDO ANALISIS";
    qDebug() << "[AnalizadorSoluciones] Estado:" << estadoEjecucion;
    qDebug() << "[AnalizadorSoluciones] Tiempo:" << tiempoPromedioMs << "ms";
    qDebug() << "[AnalizadorSoluciones] Memoria:" << memoriaMaxMB << "MB";
    qDebug() << "[AnalizadorSoluciones] Codigo length:" << codigoUsuario.length();
    qDebug() << "========================================";

    analizarComplejidad(codigoUsuario, [=](const AnalisisComplejidad& complejidad) {
        qDebug() << "[AnalizadorSoluciones] Complejidad analizada:";
        qDebug() << "  - Temporal:" << complejidad.complejidadTemporal;
        qDebug() << "  - Espacial:" << complejidad.complejidadEspacial;
        qDebug() << "  - Algoritmo:" << complejidad.tipoAlgoritmo;
        qDebug() << "  - Confianza:" << complejidad.confianza;

        generarFeedback(codigoUsuario, estadoEjecucion, complejidad,
                        enunciadoProblema,
                        [=](const FeedbackLLM& feedback) {

                            qDebug() << "[AnalizadorSoluciones] Feedback generado:";
                            qDebug() << "  - Mensaje length:" << feedback.mensaje.length();
                            qDebug() << "  - Sugerencias count:" << feedback.sugerencias.size();
                            qDebug() << "  - Nivel:" << feedback.nivelAyuda;
                            qDebug() << "  - Es optimo:" << feedback.esOptimo;

                            ResultadoAnalisis resultado;
                            resultado.complejidad = complejidad;
                            resultado.feedback = feedback;
                            resultado.analisisExitoso = true;

                            qDebug() << "[AnalizadorSoluciones] ANALISIS COMPLETADO EXITOSAMENTE";
                            callback(resultado);
                        });
    });
}

void AnalizadorSoluciones::analizarComplejidad(const QString& codigo,
                                               std::function<void(const AnalisisComplejidad&)> callback) {
    qDebug() << "[AnalizadorSoluciones] Iniciando analisis de complejidad...";

    QString prompt = "Analiza la complejidad algoritmica del siguiente codigo C++.\n\n"
                     "RESPONDE UNICAMENTE CON UN JSON en este formato exacto (sin markdown):\n"
                     "{\n"
                     "  \"complejidad_temporal\": \"O(...)\",\n"
                     "  \"complejidad_espacial\": \"O(...)\",\n"
                     "  \"tipo_algoritmo\": \"nombre del patron algoritmico\",\n"
                     "  \"confianza\": \"alta|media|baja\"\n"
                     "}\n\n"
                     "Codigo a analizar:\n```cpp\n" + codigo + "\n```\n";

    clienteLLM_->enviarPeticion(prompt,
                                [this, callback](const QString& respuesta, bool success) {
                                    qDebug() << "[AnalizadorSoluciones] Parseando analisis de complejidad...";
                                    qDebug() << "[AnalizadorSoluciones] Success:" << success;

                                    AnalisisComplejidad analisis;

                                    if (success) {
                                        parsearAnalisisComplejidad(respuesta, analisis);
                                    } else {
                                        qWarning() << "[AnalizadorSoluciones] ERROR: No se pudo analizar complejidad";
                                        analisis.complejidadTemporal = "O(?)";
                                        analisis.complejidadEspacial = "O(?)";
                                        analisis.tipoAlgoritmo = "Error en analisis";
                                        analisis.confianza = "baja";
                                    }

                                    callback(analisis);
                                },
                                "llama-3.3-70b-versatile", 0.7, 800
                                );
}

QString AnalizadorSoluciones::construirPrompt(
    const QString& codigo,
    const QString& estadoEjecucion,
    const QString& enunciado) {

    QString prompt = "Eres un coach de programacion competitiva. Analiza esta solucion.\n\n";
    prompt += "=== PROBLEMA ===\n" + enunciado + "\n\n";
    prompt += "=== CODIGO DEL ESTUDIANTE ===\n```cpp\n" + codigo + "\n```\n\n";
    prompt += "=== RESULTADO ===\nEstado: " + estadoEjecucion + "\n\n";
    prompt += "=== INSTRUCCIONES ===\n";

    if (estadoEjecucion.contains("Correcto") || estadoEjecucion.contains("OK")) {
        prompt += "La solucion es CORRECTA. Proporciona:\n"
                  "1. Reconocimiento positivo\n"
                  "2. Analisis de eficiencia\n"
                  "3. Sugerencias de optimizacion\n"
                  "NO des soluciones alternativas completas.\n";
    } else if (estadoEjecucion.contains("Incorrecto") || estadoEjecucion.contains("WA")) {
        prompt += "La solucion es INCORRECTA. Proporciona:\n"
                  "1. Tipo de error (logico, caso borde, etc.)\n"
                  "2. Pistas sobre que revisar (NO des la solucion)\n"
                  "3. Preguntas guia\n"
                  "NUNCA des el codigo correcto.\n";
    } else if (estadoEjecucion.contains("TL") || estadoEjecucion.contains("Tiempo")) {
        prompt += "La solucion excede el tiempo (TLE). Proporciona:\n"
                  "1. Por que es lenta\n"
                  "2. Algoritmos mas eficientes\n"
                  "3. Conceptos a investigar\n";
    } else {
        prompt += "Error en ejecucion. Proporciona:\n"
                  "1. Posibles causas\n"
                  "2. Areas a revisar\n"
                  "3. Sugerencias de depuracion\n";
    }

    prompt += "\nRESPONDE EN JSON:\n"
              "{\n"
              "  \"mensaje\": \"feedback en espanol\",\n"
              "  \"sugerencias\": [\"sug1\", \"sug2\"],\n"
              "  \"nivel_ayuda\": \"pista|explicacion|optimizacion\",\n"
              "  \"es_optimo\": true|false\n"
              "}\n";

    return prompt;
}

void AnalizadorSoluciones::generarFeedback(
    const QString& codigo,
    const QString& estadoEjecucion,
    const AnalisisComplejidad& complejidad,
    const QString& enunciadoProblema,
    std::function<void(const FeedbackLLM&)> callback) {

    qDebug() << "[AnalizadorSoluciones] Generando feedback...";

    QString prompt = construirPrompt(codigo, estadoEjecucion, enunciadoProblema);

    clienteLLM_->enviarPeticion(prompt,
                                [this, callback](const QString& respuesta, bool success) {
                                    qDebug() << "[AnalizadorSoluciones] Parseando feedback...";
                                    qDebug() << "[AnalizadorSoluciones] Success:" << success;

                                    FeedbackLLM feedback;

                                    if (success) {
                                        parsearRespuestaLLM(respuesta, feedback);
                                    } else {
                                        qWarning() << "[AnalizadorSoluciones] ERROR: No se pudo generar feedback";
                                        feedback.mensaje = "Error al generar feedback: No se pudo conectar con el LLM";
                                        feedback.nivelAyuda = "error";
                                        feedback.esOptimo = false;
                                    }

                                    callback(feedback);
                                },
                                "llama-3.3-70b-versatile", 0.3, 300
                                );
}

void AnalizadorSoluciones::parsearAnalisisComplejidad(const QString& respuestaJson,
                                                      AnalisisComplejidad& analisis) {
    qDebug() << "[AnalizadorSoluciones] Parseando analisis de complejidad...";

    try {
        QJsonDocument doc = QJsonDocument::fromJson(respuestaJson.toUtf8());

        if (doc.isNull()) {
            qWarning() << "[AnalizadorSoluciones] ERROR: JSON document is null";
            throw std::runtime_error("JSON null");
        }

        QJsonObject obj = doc.object();
        qDebug() << "[AnalizadorSoluciones] JSON object keys:" << obj.keys();

        QString contenido;
        if (obj.contains("choices") && obj["choices"].isArray()) {
            QJsonArray choices = obj["choices"].toArray();
            qDebug() << "[AnalizadorSoluciones] Choices array size:" << choices.size();

            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                if (choice.contains("message")) {
                    QJsonObject message = choice["message"].toObject();
                    contenido = message["content"].toString();
                    qDebug() << "[AnalizadorSoluciones] Content extracted, length:" << contenido.length();
                }
            }
        } else {
            contenido = respuestaJson;
            qDebug() << "[AnalizadorSoluciones] Using raw response as content";
        }

        int inicio = contenido.indexOf("{");
        int fin = contenido.lastIndexOf("}");

        qDebug() << "[AnalizadorSoluciones] JSON bounds: inicio=" << inicio << "fin=" << fin;

        if (inicio != -1 && fin != -1 && fin > inicio) {
            QString jsonStr = contenido.mid(inicio, fin - inicio + 1);
            qDebug() << "[AnalizadorSoluciones] Extracted JSON:" << jsonStr;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
            QJsonObject jsonObj = jsonDoc.object();

            analisis.complejidadTemporal = jsonObj["complejidad_temporal"].toString("O(?)");
            analisis.complejidadEspacial = jsonObj["complejidad_espacial"].toString("O(?)");
            analisis.tipoAlgoritmo = jsonObj["tipo_algoritmo"].toString("Desconocido");
            analisis.confianza = jsonObj["confianza"].toString("baja");

            qDebug() << "[AnalizadorSoluciones] Parse successful!";
        } else {
            qWarning() << "[AnalizadorSoluciones] WARNING: No se encontro JSON valido en la respuesta";
            analisis.complejidadTemporal = "O(?)";
            analisis.complejidadEspacial = "O(?)";
            analisis.tipoAlgoritmo = "Analisis no disponible";
            analisis.confianza = "baja";
        }
    } catch (const std::exception& e) {
        qWarning() << "[AnalizadorSoluciones] EXCEPTION:" << e.what();
        analisis.complejidadTemporal = "O(?)";
        analisis.complejidadEspacial = "O(?)";
        analisis.tipoAlgoritmo = "Error";
        analisis.confianza = "baja";
    }
}

void AnalizadorSoluciones::parsearRespuestaLLM(const QString& respuestaJson,
                                               FeedbackLLM& feedback) {
    qDebug() << "[AnalizadorSoluciones] Parseando feedback LLM...";

    try {
        QJsonDocument doc = QJsonDocument::fromJson(respuestaJson.toUtf8());

        if (doc.isNull()) {
            qWarning() << "[AnalizadorSoluciones] ERROR: JSON document is null";
            throw std::runtime_error("JSON null");
        }

        QJsonObject obj = doc.object();
        qDebug() << "[AnalizadorSoluciones] JSON object keys:" << obj.keys();

        QString contenido;
        if (obj.contains("choices") && obj["choices"].isArray()) {
            QJsonArray choices = obj["choices"].toArray();
            qDebug() << "[AnalizadorSoluciones] Choices array size:" << choices.size();

            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                if (choice.contains("message")) {
                    QJsonObject message = choice["message"].toObject();
                    contenido = message["content"].toString();
                    qDebug() << "[AnalizadorSoluciones] Content extracted, length:" << contenido.length();
                }
            }
        } else {
            contenido = respuestaJson;
            qDebug() << "[AnalizadorSoluciones] Using raw response as content";
        }

        int inicio = contenido.indexOf("{");
        int fin = contenido.lastIndexOf("}");

        qDebug() << "[AnalizadorSoluciones] JSON bounds: inicio=" << inicio << "fin=" << fin;

        if (inicio != -1 && fin != -1 && fin > inicio) {
            QString jsonStr = contenido.mid(inicio, fin - inicio + 1);
            qDebug() << "[AnalizadorSoluciones] Extracted JSON:" << jsonStr;

            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
            QJsonObject jsonObj = jsonDoc.object();

            feedback.mensaje = jsonObj["mensaje"].toString("Analisis completado");
            feedback.nivelAyuda = jsonObj["nivel_ayuda"].toString("explicacion");
            feedback.esOptimo = jsonObj["es_optimo"].toBool(false);

            if (jsonObj.contains("sugerencias") && jsonObj["sugerencias"].isArray()) {
                QJsonArray sugerencias = jsonObj["sugerencias"].toArray();
                qDebug() << "[AnalizadorSoluciones] Sugerencias count:" << sugerencias.size();
                for (const auto& sug : sugerencias) {
                    feedback.sugerencias.append(sug.toString());
                }
            }

            qDebug() << "[AnalizadorSoluciones] Parse successful!";
        } else {
            qWarning() << "[AnalizadorSoluciones] WARNING: No se encontro JSON valido";
            feedback.mensaje = contenido.isEmpty() ? "Analisis completado" : contenido;
            feedback.nivelAyuda = "explicacion";
            feedback.esOptimo = false;
        }
    } catch (const std::exception& e) {
        qWarning() << "[AnalizadorSoluciones] EXCEPTION:" << e.what();
        feedback.mensaje = "Error parseando respuesta";
        feedback.nivelAyuda = "error";
        feedback.esOptimo = false;
    }
}
