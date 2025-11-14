#ifndef CODECOACH_MOTOR_EVALUACION_H
#define CODECOACH_MOTOR_EVALUACION_H

#include <string>
#include <vector>

// Representa un caso de prueba: datos de entrada y salida esperada.
struct CasoPrueba {
    std::string entrada;
    std::string salidaEsperada;
};

// Límites de ejecución que se aplican a cada caso.
struct LimitesEjecucion {
    int tiempoMaximoSegundos = 2;
    int memoriaMaximaMB      = 256;  // Solo se reporta, no se corta por memoria en esta versión.
};

// Cómo se comparan las salidas: exacta o normalizando espacios.
enum class PoliticaComparacion {
    ESTRICTO,
    NORMALIZA_ESPACIOS
};

// Resultado detallado por caso de prueba.
struct ResultadoCaso {
    int indiceCaso = 0;
    std::string estado;          // "OK", "INCORRECTO", "TL", "RE"
    double tiempoMs = 0.0;
    double memoriaMB = 0.0;
    std::string detalleDiferencia;
};

// Resumen global de la evaluación de todo el código.
struct ResultadoGlobal {
    bool compilacionExitosa = false;
    std::string erroresCompilacion;
    std::string estadoGlobal;     // "Correcto", "Incorrecto", "Error de ejecución", "Tiempo excedido", "Error de compilación"
    double tiempoPromedioMs = 0.0;
    double memoriaMaximaMB = 0.0;
    std::vector<ResultadoCaso> resultadosPorCaso;
};

// Clase principal que coordina la evaluación de una solución.
class MotorEvaluacion {
public:
    explicit MotorEvaluacion(std::string directorioTrabajo);

    ResultadoGlobal evaluarCodigoFuente(const std::string& codigoFuente,
                                        const std::vector<CasoPrueba>& listaCasos,
                                        const LimitesEjecucion& limites,
                                        PoliticaComparacion politica);

private:
    std::string directorioTrabajo_;

    std::string guardarCodigoFuenteTemporal(const std::string& codigoFuente) const;
    bool compilarCodigoUsuario(const std::string& rutaFuente,
                               std::string& rutaEjecutable,
                               std::string& erroresCompilacion) const;

    ResultadoCaso ejecutarYCompararCaso(const std::string& rutaEjecutable,
                                        const CasoPrueba& caso,
                                        const LimitesEjecucion& limites,
                                        PoliticaComparacion politica,
                                        int indiceCaso) const;

    static bool compararSalidaEstricta(const std::string& salidaObtenida,
                                       const std::string& salidaEsperada);

    static bool compararSalidaNormalizada(const std::string& salidaObtenida,
                                          const std::string& salidaEsperada,
                                          std::string& pistaDiferencia);

    static std::string leerArchivoCompleto(const std::string& rutaArchivo);
    static void escribirArchivoCompleto(const std::string& rutaArchivo,
                                        const std::string& contenido);
};

#endif
