#include "SandboxEjecucion.h"

#include <sstream>
#include <regex>

std::string SandboxEjecucion::construirComandoEjecucion(const std::string& rutaEjecutable,
                                                        const std::string& rutaArchivoEntrada,
                                                        const std::string& rutaArchivoSalida,
                                                        const std::string& rutaArchivoError,
                                                        const LimitesEjecucion& limites) {
    // Usamos /usr/bin/timeout para cortar por tiempo
    // y /usr/bin/time -v para medir tiempo y memoria.
    std::ostringstream comando;
    comando << "/usr/bin/timeout " << limites.tiempoMaximoSegundos << "s "
            << "/usr/bin/time -v "
            << "\"" << rutaEjecutable << "\" < \"" << rutaArchivoEntrada
            << "\" > \"" << rutaArchivoSalida
            << "\" 2> \"" << rutaArchivoError << "\"";
    return comando.str();
}

void SandboxEjecucion::extraerMetricasDesdeTime(const std::string& logError,
                                                double& tiempoMs,
                                                double& memoriaMB) {
    tiempoMs = 0.0;
    memoriaMB = 0.0;

    // Memoria máxima residente en kB.
    {
        std::regex patronMemoria("Maximum resident set size \\(kbytes\\):\\s+(\\d+)");
        std::smatch coincidencia;
        if (std::regex_search(logError, coincidencia, patronMemoria)) {
            double memoriaKB = std::stod(coincidencia[1].str());
            memoriaMB = memoriaKB / 1024.0;
        }
    }

    // Tiempo wall clock. Ejemplos de formato:
    // "0:00.004", "0:01.23", "1:02.34", "0:01"
    {
        std::regex patronTiempo("Elapsed \\(wall clock\\) time .*?:\\s+([0-9:]+)\\.?([0-9]*)");
        std::smatch coincidencia;
        if (std::regex_search(logError, coincidencia, patronTiempo)) {
            std::string partePrincipal = coincidencia[1].str();

            int horas = 0;
            int minutos = 0;
            double segundos = 0.0;

            std::size_t posicionPrimerDosPuntos = partePrincipal.find(':');
            std::size_t posicionUltimoDosPuntos = partePrincipal.rfind(':');

            if (posicionPrimerDosPuntos == std::string::npos) {
                segundos = std::stod(partePrincipal);
            } else if (posicionPrimerDosPuntos == posicionUltimoDosPuntos) {
                // m:ss
                minutos = std::stoi(partePrincipal.substr(0, posicionPrimerDosPuntos));
                segundos = std::stod(partePrincipal.substr(posicionPrimerDosPuntos + 1));
            } else {
                // h:mm:ss
                horas = std::stoi(partePrincipal.substr(0, posicionPrimerDosPuntos));
                minutos = std::stoi(partePrincipal.substr(posicionPrimerDosPuntos + 1,
                                                          posicionUltimoDosPuntos - posicionPrimerDosPuntos - 1));
                segundos = std::stod(partePrincipal.substr(posicionUltimoDosPuntos + 1));
            }

            double totalMs = (horas * 3600.0 + minutos * 60.0 + segundos) * 1000.0;
            tiempoMs = totalMs;
        }
    }
}
