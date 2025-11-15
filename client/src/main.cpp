#include <QApplication>
#include "MainWindow.h"

/**
 * Punto de entrada de la aplicación CodeCoach (Interfaz Gráfica)
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("CodeCoach - Interfaz de Práctica");
    window.resize(1000, 700);
    window.show();

    return app.exec();
}
