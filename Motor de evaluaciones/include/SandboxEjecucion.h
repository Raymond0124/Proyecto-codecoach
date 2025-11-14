#ifndef CODECOACH_SANDBOX_EJECUCION_H
#define CODECOACH_SANDBOX_EJECUCION_H

#include <string>
#include "MotorEvaluacion.h"

// Encapsula la construcción del comando que se ejecuta dentro del contenedor
// y el análisis del log generado por /usr/bin/time -v.
class SandboxEjecucion {
public:
    // Construye el comando que aplica timeout y mide recursos.
    static std::string construirComandoEjecucion(const std::string& rutaEjecutable,
                                                 const std::string& rutaArchivoEntrada,
                                                 const std::string& rutaArchivoSalida,
                                                 const std::string& rutaArchivoError,
                                                 const LimitesEjecucion& limites);

    // Extrae tiempo y memoria a partir del texto generado por /usr/bin/time -v.
    static void extraerMetricasDesdeTime(const std::string& logError,
                                         double& tiempoMs,
                                         double& memoriaMB);
};

#endif
