#include "MotorEvaluacion.h"
#include "EjecutadorCodigo.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

MotorEvaluacion::MotorEvaluacion(std::string directorioTrabajo)
    : directorioTrabajo_(std::move(directorioTrabajo)) {
    fs::create_directories(directorioTrabajo_);
}

std::string MotorEvaluacion::guardarCodigoFuenteTemporal(const std::string& codigoFuente) const {
    fs::path rutaFuente = fs::path(directorioTrabajo_) / "solucion.cpp";
    escribirArchivoCompleto(rutaFuente.string(), codigoFuente);
    return rutaFuente.string();
}

bool MotorEvaluacion::compilarCodigoUsuario(const std::string& rutaFuente,
                                            std::string& rutaEjecutable,
                                            std::string& erroresCompilacion) const {
    rutaEjecutable = (fs::path(directorioTrabajo_) / "solucion.out").string();
    return EjecutadorCodigo::compilarConGpp(rutaFuente, rutaEjecutable, erroresCompilacion);
}

ResultadoCaso MotorEvaluacion::ejecutarYCompararCaso(const std::string& rutaEjecutable,
                                                     const CasoPrueba& caso,
                                                     const LimitesEjecucion& limites,
                                                     PoliticaComparacion politica,
                                                     int indiceCaso) const {
    ResultadoCaso resultado;
    resultado.indiceCaso = indiceCaso;

    MedicionEjecucion medicion = EjecutadorCodigo::ejecutarConLimites(rutaEjecutable,
                                                                       caso.entrada,
                                                                       limites);

    resultado.tiempoMs = medicion.tiempoMs;
    resultado.memoriaMB = medicion.memoriaMB;

    if (medicion.seExcedioTiempo) {
        resultado.estado = "TL";
        return resultado;
    }

    if (medicion.huboErrorEjecucion || medicion.codigoSalida != 0) {
        resultado.estado = "RE";
        return resultado;
    }

    bool sonIguales = false;
    std::string detalle;

    if (politica == PoliticaComparacion::ESTRICTO) {
        sonIguales = compararSalidaEstricta(medicion.salidaEstandar, caso.salidaEsperada);
        if (!sonIguales) {
            detalle = "Salida distinta en comparación estricta.";
        }
    } else {
        sonIguales = compararSalidaNormalizada(medicion.salidaEstandar,
                                               caso.salidaEsperada,
                                               detalle);
    }

    if (sonIguales) {
        resultado.estado = "OK";
    } else {
        resultado.estado = "INCORRECTO";
        resultado.detalleDiferencia = detalle;
    }

    return resultado;
}

ResultadoGlobal MotorEvaluacion::evaluarCodigoFuente(const std::string& codigoFuente,
                                                     const std::vector<CasoPrueba>& listaCasos,
                                                     const LimitesEjecucion& limites,
                                                     PoliticaComparacion politica) {
    ResultadoGlobal global;

    // 1. Guardar código fuente temporalmente
    std::string rutaFuente = guardarCodigoFuenteTemporal(codigoFuente);

    // 2. Compilar
    std::string rutaEjecutable;
    std::string erroresCompilacion;
    global.compilacionExitosa = compilarCodigoUsuario(rutaFuente,
                                                      rutaEjecutable,
                                                      erroresCompilacion);
    global.erroresCompilacion = erroresCompilacion;

    if (!global.compilacionExitosa) {
        global.estadoGlobal = "Error de compilación";
        return global;
    }

    // 3. Ejecutar y comparar cada caso
    double sumaTiemposMs = 0.0;
    double memoriaMaximaMB = 0.0;
    bool huboIncorrectos = false;
    bool huboTimeout = false;
    bool huboErroresEjecucion = false;

    int indice = 1;
    for (const auto& caso : listaCasos) {
        ResultadoCaso resultadoCaso = ejecutarYCompararCaso(rutaEjecutable,
                                                            caso,
                                                            limites,
                                                            politica,
                                                            indice++);
        global.resultadosPorCaso.push_back(resultadoCaso);

        sumaTiemposMs += resultadoCaso.tiempoMs;
        memoriaMaximaMB = std::max(memoriaMaximaMB, resultadoCaso.memoriaMB);

        if (resultadoCaso.estado == "INCORRECTO") huboIncorrectos = true;
        if (resultadoCaso.estado == "TL")          huboTimeout = true;
        if (resultadoCaso.estado == "RE")          huboErroresEjecucion = true;
    }

    if (!global.resultadosPorCaso.empty()) {
        global.tiempoPromedioMs = sumaTiemposMs / global.resultadosPorCaso.size();
        global.memoriaMaximaMB = memoriaMaximaMB;
    }

    if (huboErroresEjecucion) {
        global.estadoGlobal = "Error de ejecución";
    } else if (huboTimeout) {
        global.estadoGlobal = "Tiempo excedido";
    } else if (huboIncorrectos) {
        global.estadoGlobal = "Incorrecto";
    } else {
        global.estadoGlobal = "Correcto";
    }

    return global;
}

bool MotorEvaluacion::compararSalidaEstricta(const std::string& salidaObtenida,
                                             const std::string& salidaEsperada) {
    return salidaObtenida == salidaEsperada;
}

static std::string normalizarEspaciosInterno(const std::string& texto) {
    std::string resultado;
    resultado.reserve(texto.size());

    bool enBlanco = false;
    for (char c : texto) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!enBlanco) {
                resultado.push_back(' ');
                enBlanco = true;
            }
        } else {
            resultado.push_back(c);
            enBlanco = false;
        }
    }

    // Quitar espacios al final
    while (!resultado.empty() &&
           std::isspace(static_cast<unsigned char>(resultado.back()))) {
        resultado.pop_back();
    }

    // Quitar espacios al inicio
    std::size_t inicio = 0;
    while (inicio < resultado.size() &&
           std::isspace(static_cast<unsigned char>(resultado[inicio]))) {
        ++inicio;
    }

    return resultado.substr(inicio);
}

bool MotorEvaluacion::compararSalidaNormalizada(const std::string& salidaObtenida,
                                                const std::string& salidaEsperada,
                                                std::string& pistaDiferencia) {
    std::string normalizadaObtenida = normalizarEspaciosInterno(salidaObtenida);
    std::string normalizadaEsperada = normalizarEspaciosInterno(salidaEsperada);

    if (normalizadaObtenida == normalizadaEsperada) {
        return true;
    }

    pistaDiferencia = "Salida normalizada distinta. Obtenido: [" +
                      normalizadaObtenida + "], esperado: [" +
                      normalizadaEsperada + "].";
    return false;
}

std::string MotorEvaluacion::leerArchivoCompleto(const std::string& rutaArchivo) {
    std::ifstream archivo(rutaArchivo, std::ios::binary);
    std::ostringstream contenido;
    contenido << archivo.rdbuf();
    return contenido.str();
}

void MotorEvaluacion::escribirArchivoCompleto(const std::string& rutaArchivo,
                                              const std::string& contenido) {
    std::ofstream archivo(rutaArchivo, std::ios::binary);
    archivo << contenido;
    archivo.flush();
}
