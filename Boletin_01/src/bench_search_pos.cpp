/**
 * bench_search_pos.cpp: Medicion del efecto de la posicion del objetivo (k)
 * en algoritmos de busqueda con largo fijo (n)
 * Basado en la plantilla uhr de LELE 
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/generator.hpp"
#include "search/search.hpp"

// Funcion para validar y parsear argumentos de linea de comandos
inline void validate_input(int argc, char *argv[], std::string& algo_name,
    std::string& filename, std::int64_t& runs, std::int64_t& n_size,
    std::int64_t& lower_k, std::int64_t& upper_k, std::int64_t& step_k)
{
    if (argc != 8) {
        std::cerr << "Uso: " << argv[0] << " <ALGO> <FILENAME> <RUNS> <N_SIZE> <LOWER_K> <UPPER_K> <STEP_K>\n"
                  << "  <ALGO>:     seq | seq_stl | bin | bin_stl | gal | gal_stl\n"
                  << "  <FILENAME>: Ruta del archivo CSV de salida\n"
                  << "  <RUNS>:     Repeticiones por cada posicion k (debe ser >= 4, recomendado >= 32)\n"
                  << "  <N_SIZE>:   Tamano fijo del vector de datos (positivo)\n"
                  << "  <LOWER_K>:  Indice inicial de busqueda (0 <= LOWER_K < N_SIZE)\n"
                  << "  <UPPER_K>:  Indice final de busqueda (LOWER_K <= UPPER_K < N_SIZE)\n"
                  << "  <STEP_K>:   Paso aditivo de incremento en k (>= 1)\n";
        std::exit(EXIT_FAILURE);
    }

    algo_name = argv[1];
    filename = argv[2];

    // Validacion de algoritmo soportado
    if (algo_name != "seq" && algo_name != "seq_stl" &&
        algo_name != "bin" && algo_name != "bin_stl" &&
        algo_name != "gal" && algo_name != "gal_stl") {
        std::cerr << "Error: Algoritmo desconocido '" << algo_name << "'.\n"
                  << "Opciones validas: seq, seq_stl, bin, bin_stl, gal, gal_stl\n";
        std::exit(EXIT_FAILURE);
    }

    // Conversion segura de argumentos numericos
    try {
        runs    = std::stoll(argv[3]);
        n_size  = std::stoll(argv[4]);
        lower_k = std::stoll(argv[5]);
        upper_k = std::stoll(argv[6]);
        step_k  = std::stoll(argv[7]);
    } catch (const std::invalid_argument& ex) {
        std::cerr << "Error numerico: argumento invalido (" << ex.what() << ")\n";
        std::exit(EXIT_FAILURE);
    } catch (const std::out_of_range& ex) {
        std::cerr << "Error numerico: valor fuera de rango (" << ex.what() << ")\n";
        std::exit(EXIT_FAILURE);
    }

    // Validaciones de dominio experimental
    if (runs < 4) {
        std::cerr << "Error: <RUNS> debe ser al menos 4 (para cuartiles).\n";
        std::exit(EXIT_FAILURE);
    }
    if (n_size <= 0) {
        std::cerr << "Error: <N_SIZE> debe ser estrictamente positivo.\n";
        std::exit(EXIT_FAILURE);
    }
    if (lower_k < 0 || lower_k >= n_size) {
        std::cerr << "Error: <LOWER_K> debe satisfacer 0 <= LOWER_K < N_SIZE.\n";
        std::exit(EXIT_FAILURE);
    }
    if (upper_k < lower_k || upper_k >= n_size) {
        std::cerr << "Error: <UPPER_K> debe satisfacer LOWER_K <= UPPER_K < N_SIZE.\n";
        std::exit(EXIT_FAILURE);
    }
    if (step_k < 1) {
        std::cerr << "Error: En paso aditivo, <STEP_K> debe ser al menos 1.\n";
        std::exit(EXIT_FAILURE);
    }
}

// Dibuja una barra de progreso dinamica en la terminal
inline void display_progress(std::int64_t u, std::int64_t v)
{
    const double progress = u / double(v);
    const std::int64_t width = 70;
    const std::int64_t p = width * progress;
    std::int64_t i;

    std::cout << "\033[1m[";
    for (i = 0; i < width; i++) {
        if (i < p)
            std::cout << "=";
        else if (i == p)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << std::int64_t(progress * 100.0) << "%\r\033[0m";
    std::cout.flush();
}

// Calcula los 5 numeros de Tukey
inline void quartiles(std::vector<double>& data, std::vector<double>& q)
{
    q.resize(5);
    std::size_t n = data.size();
    std::size_t p;

    std::sort(data.begin(), data.end());

    if (n < 4) {
        std::cerr << "quartiles needs at least 4 data points." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Minimo y Maximo
    q[0] = data.front();
    q[4] = data.back();

    // Mediana
    if (n % 2 == 1) {
        q[2] = data[n / 2];
    } else {
        p = n / 2;
        q[2] = (data[p - 1] + data[p]) / 2.0;
    }

    // Cuartiles inferior y superior
    if (n % 4 >= 2) {
        q[1] = data[n / 4];
        q[3] = data[(3 * n) / 4];
    } else {
        p = n / 4;
        q[1] = 0.25 * data[p - 1] + 0.75 * data[p];
        p = (3 * n) / 4;
        q[3] = 0.75 * data[p - 1] + 0.25 * data[p];
    }
}

int main(int argc, char *argv[])
{
    // Validacion y parseo de argumentos
    std::string algo_name;
    std::string filename;
    std::int64_t runs, n_size, lower_k, upper_k, step_k;
    validate_input(argc, argv, algo_name, filename, runs, n_size, lower_k, upper_k, step_k);

    // Seleccion del algoritmo mediante puntero a funcion
    std::int64_t (*search_fn)(const std::vector<std::int64_t>&, const std::int64_t&) = nullptr;

    if (algo_name == "seq") {
        search_fn = &sequential_search_custom<std::int64_t>;
    } else if (algo_name == "seq_stl") {
        search_fn = &sequential_search_STL<std::int64_t>;
    } else if (algo_name == "bin") {
        search_fn = &binary_search_custom<std::int64_t>;
    } else if (algo_name == "bin_stl") {
        search_fn = &binary_search_STL<std::int64_t>;
    } else if (algo_name == "gal") {
        search_fn = &galloping_search_custom<std::int64_t>;
    } else if (algo_name == "gal_stl") {
        search_fn = &galloping_search_STL<std::int64_t>;
    } else {
        std::cerr << "Error interno: algoritmo no mapeado correctamente.\n";
        return EXIT_FAILURE;
    }

    // Calculo de corridas totales para la barra de progreso (paso aditivo)
    std::int64_t total_runs = runs * (((upper_k - lower_k) / step_k) + 1);

    // Variables de medicion y cronometrado
    std::int64_t k, i, executed_runs = 0;
    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;
    auto begin_time = std::chrono::high_resolution_clock::now();
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed_time = end_time - begin_time;

    // Apertura y cabecera del archivo CSV
    std::ofstream time_data(filename);
    if (!time_data.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de salida: " << filename << "\n";
        return EXIT_FAILURE;
    }
    time_data << "k,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";

    // Generacion unica del arreglo ordenado de datos de tamano n_size
    // Se realiza una sola vez antes de variar k para no incurrir en overhead
    std::cout << "\033[0;33mGenerando vector ordenado unico de tamano n = " << n_size << "...\033[0m\n";
    std::vector<std::int64_t> data = generar_vector<std::int64_t>(n_size, 123456789ULL);

    // Inicio de mediciones
    std::cout << "\033[0;36mEjecutando mediciones de posicion para [" << algo_name << "]...\033[0m\n\n";

    // Bucle aditivo sobre la posicion k
    for (k = lower_k; k <= upper_k; k += step_k) {
        mean_time = 0.0;
        time_stdev = 0.0;

        // Elemento objetivo ubicado contractualmente en la posicion k
        std::int64_t target_val = data[k];

        // Bucle de repeticiones estadisticas para la posicion k
        for (i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            begin_time = std::chrono::high_resolution_clock::now();

            std::int64_t res = search_fn(data, target_val);

            end_time = std::chrono::high_resolution_clock::now();

            // Barrera para evitar que GCC con -O3 elimine la llamada a search_fn
            asm volatile("" : : "r,m"(res) : "memory");

            // Verificacion contractual de correccion: debe hallar exactamente la posicion k
            assert(res == k);

            elapsed_time = end_time - begin_time;
            times[i] = elapsed_time.count();

            mean_time += times[i];
        }

        // Calculo de la media muestral
        mean_time /= runs;

        // Calculo de la desviacion estandar muestral insesgada (N - 1)
        for (i = 0; i < runs; i++) {
            dev = times[i] - mean_time;
            time_stdev += dev * dev;
        }
        time_stdev /= (runs - 1);
        time_stdev = std::sqrt(time_stdev);

        // Calculo de los 5 numeros de Tukey
        quartiles(times, q);

        // Escritura de la fila en el CSV
        time_data << k << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << "\n";
    }

    // Mensaje de finalizacion y cierre
    std::cout << "\n\n\033[1;32mListo! Mediciones completadas con exito.\033[0m\n";
    time_data.close();

    return EXIT_SUCCESS;
}
