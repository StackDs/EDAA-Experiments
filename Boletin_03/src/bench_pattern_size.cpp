#include "utils/string_benchmark.hpp"

int main(int argc, char **argv) {
    // Ejecutamos el experimento que cambia el largo del patron
    return bench::run_experiment(argc, argv, "pattern_size");
}
