#!/usr/bin/env python3
"""Comprobaciones funcionales temporales sin generar resultados del informe."""

import importlib.util
from contextlib import redirect_stdout
import fcntl
import io
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from unittest.mock import patch

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE / "plots"))
from progress_utils import Progress
SOURCE = r'''
#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>
#include "range_queries/sparse_table.hpp"
#include "range_queries/segment_tree.hpp"
#include "utils/rmq_inputs.hpp"
#include "utils/rmq_workload.hpp"
#include "utils/sha256.hpp"
#include "utils/progress.hpp"
int main() {
    using namespace bench;
    std::ostringstream log;
    auto *original_output = std::cout.rdbuf(log.rdbuf());
    {
        Progress progress(2, "Prueba");
        progress.advance("Primera tarea");
        progress.show("Primera tarea", true);
        progress.advance("Completo");
    }
    std::cout.rdbuf(original_output);
    assert(log.str().find("0% 0/2") != std::string::npos);
    assert(log.str().find("50% 1/2") != std::string::npos);
    assert(log.str().find("100% 2/2") != std::string::npos);
    assert(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    for (std::size_t n = 1; n <= 33; ++n) {
        auto values = generar_arreglo(n, 1729);
        assert(values == generar_arreglo(n, 1729));
        for (std::size_t i = 0; i < n; ++i)
            assert(values[i].first >= 0 && values[i].first <= max_value && values[i].second == i);
        if (n > 1) values[0].first = values[1].first = -1;
        sparse_table<Entry> sparse(values, [](Entry a, Entry b) { return std::min(a,b); });
        segment_tree<Entry> tree(values);
        for (std::size_t l = 0; l < n; ++l) for (std::size_t r = l+1; r <= n; ++r) {
            const auto expected = reference_rmq(values, {l,r});
            assert(sparse.query_idempotent(l,r) == expected);
            assert(sparse.query(l,r) == expected);
            assert(tree.query(l,r) == expected);
        }
        for (const auto &range : rangos(n)) {
            const auto queries = generar_consultas(n, range.length, 23, 31415);
            for (auto q : queries) assert(q.right - q.left == range.length && q.right <= n);
        }
        for (auto update : generar_actualizaciones(values, 20, 2718)) {
            assert(update.value != values[update.index].first);
            values[update.index] = {update.value, update.index};
            tree.update(update.index, values[update.index]);
            for (std::size_t l = 0; l < n; ++l) for (std::size_t r = l+1; r <= n; ++r)
                assert(tree.query(l,r) == reference_rmq(values, {l,r}));
        }
    }
    bool rejected = false;
    try { generar_consultas(3,4,1,1); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    for (const auto &raw : std::vector<std::vector<Value>>{{0}, {1,2,3}, {3,2,1}, {4,4,4},
             {std::numeric_limits<Value>::min(), 0, std::numeric_limits<Value>::max()}}) {
        Array values;
        for (std::size_t i = 0; i < raw.size(); ++i) values.emplace_back(raw[i], i);
        sparse_table<Entry> sparse(values, [](Entry a, Entry b) { return std::min(a,b); });
        segment_tree<Entry> tree(values);
        for (std::size_t l = 0; l < values.size(); ++l) for (std::size_t r = l+1; r <= values.size(); ++r) {
            assert(sparse.query_idempotent(l,r) == reference_rmq(values, {l,r}));
            assert(tree.query(l,r) == reference_rmq(values, {l,r}));
        }
    }
    rejected = false;
    try { generar_arreglo(0,1); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    Options options;
    options.profile = "smoke";
    options.static_queries = 16;
    options.cycles = 2;
    options.dynamic_queries = 1000;
    const auto work = prepare_workload(options, 1000, 1729);
    assert(work.batches.size() == 6);
    assert(work.hash == prepare_workload(options, 1000, 1729).hash);
    for (const auto &cycle : work.cycles) {
        std::size_t total = 0, smallest = 1000, largest = 0;
        for (const auto &batch : cycle.batches) {
            total += batch.queries.size();
            smallest = std::min(smallest, batch.queries.size());
            largest = std::max(largest, batch.queries.size());
        }
        assert(total == 1000 && largest - smallest <= 1);
    }
}
'''


def main():
    checks = Progress(7, "Comprobaciones")
    for terminal in [False, True]:
        class Output(io.StringIO):
            def isatty(self):
                return terminal
        log = Output()
        with redirect_stdout(log):
            bar = Progress(2, "Prueba")
            bar.advance("Primera tarea")
            bar.advance("Completo")
        text = log.getvalue()
        assert "0% 0/2" in text and "50% 1/2" in text and "100% 2/2" in text
        assert text.endswith("\n") and ("\r" in text) == terminal
    checks.advance("Barra Python en terminal y salida redirigida")
    root = "/tmp/opencode" if Path("/tmp/opencode").is_dir() else None
    with tempfile.TemporaryDirectory(prefix="boletin04-check-", dir=root) as directory:
        temp = Path(directory)
        source = temp / "check.cpp"
        source.write_text(SOURCE)
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        headers = sorted((BASE / "include").rglob("*.hpp"))
        header_progress = Progress(len(headers), "Encabezados C++")
        for header in headers:
            subprocess.run(compiler + ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-x", "c++", "-fsyntax-only", "-"],
                           input=f'#include "{header}"\n', text=True, check=True)
            header_progress.advance(header.name)
        checks.advance("Encabezados C++ independientes")
        subprocess.run(compiler + ["-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(BASE / "include"),
            str(source), "-o", str(temp / "check")], check=True)
        subprocess.run([str(temp / "check")], check=True)
        checks.advance("Generadores RMQ y barra C++ con sanitizadores")
        for arguments in [["--unknown", "1"], ["--profile", "invalid"], ["--cycles", "-1"]]:
            result = subprocess.run([str(BASE / "bin/bench_rmq"), *arguments], capture_output=True)
            assert result.returncode != 0
        cpu = min(os.sched_getaffinity(0))
        env = dict(os.environ, PYTHON=sys.executable, PYTHONDONTWRITEBYTECODE="1", MPLCONFIGDIR=str(temp / "mpl"))
        os.environ["MPLCONFIGDIR"] = str(temp / "mpl")
        denied = subprocess.run(["bash", str(BASE / "benchmarks/common.sh"), "official",
                                 "--cpu-core", str(cpu), "--data-root", str(temp / "denied")],
                                cwd=BASE, env=env, capture_output=True)
        assert denied.returncode != 0 and not (temp / "denied").exists()
        checks.advance("Parametros invalidos y rechazo sin piloto")
        with (BASE / "data/.benchmark.lock").open("a") as held:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            result = subprocess.run(["bash", str(BASE / "benchmarks/run_smoke.sh"),
                            "--cpu-core", str(cpu), "--data-root", str(temp / "data"),
                            "--session-path-file", str(temp / "session-path"), "--timeout-seconds", "120"],
                           cwd=BASE, env=dict(env, BENCHMARK_LOCK_FD=str(held.fileno())),
                            pass_fds=(held.fileno(),), capture_output=True, text=True)
            print(result.stdout, end="", flush=True)
            if result.returncode:
                print(result.stderr, file=sys.stderr)
            result.check_returncode()
        for label in ["Mediciones smoke", "Memoria smoke", "Validacion CSV", "Estadisticas y tablas",
                      "Graficos PNG y PDF", "smoke | Resultados y graficos completos"]:
            assert any("100%" in line and label in line for line in result.stdout.splitlines()), label
        assert "repeticion=1/4" in result.stdout and "semilla=1729" in result.stdout
        checks.advance("Pipeline pequeno completo con progreso")
        session = next((temp / "data").glob("smoke-*"))
        assert Path((temp / "session-path").read_text().strip()) == session
        assert (session / "COMPLETE").is_file()
        spec = importlib.util.spec_from_file_location("rmq_plots", BASE / "plots/plot_experiments.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _, samples, memory = module.load_session(session)
        assert len(samples) > 0 and len(memory) == 8
        tree_memory = memory[(memory.method == "segment_tree") & (memory.phase == "construction")]
        assert (tree_memory.index_bytes == tree_memory.input_bytes * 2).all()
        per_run, summary = module.summarize(samples)
        figure_count = (summary.groupby(["experiment", "range"]).ngroups
                        + summary[summary.experiment.isin(["static_queries", "dynamic_queries"])].groupby(["experiment", "n"]).ngroups + 3)
        assert len(list((session / "plots").glob("*.png"))) == figure_count
        assert len(list((session / "plots").glob("*.pdf"))) == figure_count
        assert any(f"100% {figure_count}/{figure_count}" in line and "Graficos PNG y PDF" in line
                   for line in result.stdout.splitlines())
        log = io.StringIO()
        with redirect_stdout(log), patch.object(module.plt.Figure, "savefig", side_effect=[None, OSError("Fallo PDF")]):
            try:
                module.render(summary, memory, temp)
            except OSError:
                pass
            else:
                raise AssertionError("Se ignoro un fallo al guardar PDF")
        assert "100%" not in log.getvalue() and not module.plt.get_fignums()
        assert (summary.runs == 4).all()
        assert module.quartiles([1,2,3,4]) == (1, 1.75, 2.5, 3.25, 4)
        path = session / "construction.csv"
        original = path.read_text()
        path.write_text(original + original.splitlines()[1] + "\n")
        try:
            module.load_session(session)
        except ValueError:
            pass
        else:
            raise AssertionError("Se aceptaron muestras duplicadas")
        path.write_text(original)
        session_id = json.loads((session / "session.json").read_text())["session_id"]
        path.write_text(original.replace(session_id, "wrong-session", 1))
        try:
            module.load_session(session)
        except ValueError:
            pass
        else:
            raise AssertionError("Se aceptaron sesiones mezcladas")
        path.write_text(original)
        marker = (session / "COMPLETE").read_text()
        (session / "COMPLETE").unlink()
        try:
            module.load_session(session)
        except OSError:
            pass
        else:
            raise AssertionError("Se acepto una sesion incompleta")
        (session / "COMPLETE").write_text(marker)
        metadata = json.loads((session / "session.json").read_text())
        assert metadata["profile"] == "smoke" and not metadata.get("budget_approved")
        assert set(metadata["runtime"]["packages"]) == {"numpy", "pandas", "matplotlib"}
        checks.advance("CSV memoria estadisticas cobertura y fallo al guardar graficos")
        failures = temp / "commands"
        failures.mkdir()
        fake_timeout = failures / "timeout"
        fake_timeout.write_text("#!/usr/bin/env bash\nexit 124\n")
        fake_timeout.chmod(0o755)
        result = subprocess.run(["bash", str(BASE / "benchmarks/run_smoke.sh"),
                                 "--cpu-core", str(cpu), "--data-root", str(temp / "failed")],
                                cwd=BASE, env=dict(env, PATH=str(failures) + os.pathsep + env["PATH"]),
                                capture_output=True, text=True)
        assert result.returncode != 0
        assert "smoke | Resultados y graficos completos" not in result.stdout
        assert "Interrumpido en mediciones" in result.stderr
        failed = next((temp / "failed").glob("smoke-*"))
        assert not (failed / "COMPLETE").exists()
        assert json.loads((failed / "session.json").read_text())["status"] == "incomplete"
        checks.advance("Timeout sin anunciar exito ni completar la sesion")
    print("OK generadores RMQ actualizaciones sanitizadores CSV y figuras")
    print("Pruebas y salidas pequenas eliminadas sin ejecutar la bateria oficial")


if __name__ == "__main__":
    main()
