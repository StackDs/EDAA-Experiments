/**
 * bench_search_size : Medicion del efecto del largo del vector (n)
 * en algoritmos de busqueda (Secuencial, Binaria y Galopante)
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



// Funcion para validar y parsear argumentos de linea de comandos, agregamos soporte para seleccionar el algoritmo a probar
inline void validate_input(int argc, char *argv[], std::string& algo_name,
    std::string& filename, std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, std::int64_t& step)
{
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] << " <ALGO> <FILENAME> <RUNS> <LOWER> <UPPER> <STEP>\n"
                  << "  <ALGO>:     seq | seq_stl | bin | bin_stl | gal | gal_stl\n"
                  << "  <FILENAME>: Ruta del archivo CSV de salida\n"
                  << "  <RUNS>:     Repeticiones por cada tamano (debe ser >= 4, recomendado >= 32)\n"
                  << "  <LOWER>:    Tamano inicial minimo (positivo)\n"
                  << "  <UPPER>:    Tamano maximo (LOWER <= UPPER)\n"
                  << "  <STEP>:     Factor multiplicativo de escala (debe ser >= 2)\n";
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
        runs  = std::stoll(argv[3]);
        lower = std::stoll(argv[4]);
        upper = std::stoll(argv[5]);
        step  = std::stoll(argv[6]);
    } catch (const std::invalid_argument& ex) {
        std::cerr << "Error numerico: argumento invalido (" << ex.what() << ")\n";
        std::exit(EXIT_FAILURE);
    } catch (const std::out_of_range& ex) {
        std::cerr << "Error numerico: valor fuera de rango (" << ex.what() << ")\n";
        std::exit(EXIT_FAILURE);
    }

    // Validacion de restricciones del experimento
    if (runs < 4) {
        std::cerr << "Error: <RUNS> debe ser al menos 4 (para cuartiles).\n";
        std::exit(EXIT_FAILURE);
    }
    if (lower <= 0 || upper <= 0) {
        std::cerr << "Error: <LOWER> y <UPPER> deben ser estrictamente positivos.\n";
        std::exit(EXIT_FAILURE);
    }
    if (lower > upper) {
        std::cerr << "Error: <LOWER> no puede ser mayor que <UPPER>.\n";
        std::exit(EXIT_FAILURE);
    }
    if (step < 2) {
        std::cerr << "Error: En paso multiplicativo, <STEP> debe ser al menos 2.\n";
        std::exit(EXIT_FAILURE);
    }
}

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

    // Get min and max
    q[0] = data.front();
    q[4] = data.back();

    // Find median
    if (n % 2 == 1) {
        q[2] = data[n / 2];
    } else {
        p = n / 2;
        q[2] = (data[p - 1] + data[p]) / 2.0;
    }

    // Find lower and upper quartiles
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
    std::int64_t runs, lower, upper, step;
    validate_input(argc, argv, algo_name, filename, runs, lower, upper, step);

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

    // Calculo exacto de repeticiones totales para la barra de progreso
    std::int64_t count_sizes = 0;
    for (std::int64_t temp_n = lower; temp_n <= upper; temp_n *= step) {
        count_sizes++;
    }
    std::int64_t total_runs = runs * count_sizes;

    // Variables de cronometrado 
    std::int64_t n, i, executed_runs = 0;
    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;
    auto begin_time = std::chrono::high_resolution_clock::now();
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed_time = end_time - begin_time;

    // Generador de numeros aleatorios para seleccionar indices a consultar
    // Usamos semilla fija para garantizar reproducibilidad entre algoritmos
    std::mt19937_64 rng(42ULL);

    // Apertura y cabecera del archivo CSV
    std::ofstream time_data(filename);
    if (!time_data.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de salida: " << filename << "\n";
        return EXIT_FAILURE;
    }
    time_data << "n,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";

        // Inicio de la ejecucion de pruebas
    std::cout << "\033[0;36mEjecutando mediciones para [" << algo_name << "]...\033[0m\n\n";
    executed_runs = 0;

    // Bucle multiplicativo para el Experimento 1
    for (n = lower; n <= upper; n *= step) {
        mean_time = 0.0;
        time_stdev = 0.0;

        // Generamos el vector mediante la semilla estatica
        std::vector<std::int64_t> data = generar_vector<std::int64_t>(n, 123456789ULL);

        // Distribucion uniforme para seleccionar elementos validos en [0, n - 1]
        std::uniform_int_distribution<std::int64_t> dist_idx(0, n - 1);

        // Bucle de repeticiones estadisticas (runs >= 32)
        for (i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            // Seleccion del objetivo a buscar antes del cronometro
            std::int64_t target_idx = dist_idx(rng);
            std::int64_t target_val = data[target_idx];

            // Medicion de tiempo de alta resolucion
            begin_time = std::chrono::high_resolution_clock::now();
            
            std::int64_t res = search_fn(data, target_val);

            end_time = std::chrono::high_resolution_clock::now();

            // Barrera para evitar que GCC con -O3 elimine la llamada a search_fn
            asm volatile("" : : "r,m"(res) : "memory");

            // Verificacion contractual de correccion
            assert(res >= 0 && data[res] == target_val);

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

        // Calculo de los 5 numeros de Tukey (min, Q1, mediana, Q3, max)
        quartiles(times, q);

        // Escritura de la fila de datos en el CSV
        time_data << n << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << "\n";
    }

    // Mensaje de termino y cierre de archivo
    std::cout << "\n\n\033[1;32mListo! Mediciones completadas con exito.\033[0m\n";
    time_data.close();

    return EXIT_SUCCESS;
}
