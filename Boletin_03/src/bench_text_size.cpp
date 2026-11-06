#include "utils/string_benchmark.hpp"

int main(int argc, char **argv) {
    // Ejecutamos el experimento que cambia el largo del texto
    return bench::run_experiment(argc, argv, "text_size");
}
