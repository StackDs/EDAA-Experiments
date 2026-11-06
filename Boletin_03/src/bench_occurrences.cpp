#include "utils/string_benchmark.hpp"

int main(int argc, char **argv) {
    // Ejecutamos el experimento de frecuencias y los casos extremos
    return bench::run_experiment(argc, argv, "occurrences");
}
