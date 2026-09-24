/**
 * bench_heaps.cpp: Medicion de rendimiento temporal de colas de prioridad (Binario y Binomial)
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
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>


#include "heaps/heaps.hpp"

// Funcion para validar y parsear argumentos de linea de comandos
inline void validate_input(int argc, char *argv[], std::string& heap_type,
    std::string& op_name, std::string& filename, std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, std::int64_t& step)
{
    if (argc != 8) {
        std::cerr << "Uso: " << argv[0] << " <HEAP_TYPE> <OP> <FILENAME> <RUNS> <LOWER> <UPPER> <STEP>\n"
                  << "  <HEAP_TYPE>: binary | binomial\n"
                  << "  <OP>:        build | insert | extract_min | top | meld\n"
                  << "  <FILENAME>:  Ruta del archivo CSV de salida\n"
                  << "  <RUNS>:      Repeticiones por cada tamano n (debe ser >= 4, recomendado >= 32)\n"
                  << "  <LOWER>:     Tamano inicial n (positivo)\n"
                  << "  <UPPER>:     Tamano maximo n (LOWER <= UPPER)\n"
                  << "  <STEP>:      Factor multiplicativo de escala (>= 2)\n";
        std::exit(EXIT_FAILURE);
    }

    heap_type = argv[1];
    op_name   = argv[2];
    filename  = argv[3];

    // Validacion de tipo de heap
    if (heap_type != "binary" && heap_type != "binomial") {
        std::cerr << "Error: Tipo de heap desconocido '" << heap_type << "'.\n"
                  << "Opciones validas: binary, binomial\n";
        std::exit(EXIT_FAILURE);
    }

    // Validacion de operacion
    if (op_name != "build" && op_name != "insert" &&
        op_name != "extract_min" && op_name != "top" && op_name != "meld") {
        std::cerr << "Error: Operacion desconocida '" << op_name << "'.\n"
                  << "Opciones validas: build, insert, extract_min, top, meld\n";
        std::exit(EXIT_FAILURE);
    }

    // Conversion segura de argumentos numericos
    try {
        runs  = std::stoll(argv[4]);
        lower = std::stoll(argv[5]);
        upper = std::stoll(argv[6]);
        step  = std::stoll(argv[7]);
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

// Funcion plantilla generica para ejecutar mediciones sobre cualquier Heap compatible
template <typename HeapType>
void run_heap_benchmark(const std::string& heap_label, const std::string& op,
                        const std::string& filename, std::int64_t runs,
                        std::int64_t lower, std::int64_t upper, std::int64_t step)
{
    // Calculo exacto de iteraciones totales
    std::int64_t count_sizes = 0;
    for (std::int64_t temp_n = lower; temp_n <= upper; temp_n *= step) {
        count_sizes++;
    }
    std::int64_t total_runs = runs * count_sizes;

    // Variables de cronometrado y estadisticas
    std::int64_t n, i, executed_runs = 0;
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
        std::exit(EXIT_FAILURE);
    }
    time_data << "n,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4\n";

    std::cout << "\033[0;36mEjecutando mediciones para [" << heap_label << " - " << op << "]...\033[0m\n\n";

    // Bucle multiplicativo sobre el tamano n
    for (n = lower; n <= upper; n *= step) {
        mean_time = 0.0;
        time_stdev = 0.0;

        // Generador pseudoaleatorio con semilla determinista ligada a n para reproducibilidad
        std::mt19937_64 rng_data(55443322ULL + n);
        std::uniform_int_distribution<int> dist_elem(1, 1000000000);

        // Vector base con n elementos aleatorios desordenados
        std::vector<int> vec(n);
        for (std::int64_t j = 0; j < n; j++) {
            vec[j] = dist_elem(rng_data);
        }

        // Vector secundario para la operacion meld
        std::vector<int> vec2;
        if (op == "meld") {
            vec2.resize(n);
            for (std::int64_t j = 0; j < n; j++) {
                vec2[j] = dist_elem(rng_data);
            }
        }

        // Elementos precalculados a insertar para la operacion insert
        std::vector<int> keys_to_insert;
        if (op == "insert") {
            keys_to_insert.resize(runs);
            for (std::int64_t j = 0; j < runs; j++) {
                keys_to_insert[j] = dist_elem(rng_data);
            }
        }

        // Para la operacion top, preconstruir el heap una sola vez fuera del bucle de repeticiones
        // ya que top() es una operacion de solo lectura y no muta la estructura
        std::unique_ptr<HeapType> h_top;
        if (op == "top") {
            h_top = std::make_unique<HeapType>(vec);
        }

        // Bucle de repeticiones estadisticas 
        for (i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            if (op == "build") {
                // Medicion pura de la construccion a partir del vector
                begin_time = std::chrono::high_resolution_clock::now();
                HeapType h(vec);
                end_time = std::chrono::high_resolution_clock::now();
                asm volatile("" : : "r,m"(h) : "memory");
                assert(!h.empty());

            } else if (op == "top") {
                begin_time = std::chrono::high_resolution_clock::now();
                int top_val = h_top->top();
                end_time = std::chrono::high_resolution_clock::now();
                asm volatile("" : : "r,m"(top_val) : "memory");
                assert(!h_top->empty());

            } else if (op == "insert") {
                // Setup: creacion del heap de tamano n fuera del cronometro
                HeapType h(vec);
                int key = keys_to_insert[i];
                begin_time = std::chrono::high_resolution_clock::now();
                h.push(key);
                end_time = std::chrono::high_resolution_clock::now();
                asm volatile("" : : "r,m"(h) : "memory");
                assert(!h.empty());

            } else if (op == "extract_min") {
                // Setup: creacion del heap de tamano n fuera del cronometro
                HeapType h(vec);
                begin_time = std::chrono::high_resolution_clock::now();
                h.pop();
                end_time = std::chrono::high_resolution_clock::now();
                asm volatile("" : : "r,m"(h) : "memory");

            } else if (op == "meld") {
                // Setup: creacion de dos heaps de tamano n fuera del cronometro
                HeapType h1(vec);
                HeapType h2(vec2);
                begin_time = std::chrono::high_resolution_clock::now();
                h1.meld(h2);
                end_time = std::chrono::high_resolution_clock::now();
                asm volatile("" : : "r,m"(h1) : "memory");
                assert(!h1.empty());
            }

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

        // Escritura en el CSV
        time_data << n << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << "\n";
    }

    std::cout << "\n\n\033[1;32mListo! Mediciones completadas con exito.\033[0m\n";
    time_data.close();
}

int main(int argc, char *argv[])
{
    std::string heap_type;
    std::string op_name;
    std::string filename;
    std::int64_t runs, lower, upper, step;
    validate_input(argc, argv, heap_type, op_name, filename, runs, lower, upper, step);

    if (heap_type == "binary") {
        run_heap_benchmark<binary_heap<int>>("binary", op_name, filename, runs, lower, upper, step);
    } else if (heap_type == "binomial") {
        run_heap_benchmark<binomial_heap>("binomial", op_name, filename, runs, lower, upper, step);
    } else {
        std::cerr << "Error interno: tipo de heap no soportado.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
