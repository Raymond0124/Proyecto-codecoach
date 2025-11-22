#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../third_party/json.hpp"
#include "../include/MotorEvaluacion.h"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

class ServidorHttpBasico {
public:
    ServidorHttpBasico(unsigned short puerto, MotorEvaluacion& motor)
        : puerto_(puerto), motor_(motor), socketEscucha_(INVALID_SOCKET) {}

    void iniciar() {
        inicializarWinsock();
        crearSocketEscucha();

        std::cout << "Servidor HTTP basico escuchando en http://0.0.0.0:"
                  << puerto_ << "\n";

        while (true) {
            sockaddr_in direccionCliente{};
            int tamDireccion = sizeof(direccionCliente);

            SOCKET socketCliente = accept(socketEscucha_,
                                          reinterpret_cast<sockaddr*>(&direccionCliente),
                                          &tamDireccion);
            if (socketCliente == INVALID_SOCKET) {
                std::cerr << "Error en accept(). Codigo: "
                          << WSAGetLastError() << "\n";
                break;
            }

            // Log de la conexión entrante
            char ipStr[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &direccionCliente.sin_addr,
                      ipStr, sizeof(ipStr));
            unsigned short puertoRemoto = ntohs(direccionCliente.sin_port);

            std::cout << "\n[NUEVA CONEXION] "
                      << ipStr << ":" << puertoRemoto
                      << " (socket=" << socketCliente << ")\n";

            manejarCliente(socketCliente);

            std::cout << "[CONEXION CERRADA] socket="
                      << socketCliente << "\n";

            closesocket(socketCliente);
        }

        closesocket(socketEscucha_);
        WSACleanup();
    }

private:
    unsigned short puerto_;
    MotorEvaluacion& motor_;
    SOCKET socketEscucha_;
    unsigned long contadorSolicitudes_ = 0;

    void inicializarWinsock() {
        WSADATA datosWinsock;
        int resultado = WSAStartup(MAKEWORD(2, 2), &datosWinsock);
        if (resultado != 0) {
            throw std::runtime_error(
                "No se pudo inicializar Winsock. Codigo: "
                + std::to_string(resultado));
        }
    }

    void crearSocketEscucha() {
        socketEscucha_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketEscucha_ == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error(
                "No se pudo crear el socket de escucha.");
        }

        sockaddr_in direccionServidor{};
        direccionServidor.sin_family = AF_INET;
        direccionServidor.sin_port   = htons(puerto_);
        direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(socketEscucha_,
                 reinterpret_cast<sockaddr*>(&direccionServidor),
                 sizeof(direccionServidor)) == SOCKET_ERROR) {
            closesocket(socketEscucha_);
            WSACleanup();
            throw std::runtime_error(
                "No se pudo hacer bind() al puerto.");
        }

        if (listen(socketEscucha_, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(socketEscucha_);
            WSACleanup();
            throw std::runtime_error(
                "No se pudo poner el socket en modo escucha.");
        }
    }

    bool leerSolicitudHttp(SOCKET socketCliente, std::string& solicitud) {
        solicitud.clear();
        char buffer[4096];

        while (true) {
            int leidos = recv(socketCliente, buffer,
                              static_cast<int>(sizeof(buffer)), 0);
            if (leidos == 0) {
                // conexión cerrada por el cliente
                break;
            }
            if (leidos == SOCKET_ERROR) {
                std::cerr << "Error en recv(). Codigo: "
                          << WSAGetLastError() << "\n";
                return false;
            }

            solicitud.append(buffer, leidos);

            // Ver si ya tenemos las cabeceras completas
            std::size_t posFinalCabeceras = solicitud.find("\r\n\r\n");
            if (posFinalCabeceras != std::string::npos) {
                // Intentar obtener Content-Length
                std::size_t contenidoLongitud =
                    extraerContentLength(solicitud);
                if (contenidoLongitud == 0) {
                    // Sin cuerpo o no especificado: ya terminamos
                    return true;
                }

                std::size_t inicioCuerpo = posFinalCabeceras + 4;
                std::size_t bytesCuerpoActual =
                    solicitud.size() - inicioCuerpo;

                if (bytesCuerpoActual >= contenidoLongitud) {
                    return true; // ya tenemos todo el cuerpo
                }
            }
        }

        return !solicitud.empty();
    }

    std::size_t extraerContentLength(const std::string& solicitud) {
        std::size_t posCabecerasFin = solicitud.find("\r\n\r\n");
        if (posCabecerasFin == std::string::npos) return 0;

        std::string cabeceras = solicitud.substr(0, posCabecerasFin);
        std::istringstream streamCabeceras(cabeceras);
        std::string linea;

        while (std::getline(streamCabeceras, linea)) {
            // eliminar \r final si existe
            if (!linea.empty() && linea.back() == '\r') {
                linea.pop_back();
            }

            std::string prefijo = "Content-Length:";
            if (linea.size() >= prefijo.size() &&
                linea.substr(0, prefijo.size()) == prefijo) {
                std::string valor = linea.substr(prefijo.size());
                // quitar espacios
                std::size_t inicio =
                    valor.find_first_not_of(" \t");
                if (inicio != std::string::npos) {
                    valor = valor.substr(inicio);
                }
                try {
                    return static_cast<std::size_t>(
                        std::stoul(valor));
                } catch (...) {
                    return 0;
                }
            }
        }

        return 0;
    }

    bool extraerCuerpoJson(const std::string& solicitud,
                           std::string& cuerpoJson) {
        std::size_t posFinalCabeceras = solicitud.find("\r\n\r\n");
        if (posFinalCabeceras == std::string::npos) return false;

        std::size_t inicioCuerpo = posFinalCabeceras + 4;
        if (inicioCuerpo > solicitud.size()) return false;

        cuerpoJson = solicitud.substr(inicioCuerpo);
        return true;
    }

    void enviarRespuestaHttp(SOCKET socketCliente, int codigo,
                             const std::string& cuerpo,
                             const std::string& tipoContenido =
                                 "application/json") {
        std::ostringstream respuesta;

        std::string lineaEstado;
        switch (codigo) {
            case 200: lineaEstado = "200 OK"; break;
            case 400: lineaEstado = "400 Bad Request"; break;
            case 500: lineaEstado = "500 Internal Server Error"; break;
            default:  lineaEstado = "200 OK"; break;
        }

        respuesta << "HTTP/1.1 " << lineaEstado << "\r\n"
                  << "Content-Type: " << tipoContenido << "\r\n"
                  << "Content-Length: " << cuerpo.size() << "\r\n"
                  << "Connection: close\r\n"
                  << "\r\n"
                  << cuerpo;

        std::string textoRespuesta = respuesta.str();
        send(socketCliente, textoRespuesta.c_str(),
             static_cast<int>(textoRespuesta.size()), 0);
    }

    PoliticaComparacion convertirPolitica(const std::string& valor) {
        if (valor == "estricto") {
            return PoliticaComparacion::ESTRICTO;
        }
        return PoliticaComparacion::NORMALIZA_ESPACIOS;
    }

    void manejarCliente(SOCKET socketCliente) {
        ++contadorSolicitudes_;
        const auto id = contadorSolicitudes_;

        std::cout << "\n========== Solicitud #"
                  << id << " (socket=" << socketCliente
                  << ") ==========\n";

        std::string solicitud;
        if (!leerSolicitudHttp(socketCliente, solicitud)) {
            std::cout << "[Solicitud #" << id
                      << "] Error: solicitud HTTP invalida o incompleta.\n";
            enviarRespuestaHttp(socketCliente, 400,
                R"({"error":"Solicitud HTTP invalida o incompleta"})");
            return;
        }

        // Aceptamos solo POST /evaluate
        if (solicitud.find("POST /evaluate") == std::string::npos) {
            std::cout << "[Solicitud #" << id
                      << "] Error: metodo o ruta no soportados.\n";
            enviarRespuestaHttp(socketCliente, 400,
                R"({"error":"Solo se admite POST /evaluate"})");
            return;
        }

        std::string cuerpoJson;
        if (!extraerCuerpoJson(solicitud, cuerpoJson)) {
            std::cout << "[Solicitud #" << id
                      << "] Error: no se encontro cuerpo JSON.\n";
            enviarRespuestaHttp(socketCliente, 400,
                R"({"error":"No se encontro cuerpo JSON en la solicitud"})");
            return;
        }

        std::cout << "[Solicitud #" << id
                  << "] JSON recibido:\n"
                  << cuerpoJson << "\n";

        try {
            json cuerpo = json::parse(cuerpoJson);

            std::string codigoFuente =
                cuerpo.at("codigo").get<std::string>();

            LimitesEjecucion limites;
            if (cuerpo.contains("limites")) {
                auto jsonLimites = cuerpo["limites"];
                if (jsonLimites.contains("tiempo_segundos")) {
                    limites.tiempoMaximoSegundos =
                        jsonLimites["tiempo_segundos"];
                }
                if (jsonLimites.contains("memoria_mb")) {
                    limites.memoriaMaximaMB =
                        jsonLimites["memoria_mb"];
                }
            }

            std::vector<CasoPrueba> listaCasos;
            for (const auto& jsonCaso : cuerpo.at("casos")) {
                CasoPrueba caso;
                caso.entrada        =
                    jsonCaso.at("input").get<std::string>();
                caso.salidaEsperada =
                    jsonCaso.at("expected").get<std::string>();
                listaCasos.push_back(caso);
            }

            PoliticaComparacion politica =
                PoliticaComparacion::NORMALIZA_ESPACIOS;
            if (cuerpo.contains("politica_comparacion")) {
                politica = convertirPolitica(
                    cuerpo["politica_comparacion"]
                        .get<std::string>());
            }

            ResultadoGlobal resultado =
                motor_.evaluarCodigoFuente(
                    codigoFuente, listaCasos, limites, politica);

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
                    {"case",             rc.indiceCaso},
                    {"estado",           rc.estado},
                    {"tiempo_ms",        rc.tiempoMs},
                    {"memoria_mb",       rc.memoriaMB},
                    {"detalle_mismatch", rc.detalleDiferencia}
                });
            }
            jsonRespuesta["detallado"] = listaDetalle;

            std::string cuerpoRespuesta = jsonRespuesta.dump(2);

            std::cout << "[Solicitud #" << id
                      << "] JSON de respuesta:\n"
                      << cuerpoRespuesta << "\n";

            enviarRespuestaHttp(socketCliente, 200, cuerpoRespuesta);

        } catch (const std::exception& e) {
            json jsonError = {
                {"error", std::string("Error procesando la solicitud: ")
                          + e.what()}
            };

            std::cout << "[Solicitud #" << id
                      << "] Error en servidor: "
                      << e.what() << "\n";

            enviarRespuestaHttp(socketCliente, 400,
                                jsonError.dump(2));
        }
    }
};

int main() {
    try {
        const std::string directorioTrabajo = "./_work";
        MotorEvaluacion motor(directorioTrabajo);

        ServidorHttpBasico servidor(8090, motor);
        servidor.iniciar();

    } catch (const std::exception& e) {
        std::cerr << "Error fatal en el servidor: "
                  << e.what() << "\n";
        return 1;
    }

    return 0;
}
