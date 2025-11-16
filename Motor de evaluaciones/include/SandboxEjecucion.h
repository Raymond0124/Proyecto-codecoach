#ifndef CODECOACH_SANDBOX_EJECUCION_H
#define CODECOACH_SANDBOX_EJECUCION_H

#include <string>

struct LimitesEjecucion;

class SandboxEjecucion {
public:
    static std::string construirComandoEjecucion(const std::string& rutaEjecutable,
                                                 const std::string& rutaArchivoEntrada,
                                                 const std::string& rutaArchivoSalida,
                                                 const std::string& rutaArchivoError,
                                                 const LimitesEjecucion& limites);

    static void extraerMetricasDesdeTime(const std::string& logError,
                                         double& tiempoMs,
                                         double& memoriaMB);
};

#endif
