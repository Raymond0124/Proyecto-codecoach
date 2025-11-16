#include "../include/EjecutadorCodigo.h"
#include "../include/MotorEvaluacion.h"   // ← AGREGA ESTA LÍNEA
// Ya NO usamos SandboxEjecucion ni wait.h aquí.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <chrono>

// Windows API para crear procesos y medir memoria.
#include <windows.h>
#include <psapi.h>

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

    // Leer errores de compilación (si los hay).
    {
        std::ifstream archivoErrores(rutaErrores, std::ios::binary);
        std::ostringstream acumuladorErrores;
        acumuladorErrores << archivoErrores.rdbuf();
        erroresCompilacion = acumuladorErrores.str();
    }

    return resultado == 0;
}

MedicionEjecucion EjecutadorCodigo::ejecutarConLimites(const std::string& rutaEjecutable,
                                                       const std::string& entrada,
                                                       const LimitesEjecucion& limites) {
    MedicionEjecucion medicion;

    fs::path directorioTrabajo = fs::path(rutaEjecutable).parent_path();
    fs::create_directories(directorioTrabajo);

    std::string rutaEntrada = (directorioTrabajo / "entrada.txt").string();
    std::string rutaSalida  = (directorioTrabajo / "salida.txt").string();
    std::string rutaError   = (directorioTrabajo / "error.txt").string();

    // 1. Guardar entrada en archivo.
    {
        std::ofstream archivoEntrada(rutaEntrada, std::ios::binary);
        archivoEntrada << entrada;
    }

    // 2. Construir comando que cmd.exe va a ejecutar:
    //    programa < entrada.txt > salida.txt 2> error.txt
    std::ostringstream comandoUsuario;
    comandoUsuario << "\"" << rutaEjecutable << "\""
                   << " < \"" << rutaEntrada << "\""
                   << " > \"" << rutaSalida << "\""
                   << " 2> \"" << rutaError << "\"";

    std::string lineaComando = "cmd.exe /C " + comandoUsuario.str();

    // Necesitamos un buffer modificable para CreateProcessA.
    std::string lineaComandoCopiable = lineaComando;
    LPSTR cmdLine = lineaComandoCopiable.data();

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // No mostrar ventana de consola.

    // 3. Crear proceso hijo en otro proceso (sandbox mínimo).
    BOOL ok = CreateProcessA(
        nullptr,               // Aplicación (usamos cmd.exe en cmdLine)
        cmdLine,               // Línea de comando
        nullptr, nullptr,      // Seguridad (por defecto)
        FALSE,                 // Heredar handles
        CREATE_NO_WINDOW,      // Sin ventana
        nullptr,               // Heredar entorno
        directorioTrabajo.string().c_str(), // Directorio de trabajo
        &si,
        &pi
    );

    if (!ok) {
        medicion.huboErrorEjecucion = true;
        medicion.codigoSalida = -1;
        medicion.salidaError = "No se pudo crear el proceso del usuario.";
        return medicion;
    }

    DWORD timeoutMs = (limites.tiempoMaximoSegundos > 0)
                        ? static_cast<DWORD>(limites.tiempoMaximoSegundos * 1000)
                        : INFINITE;

    auto inicio = std::chrono::steady_clock::now();
    DWORD waitRes = WaitForSingleObject(pi.hProcess, timeoutMs);
    auto fin = std::chrono::steady_clock::now();

    medicion.tiempoMs =
        std::chrono::duration<double, std::milli>(fin - inicio).count();

    if (waitRes == WAIT_TIMEOUT) {
        // Se excedió el tiempo: matamos el proceso.
        TerminateProcess(pi.hProcess, 1);
        medicion.seExcedioTiempo = true;
        medicion.codigoSalida = 1;
    } else if (waitRes == WAIT_FAILED) {
        medicion.huboErrorEjecucion = true;
        medicion.codigoSalida = -1;
        medicion.salidaError = "Error al esperar el proceso del usuario.";
    } else {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            medicion.codigoSalida = static_cast<int>(exitCode);
        } else {
            medicion.codigoSalida = -1;
            medicion.huboErrorEjecucion = true;
        }
    }

    // 4. Medir memoria máxima usada (aproximada).
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
        medicion.memoriaMB =
            static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    } else {
        medicion.memoriaMB = 0.0;
    }

    // Cerrar handles del proceso hijo.
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // 5. Marcar error de ejecución si no hubo timeout pero el código de salida fue distinto de 0.
    if (!medicion.seExcedioTiempo && medicion.codigoSalida != 0) {
        medicion.huboErrorEjecucion = true;
    }

    // 6. Leer stdout.
    {
        std::ifstream archivoSalida(rutaSalida, std::ios::binary);
        std::ostringstream acumuladorSalida;
        acumuladorSalida << archivoSalida.rdbuf();
        medicion.salidaEstandar = acumuladorSalida.str();
    }

    // 7. Leer stderr.
    {
        std::ifstream archivoError(rutaError, std::ios::binary);
        std::ostringstream acumuladorError;
        acumuladorError << archivoError.rdbuf();
        medicion.salidaError = acumuladorError.str();
    }

    return medicion;
}
