#ifndef CODECOACH_EJECUTADOR_CODIGO_H
#define CODECOACH_EJECUTADOR_CODIGO_H

#include <string>

// Forward declaration para evitar ciclo de includes.
struct LimitesEjecucion;

// Resultado de ejecutar un programa para un caso de prueba concreto.
struct MedicionEjecucion {
    double tiempoMs = 0.0;
    double memoriaMB = 0.0;
    int codigoSalida = 0;
    bool seExcedioTiempo = false;
    bool huboErrorEjecucion = false;
    std::string salidaEstandar;
    std::string salidaError;
};

// Clase responsable de compilar y ejecutar el código del usuario.
class EjecutadorCodigo {
public:
    // Compila el archivo fuente con g++ y devuelve errores de compilación, si los hay.
    static bool compilarConGpp(const std::string& rutaFuente,
                               const std::string& rutaEjecutable,
                               std::string& erroresCompilacion);

    // Ejecuta el binario con límites y mide tiempo y memoria.
    static MedicionEjecucion ejecutarConLimites(const std::string& rutaEjecutable,
                                                const std::string& entrada,
                                                const LimitesEjecucion& limites);
};

#endif
