#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "AdminWindow.h"

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
    service->setBaseUrl("http://localhost:3000"); // cambia si tu API está en otra URL

    // llenar combo con categorías ejemplo
    ui->cmbCategory->addItem("algoritmos");
    ui->cmbCategory->addItem("strings");
    ui->cmbCategory->addItem("matematica");

    connect(ui->btnEvaluate, &QPushButton::clicked, this, &MainWindow::on_btnEvaluate_clicked);
    connect(ui->btnNext, &QPushButton::clicked, this, &MainWindow::on_btnNext_clicked);
    connect(ui->btnOpenAdmin, &QPushButton::clicked, this, &MainWindow::on_btnOpenAdmin_clicked);

    // cargar primer problema automáticamente (categoria por defecto)
    on_btnNext_clicked();
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
        ui->lblTitle->setText("Respuesta inválida");
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
    QByteArray body = buildEvaluatePayload();
    ui->txtOutput->setPlainText("Enviando al motor...");

    // motor asume en :8080 /evaluate
    QString url = "http://localhost:8080/evaluate";
    // usamos HttpClient internamente a través de ProblemService? Simple: crear temporario
    HttpClient *hc = new HttpClient(this);
    hc->post(url, body, [this, hc](const QByteArray &data, int status) {
        showResponse(data, status);
        hc->deleteLater();
    });
}

void MainWindow::showResponse(const QByteArray &data, int status) {
    if (status != 200) {
        ui->txtOutput->setPlainText("Error del motor: " + QString::number(status) + "\n" + QString::fromUtf8(data));
        return;
    }
    // mostrar JSON bonito
    QJsonDocument doc = QJsonDocument::fromJson(data);
    ui->txtOutput->setPlainText(doc.toJson(QJsonDocument::Indented));
}

void MainWindow::on_btnOpenAdmin_clicked() {
    AdminWindow *w = new AdminWindow(this);
    w->setService(service);
    w->show();
}
MainWindow::~MainWindow()
{
    delete ui;
}
