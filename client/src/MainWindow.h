#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ProblemModel.h"
#include "ProblemService.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AdminWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnEvaluate_clicked();
    void on_btnNext_clicked();
    void on_btnOpenAdmin_clicked();

private:
    Ui::MainWindow *ui;

    ProblemModel currentProblem;
    ProblemService *service;
    AdminWindow *adminWindow;   // ✔ ventana de administración

    // --- Funciones auxiliares ---
    void loadProblemFromJson(const QByteArray &data);
    QByteArray buildEvaluatePayload() const;
    void showResponse(const QByteArray &data, int status);
};

#endif // MAINWINDOW_H
