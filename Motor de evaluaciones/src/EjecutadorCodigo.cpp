#include "../include/EjecutadorCodigo.h"
#include "../include/MotorEvaluacion.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <iostream>

// Windows API
#include <windows.h>
#include <psapi.h>

namespace fs = std::filesystem;

bool EjecutadorCodigo::compilarConGpp(const std::string& rutaFuente,
                                      const std::string& rutaEjecutable,
                                      std::string& erroresCompilacion) {
    fs::path exePath = fs::absolute(rutaEjecutable);
    fs::path directorioSalida = exePath.parent_path();
    fs::create_directories(directorioSalida);

    std::string rutaErrores = (directorioSalida / "compilacion_errores.txt").string();

    std::ostringstream comandoCompilacion;
    comandoCompilacion << "g++ -std=c++17 -O2 "
                       << "\"" << fs::absolute(rutaFuente).string() << "\""
                       << " -o \"" << exePath.string() << "\""
                       << " 2> \"" << rutaErrores << "\"";

    std::cout << "[Compilador] Ejecutando: " << comandoCompilacion.str() << "\n";

    int resultado = std::system(comandoCompilacion.str().c_str());

    {
        std::ifstream archivoErrores(rutaErrores, std::ios::binary);
        std::ostringstream acumuladorErrores;
        acumuladorErrores << archivoErrores.rdbuf();
        erroresCompilacion = acumuladorErrores.str();
    }

    std::cout << "[Compilador] Result = " << resultado << "\n";
    return resultado == 0;
}

MedicionEjecucion EjecutadorCodigo::ejecutarConLimites(const std::string& rutaEjecutable,
                                                       const std::string& entrada,
                                                       const LimitesEjecucion& limites) {
    MedicionEjecucion medicion;

    // Rutas absolutas para evitar problemas de directorios.
    fs::path exePathAbs        = fs::absolute(rutaEjecutable);
    fs::path directorioTrabajo = exePathAbs.parent_path();
    fs::create_directories(directorioTrabajo);

    fs::path rutaEntradaPath = directorioTrabajo / "entrada.txt";
    fs::path rutaSalidaPath  = directorioTrabajo / "salida.txt";
    fs::path rutaErrorPath   = directorioTrabajo / "error.txt";

    std::string rutaEntrada = rutaEntradaPath.string();
    std::string rutaSalida  = rutaSalidaPath.string();
    std::string rutaError   = rutaErrorPath.string();

    std::cout << "[EjecutadorCodigo] Ejecutable = " << exePathAbs.string() << "\n";
    std::cout << "[EjecutadorCodigo] Existe ejecutable? "
              << (fs::exists(exePathAbs) ? "SI" : "NO") << "\n";

    // 1. Guardar la entrada en archivo.
    {
        std::ofstream archivoEntrada(rutaEntrada, std::ios::binary);
        archivoEntrada << entrada;
    }

    // 2. Crear handles heredables para stdin, stdout y stderr.
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE hEntrada = CreateFileA(
        rutaEntrada.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        &sa,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    HANDLE hSalida = CreateFileA(
        rutaSalida.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    HANDLE hError = CreateFileA(
        rutaError.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hEntrada == INVALID_HANDLE_VALUE ||
        hSalida  == INVALID_HANDLE_VALUE ||
        hError   == INVALID_HANDLE_VALUE) {

        std::cout << "[EjecutadorCodigo] ERROR: no se pudieron abrir archivos de redireccion.\n";
        medicion.huboErrorEjecucion = true;
        medicion.codigoSalida = -1;
        medicion.salidaError = "No se pudieron abrir archivos de entrada/salida/error.";
        if (hEntrada != INVALID_HANDLE_VALUE) CloseHandle(hEntrada);
        if (hSalida  != INVALID_HANDLE_VALUE) CloseHandle(hSalida);
        if (hError   != INVALID_HANDLE_VALUE) CloseHandle(hError);
        return medicion;
    }

    // 3. Configurar STARTUPINFO con los std handles redirigidos.
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdInput  = hEntrada;
    si.hStdOutput = hSalida;
    si.hStdError  = hError;

    // 4. Crear el proceso directamente sobre el .exe (sin cmd.exe).
    std::string exePathStr = exePathAbs.string();
    LPSTR cmdLine = exePathStr.data(); // CreateProcessA puede modificar este buffer.

    std::cout << "[EjecutadorCodigo] Lanzando proceso directamente: " << exePathStr << "\n";

    BOOL ok = CreateProcessA(
        exePathAbs.string().c_str(),  // lpApplicationName
        nullptr,                      // lpCommandLine (sin argumentos)
        nullptr,                      // lpProcessAttributes
        nullptr,                      // lpThreadAttributes
        TRUE,                         // bInheritHandles -> importante para los std handles
        CREATE_NO_WINDOW,             // dwCreationFlags
        nullptr,                      // lpEnvironment
        directorioTrabajo.string().c_str(), // lpCurrentDirectory
        &si,
        &pi
    );

    // Ya no necesitamos los handles en este proceso; el hijo los heredó.
    CloseHandle(hEntrada);
    CloseHandle(hSalida);
    CloseHandle(hError);

    if (!ok) {
        DWORD err = GetLastError();
        std::cout << "[EjecutadorCodigo] ERROR: CreateProcessA fallo. Codigo Win32 = " << err << "\n";

        medicion.huboErrorEjecucion = true;
        medicion.codigoSalida = -1;
        std::ostringstream msg;
        msg << "No se pudo crear el proceso de usuario. Error Win32 = " << err;
        medicion.salidaError = msg.str();

        return medicion;
    }

    // 5. Esperar con timeout y medir tiempo.
    DWORD timeoutMs = (limites.tiempoMaximoSegundos > 0)
                      ? static_cast<DWORD>(limites.tiempoMaximoSegundos * 1000)
                      : INFINITE;

    auto inicio = std::chrono::steady_clock::now();
    DWORD waitRes = WaitForSingleObject(pi.hProcess, timeoutMs);
    auto fin = std::chrono::steady_clock::now();

    medicion.tiempoMs =
        std::chrono::duration<double, std::milli>(fin - inicio).count();

    if (waitRes == WAIT_TIMEOUT) {
        std::cout << "[EjecutadorCodigo] Tiempo excedido; terminando proceso.\n";
        TerminateProcess(pi.hProcess, 1);
        medicion.seExcedioTiempo = true;
        medicion.codigoSalida = 1;
    } else if (waitRes == WAIT_FAILED) {
        std::cout << "[EjecutadorCodigo] ERROR: WaitForSingleObject fallo.\n";
        medicion.huboErrorEjecucion = true;
        medicion.codigoSalida = -1;
        medicion.salidaError = "WaitForSingleObject fallo.";
    } else {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            medicion.codigoSalida = static_cast<int>(exitCode);
        } else {
            medicion.codigoSalida = -1;
            medicion.huboErrorEjecucion = true;
        }
        std::cout << "[EjecutadorCodigo] Exit code proceso usuario = "
                  << medicion.codigoSalida << "\n";
    }

    // 6. Medir memoria.
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
        medicion.memoriaMB =
            static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    } else {
        medicion.memoriaMB = 0.0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // 7. Leer stdout y stderr desde los archivos.
    {
        std::ifstream archivoSalida(rutaSalida, std::ios::binary);
        std::ostringstream acum;
        acum << archivoSalida.rdbuf();
        medicion.salidaEstandar = acum.str();
    }
    {
        std::ifstream archivoError(rutaError, std::ios::binary);
        std::ostringstream acum;
        acum << archivoError.rdbuf();
        medicion.salidaError = acum.str();
    }

    // 8. Si no hubo timeout y el exit code es distinto de 0, marcamos error.
    if (!medicion.seExcedioTiempo && medicion.codigoSalida != 0) {
        medicion.huboErrorEjecucion = true;
    }

    return medicion;
}
