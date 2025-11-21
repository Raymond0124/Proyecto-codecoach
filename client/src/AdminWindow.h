#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QDialog>
#include "ProblemModel.h"
#include "ProblemService.h"

namespace Ui {
class AdminWindow;
}

class AdminWindow : public QDialog {
    Q_OBJECT

public:
    explicit AdminWindow(QWidget *parent = nullptr);
    ~AdminWindow();

    void setService(ProblemService *service);

private slots:
    void on_btnAddCase_clicked();
    void on_btnRemoveCase_clicked();
    void on_btnSaveProblem_clicked();
    void on_btnClose_clicked();

private:
    Ui::AdminWindow *ui;
    ProblemService *service;

    ProblemModel collectProblemFromUi() const;
};

#endif // ADMINWINDOW_H
