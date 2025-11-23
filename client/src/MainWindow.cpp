#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "AdminWindow.h"
#include "analizadorsoluciones.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    service = new ProblemService(this);
    service->setBaseUrl("http://localhost:3000");

    // ========== GROQ API (GRATIS) ==========
    qDebug() << "========================================";
    qDebug() << "[MainWindow] INICIALIZANDO ANALIZADOR CON GROQ";

    QString apiKey = "x";
    QString apiUrl = "https://api.groq.com/openai/v1/chat/completions";

    qDebug() << "[MainWindow] API Key length:" << apiKey.length();
    qDebug() << "[MainWindow] API URL:" << apiUrl;

    analizador = new AnalizadorSoluciones(apiKey, apiUrl, this);

    qDebug() << "[MainWindow] Analizador Groq inicializado correctamente";
    qDebug() << "========================================";
    // ========================================

    // llenar combo con categorias
    ui->cmbCategory->addItem("algoritmos");
    ui->cmbCategory->addItem("strings");
    ui->cmbCategory->addItem("matematica");

    connect(ui->btnEvaluate, &QPushButton::clicked, this, &MainWindow::on_btnEvaluate_clicked);
    connect(ui->btnNext, &QPushButton::clicked, this, &MainWindow::on_btnNext_clicked);
    connect(ui->btnOpenAdmin, &QPushButton::clicked, this, &MainWindow::on_btnOpenAdmin_clicked);

    // cargar primer problema
    on_btnNext_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnNext_clicked() {
    QString cat = ui->cmbCategory->currentText();
    ui->lblTitle->setText("Cargando problema...");
    ui->txtDescription->setPlainText("");
    ui->txtCode->clear();
    ui->txtOutput->clear();

    service->getRandomProblem(cat, [this](const QByteArray &data, int status){
        if (status != 200) {
            ui->lblTitle->setText("Error al cargar problema");
            ui->txtDescription->setPlainText(QString::fromUtf8(data));
            return;
        }
        loadProblemFromJson(data);
    });
}

void MainWindow::loadProblemFromJson(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        ui->lblTitle->setText("Respuesta invalida");
        return;
    }
    QJsonObject obj = doc.object();
    currentProblem.title = obj.value("title").toString();
    currentProblem.description = obj.value("description").toString();
    currentProblem.category = obj.value("category").toString();
    currentProblem.cases.clear();

    QJsonArray cases = obj.value("cases").toArray();
    for (auto v : cases) {
        QJsonObject c = v.toObject();
        TestCase t;
        t.input = c.value("input").toString();
        t.expected = c.value("expected").toString();
        currentProblem.cases.append(t);
    }

    ui->lblTitle->setText(currentProblem.title + "  [" + currentProblem.category + "]");
    ui->txtDescription->setPlainText(currentProblem.description);
}

QByteArray MainWindow::buildEvaluatePayload() const {
    QJsonObject root;
    root["codigo"] = ui->txtCode->toPlainText();

    QJsonArray arr;
    for (const auto &c : currentProblem.cases) {
        QJsonObject o;
        o["input"] = c.input;
        o["expected"] = c.expected;
        arr.append(o);
    }
    root["casos"] = arr;

    QJsonObject limits;
    limits["tiempo_segundos"] = 2;
    limits["memoria_mb"] = 256;
    root["limites"] = limits;

    root["politica_comparacion"] = "normalizado";

    return QJsonDocument(root).toJson();
}

void MainWindow::on_btnEvaluate_clicked() {
    qDebug() << "========================================";
    qDebug() << "[MainWindow] BOTON EVALUATE PRESIONADO";

    QString codigo = ui->txtCode->toPlainText();
    if (codigo.isEmpty()) {
        QMessageBox::warning(this, "Error", "Escribe tu codigo primero.");
        return;
    }

    qDebug() << "[MainWindow] Codigo length:" << codigo.length();
    qDebug() << "[MainWindow] Problema:" << currentProblem.title;
    qDebug() << "[MainWindow] Casos de prueba:" << currentProblem.cases.size();

    QByteArray body = buildEvaluatePayload();
    ui->txtOutput->setPlainText("Enviando al motor...");

    QString url = "http://localhost:8090/evaluate";
    qDebug() << "[MainWindow] URL del motor:" << url;

    HttpClient *hc = new HttpClient(this);

    hc->post(url, body, [this, hc, codigo](const QByteArray &data, int status) {
        qDebug() << "========================================";
        qDebug() << "[MainWindow] RESPUESTA DEL MOTOR RECIBIDA";
        qDebug() << "[MainWindow] HTTP Status:" << status;
        qDebug() << "[MainWindow] Response size:" << data.size() << "bytes";

        if (status != 200) {
            ui->txtOutput->setPlainText("Error del motor: " + QString::number(status) +
                                        "\n" + QString::fromUtf8(data));
            hc->deleteLater();
            return;
        }

        // Parsear respuesta del motor
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        qDebug() << "[MainWindow] JSON parseado correctamente";

        // Extraer estado global
        QString estado = "Desconocido";
        double tiempo = 0.0;
        double memoria = 0.0;

        if (obj.contains("resumen")) {
            QJsonObject resumen = obj["resumen"].toObject();
            estado = resumen["estado"].toString();
            tiempo = resumen["tiempo_promedio_ms"].toDouble();
            memoria = resumen["memoria_max_mb"].toDouble();

            qDebug() << "[MainWindow] Estado extraido:" << estado;
            qDebug() << "[MainWindow] Tiempo extraido:" << tiempo;
            qDebug() << "[MainWindow] Memoria extraida:" << memoria;
        } else {
            qWarning() << "[MainWindow] WARNING: No se encontro 'resumen' en la respuesta";
        }

        // ========== LLAMAR AL ANALIZADOR ==========
        if (analizador) {
            qDebug() << "========================================";
            qDebug() << "[MainWindow] ENVIANDO AL ANALIZADOR";
            qDebug() << "[MainWindow] Codigo para analizar:" << codigo.length() << "chars";
            qDebug() << "[MainWindow] Estado:" << estado;
            qDebug() << "[MainWindow] Enunciado:" << currentProblem.description.left(50) << "...";

            analizador->analizarSolucion(
                codigo,
                estado,
                tiempo,
                memoria,
                currentProblem.description,
                [this, data](const ResultadoAnalisis& analisis) {
                    qDebug() << "========================================";
                    qDebug() << "[MainWindow] ANALISIS RECIBIDO";
                    qDebug() << "[MainWindow] Analisis exitoso:" << analisis.analisisExitoso;

                    if (analisis.analisisExitoso) {
                        qDebug() << "[MainWindow] Complejidad temporal:" << analisis.complejidad.complejidadTemporal;
                        qDebug() << "[MainWindow] Feedback length:" << analisis.feedback.mensaje.length();
                    } else {
                        qDebug() << "[MainWindow] Error analisis:" << analisis.errorAnalisis;
                    }

                    QString resultadoMotor = QJsonDocument::fromJson(data).toJson(QJsonDocument::Indented);
                    mostrarResultadosCompletos(resultadoMotor, analisis);

                    qDebug() << "========================================";
                }
                );
        } else {
            qWarning() << "[MainWindow] WARNING: Analizador es NULL!";
            showResponse(data, status);
        }

        hc->deleteLater();
    });
}

void MainWindow::showResponse(const QByteArray &data, int status) {
    if (status != 200) {
        ui->txtOutput->setPlainText("Error del motor: " + QString::number(status) +
                                    "\n" + QString::fromUtf8(data));
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(data);
    ui->txtOutput->setPlainText(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::mostrarResultadosCompletos(const QString& resultadoMotor, const ResultadoAnalisis& analisis) {
    qDebug() << "[MainWindow] Mostrando resultados completos en UI...";

    QString output;

    output += "=====================================\n";
    output += "      RESULTADO DEL MOTOR\n";
    output += "=====================================\n";
    output += resultadoMotor + "\n\n";

    if (analisis.analisisExitoso) {
        output += "=====================================\n";
        output += "    ANALISIS DE COMPLEJIDAD\n";
        output += "=====================================\n";
        output += "Temporal: " + analisis.complejidad.complejidadTemporal + "\n";
        output += "Espacial: " + analisis.complejidad.complejidadEspacial + "\n";
        output += "Algoritmo: " + analisis.complejidad.tipoAlgoritmo + "\n";
        output += "Confianza: " + analisis.complejidad.confianza + "\n\n";

        output += "=====================================\n";
        output += "       FEEDBACK DEL COACH AI\n";
        output += "=====================================\n";
        output += analisis.feedback.mensaje + "\n\n";

        if (!analisis.feedback.sugerencias.isEmpty()) {
            output += "SUGERENCIAS:\n";
            for (const auto& sug : analisis.feedback.sugerencias) {
                output += "  * " + sug + "\n";
            }
        }

        if (analisis.feedback.esOptimo) {
            output += "\n[Tu solucion es OPTIMA!]\n";
        }
    } else {
        output += "Error en analisis: " + analisis.errorAnalisis + "\n";
    }

    ui->txtOutput->setPlainText(output);
    qDebug() << "[MainWindow] Resultados mostrados en UI";
}

void MainWindow::on_btnOpenAdmin_clicked() {
    AdminWindow *w = new AdminWindow(this);
    w->setService(service);
    w->show();
}
