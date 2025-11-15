#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Conectar señales a slots
    connect(ui->btnSubmit, &QPushButton::clicked, this, &MainWindow::onBtnSubmitClicked);
    connect(ui->btnAdd, &QPushButton::clicked, this, &MainWindow::onBtnAddClicked);
    connect(ui->btnEdit, &QPushButton::clicked, this, &MainWindow::onBtnEditClicked);
    connect(ui->btnDelete, &QPushButton::clicked, this, &MainWindow::onBtnDeleteClicked);
    connect(ui->listProblems, &QListWidget::itemSelectionChanged, this, &MainWindow::onProblemSelected);
    connect(ui->txtSearch, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    loadProblemsFromServer();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::loadProblemsFromServer() {
    http.get("http://localhost:5000/problems", [this](const QByteArray &data, int status) {
        if (status != 200) {
            QMessageBox::warning(this, "Error", "No se pudieron cargar los problemas.");
            return;
        }

        problems.clear();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray arr = doc.array();

        for (auto val : arr) {
            QJsonObject o = val.toObject();
            ProblemView p;
            p.id = o["_id"].toString();
            p.title = o["title"].toString();
            p.category = o["category"].toString();
            p.statement = o["statement"].toString();
            p.examples = o["examples"].toString();
            p.difficulty = o["difficulty"].toString();
            problems.append(p);
        }

        populateList(problems);
    });
}

void MainWindow::populateList(const QVector<ProblemView> &data) {
    ui->listProblems->clear();
    for (const auto &p : data)
        ui->listProblems->addItem(p.title);
}

void MainWindow::showProblem(const ProblemView &p) {
    ui->txtStatement->setPlainText(p.statement);
    ui->txtExamples->setPlainText(p.examples);
}

void MainWindow::onProblemSelected() {
    int idx = ui->listProblems->currentRow();
    if (idx >= 0 && idx < problems.size())
        showProblem(problems[idx]);
}

void MainWindow::onBtnSubmitClicked() {
    int idx = ui->listProblems->currentRow();
    if (idx < 0) return;

    QString code = ui->txtCode->toPlainText();
    QString id = problems[idx].id;

    QJsonObject json;
    json["id"] = id;
    json["code"] = code;
    QByteArray body = QJsonDocument(json).toJson();

    http.post("http://localhost:5000/submit", body, [this](const QByteArray &data, int status) {
        if (status == 200)
            QMessageBox::information(this, "Resultado", QString::fromUtf8(data));
        else
            QMessageBox::warning(this, "Error", "No se pudo enviar la solución.");
    });
}

void MainWindow::onBtnAddClicked() {
    QString title = QInputDialog::getText(this, "Nuevo problema", "Título:");
    if (title.isEmpty()) return;

    QJsonObject json;
    json["title"] = title;
    json["category"] = "General";
    json["statement"] = "Enunciado aquí...";
    json["examples"] = "Ejemplo aquí...";
    json["difficulty"] = "Fácil";

    http.post("http://localhost:5000/problems", QJsonDocument(json).toJson(), [this](const QByteArray &, int status) {
        if (status == 201) loadProblemsFromServer();
    });
}

void MainWindow::onBtnEditClicked() {
    int idx = ui->listProblems->currentRow();
    if (idx < 0) return;

    QString newTitle = QInputDialog::getText(this, "Editar", "Nuevo título:", QLineEdit::Normal, problems[idx].title);
    if (newTitle.isEmpty()) return;

    QJsonObject json;
    json["title"] = newTitle;
    QByteArray body = QJsonDocument(json).toJson();

    QString url = "http://localhost:5000/problems/" + problems[idx].id;

    http.put(url, body, [this](const QByteArray &, int status) {
        if (status == 200) loadProblemsFromServer();
    });
}

void MainWindow::onBtnDeleteClicked() {
    int idx = ui->listProblems->currentRow();
    if (idx < 0) return;

    QString url = "http://localhost:5000/problems/" + problems[idx].id;

    http.del(url, [this](const QByteArray &, int status) {
        if (status == 200) loadProblemsFromServer();
    });
}

void MainWindow::onSearchTextChanged(const QString &text) {
    QVector<ProblemView> filtered;
    for (const auto &p : problems)
        if (p.title.contains(text, Qt::CaseInsensitive))
            filtered.append(p);

    populateList(filtered);
}
