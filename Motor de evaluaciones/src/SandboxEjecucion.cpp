#include "../include/SandboxEjecucion.h"
#include <sstream>

std::string SandboxEjecucion::construirComandoEjecucion(const std::string& rutaEjecutable,
                                                        const std::string& rutaArchivoEntrada,
                                                        const std::string& rutaArchivoSalida,
                                                        const std::string& rutaArchivoError,
                                                        const LimitesEjecucion& /*limites*/) {
    // Esta función ya no se usa en la versión Windows.
    // La ejecución se hace directamente en EjecutadorCodigo.
    // Devolvemos un comando simple por si algún día quieres reutilizarla.
    std::ostringstream comando;
    comando << "\"" << rutaEjecutable << "\""
            << " < \"" << rutaArchivoEntrada << "\""
            << " > \"" << rutaArchivoSalida << "\""
            << " 2> \"" << rutaArchivoError << "\"";
    return comando.str();
}

void SandboxEjecucion::extraerMetricasDesdeTime(const std::string& /*logError*/,
                                                double& tiempoMs,
                                                double& memoriaMB) {
    // Sin /usr/bin/time no extraemos nada aquí.
    tiempoMs = 0.0;
    memoriaMB = 0.0;
}
