/**
 * bench_trees.cpp: Medicion de insercion, busqueda y eliminacion en arboles.
 * Basado en la plantilla uhr de LELE y los benchmarks del Boletin 1.
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../include/trees/avl_tree.h"
#include "../include/trees/red_black_tree.h"
#include "../include/trees/splay_tree.h"
#include "../include/utils/discrete_queries.h"
#include "../include/utils/tree_benchmark.h"
#include "../include/utils/tree_sequences.h"

// Lee los parametros que luego podra entregar el script de Bash.
void validate_input(int argc, char *argv[], std::string &tree_name,
                    std::string &op_name, std::string &distribution,
                    std::string &filename, std::int64_t &runs,
                    std::int64_t &lower, std::int64_t &upper,
                    std::int64_t &step) {
  if (argc != 9) {
    std::cerr
        << "Uso: " << argv[0]
        << " <TREE> <OP> <DISTRIBUTION> <FILENAME> <RUNS> <LOWER>"
           " <UPPER> <STEP>\n"
        << "  TREE:         avl | red_black | splay\n"
        << "  OP:           insert | search | erase | erase_after_search\n"
        << "  DISTRIBUTION: none | uniform | negative_binomial\n"
        << "  RUNS:         repeticiones por tamano, minimo 4\n"
        << "  STEP:         factor multiplicativo, minimo 2\n";
    std::exit(EXIT_FAILURE);
  }

  tree_name = argv[1];
  op_name = argv[2];
  distribution = argv[3];
  filename = argv[4];

  if (tree_name != "avl" && tree_name != "red_black" && tree_name != "splay") {
    std::cerr << "Arbol desconocido: " << tree_name << '\n';
    std::exit(EXIT_FAILURE);
  }
  if (op_name != "insert" && op_name != "search" && op_name != "erase" &&
      op_name != "erase_after_search") {
    std::cerr << "Operacion desconocida: " << op_name << '\n';
    std::exit(EXIT_FAILURE);
  }
  const bool usa_consultas =
      op_name == "search" || op_name == "erase_after_search";
  if (usa_consultas && distribution != "uniform" &&
      distribution != "negative_binomial") {
    std::cerr << "Esta operacion necesita una distribucion de consultas.\n";
    std::exit(EXIT_FAILURE);
  }
  if (!usa_consultas && distribution != "none") {
    std::cerr << "Use 'none' cuando no hay consultas.\n";
    std::exit(EXIT_FAILURE);
  }

  try {
    runs = std::stoll(argv[5]);
    lower = std::stoll(argv[6]);
    upper = std::stoll(argv[7]);
    step = std::stoll(argv[8]);
  } catch (const std::exception &) {
    std::cerr << "RUNS, LOWER, UPPER y STEP deben ser enteros.\n";
    std::exit(EXIT_FAILURE);
  }
  if (runs < 4 || lower <= 0 || lower > upper || step < 2) {
    std::cerr << "Parametros fuera del rango del experimento.\n";
    std::exit(EXIT_FAILURE);
  }
}

// Barra de progreso de la plantilla uhr.
void display_progress(std::int64_t actual, std::int64_t total) {
  const double progreso = actual / static_cast<double>(total);
  const std::int64_t ancho = 60;
  const std::int64_t lleno = static_cast<std::int64_t>(ancho * progreso);
  std::cout << '\r' << '[';
  for (std::int64_t i = 0; i < ancho; ++i) {
    std::cout << (i < lleno ? '=' : ' ');
  }
  std::cout << "] " << static_cast<int>(100 * progreso) << '%' << std::flush;
}

// Los cinco valores de uhr: minimo, Q1, mediana, Q3 y maximo.
void quartiles(std::vector<double> &data, std::vector<double> &q) {
  q.resize(5);
  std::sort(data.begin(), data.end());
  const std::size_t n = data.size();
  std::size_t p;

  q[0] = data.front();
  q[4] = data.back();
  if (n % 2 == 1) {
    q[2] = data[n / 2];
  } else {
    p = n / 2;
    q[2] = (data[p - 1] + data[p]) / 2.0;
  }
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

// Devuelve consultas preparadas para uno de los dos patrones de acceso.
std::vector<std::int64_t>
preparar_consultas(const std::vector<std::int64_t> &claves,
                   const std::vector<std::int64_t> &popularidad,
                   const std::string &distribution, std::size_t cantidad,
                   std::uint64_t seed) {
  if (distribution == "uniform") {
    return generar_consultas_uniformes(claves, cantidad, seed);
  }
  return generar_consultas_binomiales_negativas(popularidad, cantidad, seed);
}

// Repite una fase para un tipo de arbol. La generacion queda fuera del reloj.
template <class Arbol>
void run_tree_benchmark(const std::string &tree_name,
                        const std::string &op_name,
                        const std::string &distribution,
                        const std::string &filename, std::int64_t runs,
                        std::int64_t lower, std::int64_t upper,
                        std::int64_t step) {
  std::int64_t count_sizes = 0;
  for (std::int64_t n = lower; n <= upper;) {
    ++count_sizes;
    if (n > upper / step) {
      break;
    }
    n *= step;
  }
  const std::int64_t total_runs = runs * count_sizes;
  std::int64_t executed_runs = 0;
  std::vector<double> times(static_cast<std::size_t>(runs));
  std::vector<double> q;

  std::ofstream time_data(filename);
  if (!time_data) {
    std::cerr << "No se pudo abrir el archivo: " << filename << '\n';
    std::exit(EXIT_FAILURE);
  }
  time_data
      << "n,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4,arbol,fase,distribucion\n";

  std::string fase;
  if (op_name == "insert") {
    fase = "insercion";
  } else if (op_name == "search") {
    fase = "busqueda";
  } else if (op_name == "erase") {
    fase = "eliminacion_d0_total";
  } else {
    fase = "eliminacion_d1_total";
  }
  const std::string distribucion_csv = distribution == "uniform"  ? "uniforme"
                                       : distribution == "negative_binomial"
                                           ? "binomial_negativa"
                                           : "sin_especificar";

  std::cout << "Midiendo " << tree_name << " / " << op_name << " / "
            << distribution << '\n';
  for (std::int64_t n = lower; n <= upper;) {
    double mean_time = 0.0;
    double time_stdev = 0.0;

    for (std::int64_t i = 0; i < runs; ++i) {
      display_progress(++executed_runs, total_runs);

      // La misma semilla para los tres arboles produce las mismas secuencias.
      const std::uint64_t seed = 123456789ULL +
                                 static_cast<std::uint64_t>(n) * 1000003ULL +
                                 static_cast<std::uint64_t>(i) * 17ULL;
      auto claves = generar_claves<std::int64_t>(static_cast<std::size_t>(n));
      barajar_claves(claves, seed);

      Arbol arbol;
      std::vector<std::int64_t> consultas;
      std::vector<std::int64_t> eliminacion;
      std::size_t operaciones = static_cast<std::size_t>(n);

      if (op_name != "insert") {
        const std::size_t insertadas = poblar_arbol(arbol, claves);
        if (insertadas != claves.size()) {
          std::cerr << "Fallo al poblar el arbol.\n";
          std::exit(EXIT_FAILURE);
        }
      }

      if (op_name == "search" || op_name == "erase_after_search") {
        std::vector<std::int64_t> popularidad;
        if (distribution == "negative_binomial") {
          popularidad = claves;
          barajar_claves(popularidad, seed + 1);
        }
        const auto preparacion =
            preparar_consultas(claves, popularidad, distribution,
                               static_cast<std::size_t>(5 * n), seed + 2);
        const std::size_t aciertos_preparacion =
            ejecutar_consultas(arbol, preparacion);
        if (aciertos_preparacion != preparacion.size()) {
          std::cerr << "Fallo en las consultas de preparacion.\n";
          std::exit(EXIT_FAILURE);
        }
        consultas =
            preparar_consultas(claves, popularidad, distribution,
                               static_cast<std::size_t>(10 * n), seed + 3);
        if (op_name == "search") {
          operaciones = consultas.size();
        } else {
          const std::size_t aciertos_previos =
              ejecutar_consultas(arbol, consultas);
          if (aciertos_previos != consultas.size()) {
            std::cerr << "Fallo en las consultas previas a eliminar.\n";
            std::exit(EXIT_FAILURE);
          }
        }
      }

      if (op_name == "erase" || op_name == "erase_after_search") {
        eliminacion = generar_orden_eliminacion(claves, seed + 4);
      }

      std::size_t exitos = 0;
      const auto begin_time = std::chrono::steady_clock::now();
      if (op_name == "insert") {
        exitos = poblar_arbol(arbol, claves);
      } else if (op_name == "search") {
        exitos = ejecutar_consultas(arbol, consultas);
      } else {
        exitos = eliminar_claves(arbol, eliminacion);
      }
      const auto end_time = std::chrono::steady_clock::now();

      // Consumimos el resultado y validamos fuera de la ventana medida.
      asm volatile("" : : "g"(exitos) : "memory");
      if (exitos != operaciones ||
          (op_name != "search" &&
           arbol.size() != (op_name == "insert" ? claves.size() : 0))) {
        std::cerr << "La operacion medida produjo un resultado incorrecto.\n";
        std::exit(EXIT_FAILURE);
      }

      const std::chrono::duration<double, std::nano> elapsed_time =
          end_time - begin_time;
      times[static_cast<std::size_t>(i)] =
          elapsed_time.count() / static_cast<double>(operaciones);
      mean_time += times[static_cast<std::size_t>(i)];
    }

    mean_time /= static_cast<double>(runs);
    for (double tiempo : times) {
      const double dev = tiempo - mean_time;
      time_stdev += dev * dev;
    }
    time_stdev = std::sqrt(time_stdev / static_cast<double>(runs - 1));
    quartiles(times, q);

    time_data << n << ',' << mean_time << ',' << time_stdev;
    for (double valor : q) {
      time_data << ',' << valor;
    }
    time_data << ',' << tree_name << ',' << fase << ',' << distribucion_csv
              << '\n';

    if (n > upper / step) {
      break;
    }
    n *= step;
  }
  std::cout << "\nListo: " << filename << '\n';
}

int main(int argc, char *argv[]) {
  std::string tree_name, op_name, distribution, filename;
  std::int64_t runs, lower, upper, step;
  validate_input(argc, argv, tree_name, op_name, distribution, filename, runs,
                 lower, upper, step);

  if (tree_name == "avl") {
    run_tree_benchmark<AVLTree<std::int64_t>>(
        tree_name, op_name, distribution, filename, runs, lower, upper, step);
  } else if (tree_name == "red_black") {
    run_tree_benchmark<RedBlackTree<std::int64_t>>(
        tree_name, op_name, distribution, filename, runs, lower, upper, step);
  } else {
    run_tree_benchmark<SplayTree<std::int64_t>>(
        tree_name, op_name, distribution, filename, runs, lower, upper, step);
  }
  return EXIT_SUCCESS;
}
