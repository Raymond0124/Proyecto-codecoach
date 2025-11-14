#include "EjecutadorCodigo.h"
#include "SandboxEjecucion.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>

namespace fs = std::filesystem;

bool EjecutadorCodigo::compilarConGpp(const std::string& rutaFuente,
                                      const std::string& rutaEjecutable,
                                      std::string& erroresCompilacion) {
    fs::path directorioSalida = fs::path(rutaEjecutable).parent_path();
    fs::create_directories(directorioSalida);

    std::string rutaErrores = (directorioSalida / "compilacion_errores.txt").string();

    std::ostringstream comandoCompilacion;
    comandoCompilacion << "g++ -std=c++17 -O2 "
                       << "\"" << rutaFuente << "\" -o \"" << rutaEjecutable
                       << "\" 2> \"" << rutaErrores << "\"";

    int resultado = std::system(comandoCompilacion.str().c_str());

    std::ifstream archivoErrores(rutaErrores, std::ios::binary);
    std::ostringstream acumuladorErrores;
    acumuladorErrores << archivoErrores.rdbuf();
    erroresCompilacion = acumuladorErrores.str();

    return resultado == 0;
}

MedicionEjecucion EjecutadorCodigo::ejecutarConLimites(const std::string& rutaEjecutable,
                                                       const std::string& entrada,
                                                       const LimitesEjecucion& limites) {
    MedicionEjecucion medicion;

    fs::path directorioTrabajo = fs::path(rutaEjecutable).parent_path();
    std::string rutaEntrada = (directorioTrabajo / "entrada.txt").string();
    std::string rutaSalida = (directorioTrabajo / "salida.txt").string();
    std::string rutaError = (directorioTrabajo / "uso_y_error.txt").string();

    // Guardar entrada en archivo
    {
        std::ofstream archivoEntrada(rutaEntrada, std::ios::binary);
        archivoEntrada << entrada;
    }

    // Construir comando sandbox
    std::string comando = SandboxEjecucion::construirComandoEjecucion(rutaEjecutable,
                                                                      rutaEntrada,
                                                                      rutaSalida,
                                                                      rutaError,
                                                                      limites);

    int resultado = std::system(comando.c_str());
    medicion.codigoSalida = WEXITSTATUS(resultado);

    // Leer stdout
    {
        std::ifstream archivoSalida(rutaSalida, std::ios::binary);
        std::ostringstream acumuladorSalida;
        acumuladorSalida << archivoSalida.rdbuf();
        medicion.salidaEstandar = acumuladorSalida.str();
    }

    // Leer stderr (incluye log de /usr/bin/time -v)
    {
        std::ifstream archivoError(rutaError, std::ios::binary);
        std::ostringstream acumuladorError;
        acumuladorError << archivoError.rdbuf();
        medicion.salidaError = acumuladorError.str();
    }

    // Detectar timeout
    if (WEXITSTATUS(resultado) == 124 ||
        medicion.salidaError.find("Command terminated by signal") != std::string::npos) {
        medicion.seExcedioTiempo = true;
    }

    // Detectar error de ejecución
    if (!medicion.seExcedioTiempo && medicion.codigoSalida != 0) {
        medicion.huboErrorEjecucion = true;
    }

    // Extraer métricas de tiempo y memoria
    SandboxEjecucion::extraerMetricasDesdeTime(medicion.salidaError,
                                               medicion.tiempoMs,
                                               medicion.memoriaMB);

    return medicion;
}
