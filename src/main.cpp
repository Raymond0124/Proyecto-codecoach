#include <iostream>
#include <string>
#include <vector>

#include "httplib.h"
#include "json.hpp"
#include "MotorEvaluacion.h"

using json = nlohmann::json;

// Convierte la cadena del JSON en la política de comparación interna.
static PoliticaComparacion convertirPolitica(const std::string& valor) {
    if (valor == "estricto") {
        return PoliticaComparacion::ESTRICTO;
    }
    return PoliticaComparacion::NORMALIZA_ESPACIOS;
}

int main() {
    httplib::Server servidor;

    const std::string directorioTrabajo = "./_work";
    MotorEvaluacion motor(directorioTrabajo);

    servidor.Post("/evaluate", [&](const httplib::Request& solicitud,
                                   httplib::Response& respuesta) {
        try {
            json cuerpo = json::parse(solicitud.body);

            std::string codigoFuente = cuerpo.at("codigo").get<std::string>();

            LimitesEjecucion limites;
            if (cuerpo.contains("limites")) {
                auto jsonLimites = cuerpo["limites"];
                if (jsonLimites.contains("tiempo_segundos")) {
                    limites.tiempoMaximoSegundos = jsonLimites["tiempo_segundos"];
                }
                if (jsonLimites.contains("memoria_mb")) {
                    limites.memoriaMaximaMB = jsonLimites["memoria_mb"];
                }
            }

            std::vector<CasoPrueba> listaCasos;
            for (const auto& jsonCaso : cuerpo.at("casos")) {
                CasoPrueba caso;
                caso.entrada        = jsonCaso.at("input").get<std::string>();
                caso.salidaEsperada = jsonCaso.at("expected").get<std::string>();
                listaCasos.push_back(caso);
            }

            PoliticaComparacion politica = PoliticaComparacion::NORMALIZA_ESPACIOS;
            if (cuerpo.contains("politica_comparacion")) {
                politica = convertirPolitica(cuerpo["politica_comparacion"].get<std::string>());
            }

            ResultadoGlobal resultado = motor.evaluarCodigoFuente(codigoFuente,
                                                                  listaCasos,
                                                                  limites,
                                                                  politica);

            json jsonRespuesta;
            jsonRespuesta["compilacion"] = {
                {"ok",      resultado.compilacionExitosa},
                {"errores", resultado.erroresCompilacion}
            };
            jsonRespuesta["resumen"] = {
                {"estado",             resultado.estadoGlobal},
                {"tiempo_promedio_ms", resultado.tiempoPromedioMs},
                {"memoria_max_mb",     resultado.memoriaMaximaMB}
            };

            json listaDetalle = json::array();
            for (const auto& rc : resultado.resultadosPorCaso) {
                listaDetalle.push_back({
                    {"case",              rc.indiceCaso},
                    {"estado",            rc.estado},
                    {"tiempo_ms",         rc.tiempoMs},
                    {"memoria_mb",        rc.memoriaMB},
                    {"detalle_mismatch",  rc.detalleDiferencia}
                });
            }
            jsonRespuesta["detallado"] = listaDetalle;

            respuesta.set_content(jsonRespuesta.dump(2), "application/json");
        } catch (const std::exception& e) {
            json jsonError = {
                {"error", std::string("Solicitud inválida: ") + e.what()}
            };
            respuesta.status = 400;
            respuesta.set_content(jsonError.dump(2), "application/json");
        }
    });

    std::cout << "Servidor de motor de evaluaciones escuchando en http://0.0.0.0:8080\n";
    servidor.listen("0.0.0.0", 8080);
}
