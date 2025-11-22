#include "../include/MotorEvaluacion.h"
#include "../include/EjecutadorCodigo.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

MotorEvaluacion::MotorEvaluacion(std::string directorioTrabajo)
        : directorioTrabajo_(std::move(directorioTrabajo)) {
    fs::create_directories(directorioTrabajo_);
}

std::string MotorEvaluacion::guardarCodigoFuenteTemporal(const std::string& codigoFuente) const {
    fs::path rutaFuente = fs::path(directorioTrabajo_) / "solucion.cpp";
    escribirArchivoCompleto(rutaFuente.string(), codigoFuente);

    std::cout << "[Motor] Código guardado en: " << rutaFuente.string() << "\n";
    return rutaFuente.string();
}

bool MotorEvaluacion::compilarCodigoUsuario(const std::string& rutaFuente,
                                            std::string& rutaEjecutable,
                                            std::string& erroresCompilacion) const {

    rutaEjecutable = (fs::path(directorioTrabajo_) / "solucion.exe").string();

    std::cout << "[Motor] Compilando a ejecutable: " << rutaEjecutable << "\n";

    return EjecutadorCodigo::compilarConGpp(rutaFuente, rutaEjecutable, erroresCompilacion);
}

ResultadoCaso MotorEvaluacion::ejecutarYCompararCaso(const std::string& rutaEjecutable,
                                                     const CasoPrueba& caso,
                                                     const LimitesEjecucion& limites,
                                                     PoliticaComparacion politica,
                                                     int indiceCaso) const {

    ResultadoCaso resultado;
    resultado.indiceCaso = indiceCaso;

    std::cout << "[Motor] ---- Ejecutando caso #" << indiceCaso << " ----\n";

    MedicionEjecucion medicion = EjecutadorCodigo::ejecutarConLimites(
            rutaEjecutable, caso.entrada, limites);

    resultado.tiempoMs = medicion.tiempoMs;
    resultado.memoriaMB = medicion.memoriaMB;

    if (medicion.seExcedioTiempo) {
        std::cout << "[Motor] Caso #" << indiceCaso << ": TIME-LIMIT\n";
        resultado.estado = "TL";
        return resultado;
    }

    if (medicion.huboErrorEjecucion || medicion.codigoSalida != 0) {
        std::cout << "[Motor] Caso #" << indiceCaso << ": ERROR DE EJECUCIÓN\n"
                  << "  Exit code: " << medicion.codigoSalida << "\n"
                  << "  stdout:\n" << medicion.salidaEstandar << "\n"
                  << "  stderr:\n" << medicion.salidaError << "\n";
        resultado.estado = "RE";
        return resultado;
    }

    bool ok = false;
    std::string detalle;

    if (politica == PoliticaComparacion::ESTRICTO) {
        ok = compararSalidaEstricta(medicion.salidaEstandar, caso.salidaEsperada);
    } else {
        ok = compararSalidaNormalizada(
                medicion.salidaEstandar, caso.salidaEsperada, detalle);
    }

    resultado.estado = ok ? "OK" : "INCORRECTO";
    resultado.detalleDiferencia = detalle;

    std::cout << "[Motor] Caso #" << indiceCaso
              << " => " << resultado.estado << "\n";

    return resultado;
}

ResultadoGlobal MotorEvaluacion::evaluarCodigoFuente(const std::string& codigoFuente,
                                                     const std::vector<CasoPrueba>& listaCasos,
                                                     const LimitesEjecucion& limites,
                                                     PoliticaComparacion politica) {

    ResultadoGlobal global;

    std::cout << "[Motor] ===== Iniciando evaluación =====\n";

    std::string rutaFuente = guardarCodigoFuenteTemporal(codigoFuente);

    std::string rutaEjecutable;
    std::string erroresCompilacion;

    global.compilacionExitosa =
            compilarCodigoUsuario(rutaFuente, rutaEjecutable, erroresCompilacion);
    global.erroresCompilacion = erroresCompilacion;

    if (!global.compilacionExitosa) {
        global.estadoGlobal = "Error de compilación";
        std::cout << "[Motor] ERROR de compilación:\n"
                  << erroresCompilacion << "\n";
        return global;
    }

    double sumaTiempos = 0;
    double maxMem = 0;

    for (int i = 0; i < (int)listaCasos.size(); i++) {

        ResultadoCaso rc = ejecutarYCompararCaso(
                rutaEjecutable, listaCasos[i], limites, politica, i + 1);

        sumaTiempos += rc.tiempoMs;
        maxMem = std::max(maxMem, rc.memoriaMB);

        global.resultadosPorCaso.push_back(rc);
    }

    global.tiempoPromedioMs = sumaTiempos / listaCasos.size();
    global.memoriaMaximaMB = maxMem;

    bool hayRE = false, hayTL = false, hayINC = false;

    for (auto& r : global.resultadosPorCaso) {
        if (r.estado == "RE") hayRE = true;
        if (r.estado == "TL") hayTL = true;
        if (r.estado == "INCORRECTO") hayINC = true;
    }

    if (hayRE)      global.estadoGlobal = "Error de ejecución";
    else if (hayTL) global.estadoGlobal = "Tiempo excedido";
    else if (hayINC) global.estadoGlobal = "Incorrecto";
    else             global.estadoGlobal = "Correcto";

    std::cout << "[Motor] ===== FIN evaluación =====\n"
              << "  Estado global: " << global.estadoGlobal << "\n"
              << "  Tiempo promedio: " << global.tiempoPromedioMs << " ms\n"
              << "  Memoria máxima: " << global.memoriaMaximaMB << " MB\n";

    return global;
}

bool MotorEvaluacion::compararSalidaEstricta(const std::string& salidaObtenida,
                                             const std::string& salidaEsperada) {
    return salidaObtenida == salidaEsperada;
}

static std::string normalizar(const std::string& texto) {
    std::string out;
    bool esp = false;

    for (char c : texto) {
        if (isspace((unsigned char)c)) {
            if (!esp) {
                out.push_back(' ');
                esp = true;
            }
        } else {
            out.push_back(c);
            esp = false;
        }
    }

    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());

    return out;
}

bool MotorEvaluacion::compararSalidaNormalizada(const std::string& salidaObtenida,
                                                const std::string& salidaEsperada,
                                                std::string& pista) {
    std::string o = normalizar(salidaObtenida);
    std::string e = normalizar(salidaEsperada);

    if (o == e) return true;

    pista = "Salida distinta. Obtenido=[" + o + "] Esperado=[" + e + "]";
    return false;
}

std::string MotorEvaluacion::leerArchivoCompleto(const std::string& rutaArchivo) {
    std::ifstream f(rutaArchivo, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void MotorEvaluacion::escribirArchivoCompleto(const std::string& rutaArchivo,
                                              const std::string& contenido) {
    std::ofstream f(rutaArchivo, std::ios::binary);
    f << contenido;
}
