#include "AdminWindow.h"
#include "ui_adminwindow.h"
#include <QTableWidgetItem>
#include <QMessageBox>

AdminWindow::AdminWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdminWindow),
    service(nullptr)
{
    ui->setupUi(this);

    // Categorías de ejemplo (debes sincronizarlas con la base o API luego)
    ui->cmbCategoryAdmin->addItem("algoritmos");
    ui->cmbCategoryAdmin->addItem("strings");
    ui->cmbCategoryAdmin->addItem("matematica");

    // Configuración inicial de la tabla
    ui->tblCases->setColumnCount(2);
    ui->tblCases->setHorizontalHeaderLabels(QStringList() << "Input" << "Expected");

    // Conexiones de botones
    connect(ui->btnAddCase, &QPushButton::clicked,
            this, &AdminWindow::on_btnAddCase_clicked);

    connect(ui->btnRemoveCase, &QPushButton::clicked,
            this, &AdminWindow::on_btnRemoveCase_clicked);

    connect(ui->btnSaveProblem, &QPushButton::clicked,
            this, &AdminWindow::on_btnSaveProblem_clicked);

    connect(ui->btnClose, &QPushButton::clicked,
            this, &AdminWindow::on_btnClose_clicked);
}

AdminWindow::~AdminWindow() {
    delete ui;
}

void AdminWindow::setService(ProblemService *s) {
    service = s;
}

void AdminWindow::on_btnAddCase_clicked() {
    int row = ui->tblCases->rowCount();
    ui->tblCases->insertRow(row);
    ui->tblCases->setItem(row, 0, new QTableWidgetItem(""));
    ui->tblCases->setItem(row, 1, new QTableWidgetItem(""));
}

void AdminWindow::on_btnRemoveCase_clicked() {
    int row = ui->tblCases->currentRow();
    if (row >= 0) {
        ui->tblCases->removeRow(row);
    }
}

ProblemModel AdminWindow::collectProblemFromUi() const {
    ProblemModel p;
    p.title = ui->txtTitle->text();
    p.description = ui->txtDescriptionAdmin->toPlainText();
    p.category = ui->cmbCategoryAdmin->currentText();

    for (int r = 0; r < ui->tblCases->rowCount(); ++r) {
        QTableWidgetItem *itIn = ui->tblCases->item(r, 0);
        QTableWidgetItem *itExp = ui->tblCases->item(r, 1);

        TestCase t;
        t.input = itIn ? itIn->text() : QString();
        t.expected = itExp ? itExp->text() : QString();

        p.cases.append(t);
    }

    return p;
}

void AdminWindow::on_btnSaveProblem_clicked() {
    if (!service) {
        QMessageBox::warning(this, "Error", "Servicio no configurado");
        return;
    }

    ProblemModel p = collectProblemFromUi();

    // Validación básica
    if (p.title.isEmpty()) {
        QMessageBox::warning(this, "Validación", "El título es obligatorio");
        return;
    }

    service->postProblem(p, [this](const QByteArray &data, int status) {
    qDebug() << "POST status =" << status;
    qDebug() << "POST data =" << data;

    if (status == 201 || status == 200) {
        QMessageBox::information(this, "Éxito", "Problema guardado correctamente");
    } else {
        QMessageBox::warning(this, "Error",
                             "No se pudo guardar: " + QString::fromUtf8(data));
    }
});

}

void AdminWindow::on_btnClose_clicked() {
    close();
}
