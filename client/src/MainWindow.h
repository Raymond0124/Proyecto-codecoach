#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include "HttpClient.h"
#include "ProblemModel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBtnSubmitClicked();
    void onBtnAddClicked();
    void onBtnEditClicked();
    void onBtnDeleteClicked();
    void onProblemSelected();
    void onSearchTextChanged(const QString &text);

private:
    Ui::MainWindow *ui;
    HttpClient http;
    QVector<ProblemView> problems;

    void loadProblemsFromServer();
    void populateList(const QVector<ProblemView> &data);
    void showProblem(const ProblemView &p);
};
