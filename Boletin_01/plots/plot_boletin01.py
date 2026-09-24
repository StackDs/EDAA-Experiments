#!/usr/bin/env python3
"""
plot_boletin01.py: Generador modular de graficos cientificos para el Boletin 01 (EDAA).
Organizado estrictamente en:
- Experimento 1: Variacion por tamano de la secuencia (n)
- Experimento 2: Variacion por posicion del objetivo (k)
- Experimento 3: Colas de prioridad (Heaps)

Estructura de comparaciones en busqueda:
1. Cada algoritmo de busqueda contra su equivalente en la STL (1 a 1):
   - Secuencial: Custom vs std::find
   - Binaria: Custom vs std::lower_bound
   - Galopante: Custom vs Variante STL
2. Contraste entre las implementaciones Custom (Algoritmos Propios):
   - Panoramica global: seq vs bin vs gal
   - Detalle sublineal: bin vs gal
"""

import os
import sys
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Configuracion global de estilo para graficos de publicacion cientifica
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 11,
    'axes.labelsize': 12,
    'axes.titlesize': 13,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 10,
    'figure.titlesize': 14,
    'axes.grid': True,
    'grid.alpha': 0.35,
    'grid.linestyle': '--',
    'savefig.dpi': 300,
    'savefig.bbox': 'tight'
})

# Configuracion para comparativas 1 a 1 (Custom vs STL)
PAIRWISE_CONFIG = {
    'seq': {
        'title': 'Busqueda Secuencial: Implementacion Propia vs std::find',
        'unit_size': 'Tiempo promedio [$\\mu$s]',
        'unit_pos': 'Tiempo promedio [ms]',
        'scale_size': 1e-3,
        'scale_pos': 1e-6,
        'custom_label': 'Secuencial Custom (bucle directo)',
        'custom_color': '#1f77b4',
        'custom_marker': 'o',
        'stl_label': 'Secuencial STL (std::find)',
        'stl_color': '#ff7f0e',
        'stl_marker': 's'
    },
    'bin': {
        'title': 'Busqueda Binaria: Implementacion Propia vs std::lower_bound',
        'unit_size': 'Tiempo promedio [ns]',
        'unit_pos': 'Tiempo promedio [ns]',
        'scale_size': 1.0,
        'scale_pos': 1.0,
        'custom_label': 'Binaria Custom (bifurcacion 3 ramas)',
        'custom_color': '#2ca02c',
        'custom_marker': '^',
        'stl_label': 'Binaria STL (std::lower_bound, 1 rama)',
        'stl_color': '#9467bd',
        'stl_marker': 'v'
    },
    'gal': {
        'title': 'Busqueda Galopante: Implementacion Propia vs Variante STL',
        'unit_size': 'Tiempo promedio [ns]',
        'unit_pos': 'Tiempo promedio [ns]',
        'scale_size': 1.0,
        'scale_pos': 1.0,
        'custom_label': 'Galopante Custom (biseccion manual)',
        'custom_color': '#d62728',
        'custom_marker': 'D',
        'stl_label': 'Galopante STL (std::lower_bound acotado)',
        'stl_color': '#8c564b',
        'stl_marker': 'p'
    }
}

# Estilos para comparativas entre algoritmos Custom (Propios)
FAMILY_CUSTOM_STYLES = {
    'seq': {'color': '#1f77b4', 'marker': 'o', 'linestyle': '-',  'label': 'Secuencial Custom (O(n))'},
    'bin': {'color': '#2ca02c', 'marker': '^', 'linestyle': '-',  'label': 'Binaria Custom (O(log n))'},
    'gal': {'color': '#d62728', 'marker': 'D', 'linestyle': '-',  'label': 'Galopante Custom (O(log k))'}
}

# Estilos para colas de prioridad (heaps)
HEAP_STYLES = {
    'binary':   {'color': '#1f77b4', 'marker': 'o', 'linestyle': '-',  'label': 'Binary Heap (STL Vector)'},
    'binomial': {'color': '#d62728', 'marker': 's', 'linestyle': '--', 'label': 'Binomial Heap (Bosque Nodos)'}
}

SAVE_PDF = False


def load_csv(filepath):
    """Carga un archivo CSV y valida su existencia y contenido."""
    if not os.path.exists(filepath):
        return None
    try:
        df = pd.read_csv(filepath)
        if df.empty:
            return None
        return df
    except Exception as ex:
        print(f"Error al leer {filepath}: {ex}", file=sys.stderr)
        return None


def get_clamped_yerr(t_mean, t_stdev, scale=1.0, is_log_y=False):
    """
    Calcula barras de error fisicamente validas.
    En escala lineal evita valores negativos (t >= 0).
    En escala logaritmica acota el limite inferior para que sea estrictamente positivo.
    """
    mean_s = np.array(t_mean, dtype=float) * scale
    std_s = np.array(t_stdev, dtype=float) * scale
    if is_log_y:
        lower = np.minimum(mean_s * 0.75, std_s)
    else:
        lower = np.minimum(mean_s, std_s)
    return [lower, std_s]


def save_figure(fig, target_dirs, filename_base):
    """Guarda la figura en las carpetas de destino en formato PNG (300 DPI) y opcionalmente PDF."""
    for d in target_dirs:
        os.makedirs(d, exist_ok=True)
        png_path = os.path.join(d, f"{filename_base}.png")
        fig.savefig(png_path)
        if SAVE_PDF:
            pdf_path = os.path.join(d, f"{filename_base}.pdf")
            fig.savefig(pdf_path)

    fmt = "(.png/.pdf)" if SAVE_PDF else "(.png)"
    print(f"  -> Generado: {filename_base} {fmt}")
    plt.close(fig)



def plot_experimento_1_size(data_dir, output_dir):
 
    print("\n==================================================================")
    print("[Experimento 1] Variacion por Tamano de la Secuencia (n)")
    print("==================================================================")

    size_dir = os.path.join(data_dir, 'Search_Size') if os.path.exists(os.path.join(data_dir, 'Search_Size')) else os.path.join(data_dir, 'search_size')
    exp1_dir = os.path.join(output_dir, 'Experimento_1_Size')
    target_dirs = [exp1_dir]

    dfs = {}
    for key in ['seq', 'seq_stl', 'bin', 'bin_stl', 'gal', 'gal_stl']:
        path = os.path.join(size_dir, f"{key}.csv")
        df = load_csv(path)
        if df is not None:
            dfs[key] = df
        else:
            print(f"  [Aviso] No se encontro {path}")

    if not dfs:
        print("  [Salto] No hay datos disponibles para Experimento 1.")
        return

    # --- 1. Comparaciones 1 a 1 (Custom vs STL) ---
    pairs = [('seq', 'seq_stl'), ('bin', 'bin_stl'), ('gal', 'gal_stl')]
    for c_key, s_key in pairs:
        if c_key not in dfs or s_key not in dfs:
            continue

        cfg = PAIRWISE_CONFIG[c_key]
        df_c = dfs[c_key]
        df_s = dfs[s_key]
        scale = cfg['scale_size']
        is_log_y = (c_key == 'seq')

        fig, ax = plt.subplots(figsize=(8, 5.2))
        yerr_c = get_clamped_yerr(df_c['t_mean'], df_c['t_stdev'], scale, is_log_y)
        yerr_s = get_clamped_yerr(df_s['t_mean'], df_s['t_stdev'], scale, is_log_y)

        # Custom
        ax.errorbar(df_c['n'], df_c['t_mean'] * scale, yerr=yerr_c,
                    label=cfg['custom_label'], color=cfg['custom_color'],
                    marker=cfg['custom_marker'], linestyle='-',
                    capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
        ax.fill_between(df_c['n'],
                        np.maximum(0, (df_c['t_mean'] - df_c['t_stdev']) * scale),
                        (df_c['t_mean'] + df_c['t_stdev']) * scale,
                        color=cfg['custom_color'], alpha=0.12)

        # STL
        ax.errorbar(df_s['n'], df_s['t_mean'] * scale, yerr=yerr_s,
                    label=cfg['stl_label'], color=cfg['stl_color'],
                    marker=cfg['stl_marker'], linestyle='--',
                    capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
        ax.fill_between(df_s['n'],
                        np.maximum(0, (df_s['t_mean'] - df_s['t_stdev']) * scale),
                        (df_s['t_mean'] + df_s['t_stdev']) * scale,
                        color=cfg['stl_color'], alpha=0.12)

        ax.set_xscale('log')
        if is_log_y:
            ax.set_yscale('log')
        else:
            ax.set_ylim(bottom=0)

        ax.set_xlabel('Tamano de la secuencia ($n$)')
        ax.set_ylabel(cfg['unit_size'])
        ax.set_title(f"Exp 1: {cfg['title']}")
        ax.legend(loc='best', framealpha=0.9)
        save_figure(fig, target_dirs, f"fig_size_{c_key}_vs_stl")

    # --- 2. Contraste entre Algoritmos Custom (Propios) ---
    avail_custom = [k for k in ['seq', 'bin', 'gal'] if k in dfs]
    if avail_custom:
        # A. Panoramica Global Custom (Log-Log)
        fig, ax = plt.subplots(figsize=(8, 5.2))
        for k in avail_custom:
            df = dfs[k]
            st = FAMILY_CUSTOM_STYLES[k]
            yerr = get_clamped_yerr(df['t_mean'], df['t_stdev'], 1.0, is_log_y=True)
            ax.errorbar(df['n'], df['t_mean'], yerr=yerr,
                        label=st['label'], color=st['color'],
                        marker=st['marker'], linestyle=st['linestyle'],
                        capsize=3, alpha=0.9, linewidth=1.5, markersize=5)

        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.set_xlabel('Tamano de la secuencia ($n$)')
        ax.set_ylabel('Tiempo promedio de ejecucion [ns] (Escala Log)')
        ax.set_title('Exp 1: Contraste entre Algoritmos Propios (Custom) vs Tamano')
        ax.legend(loc='upper left', framealpha=0.9)
        save_figure(fig, target_dirs, "fig_size_custom_all")

        # B. Detalle Sublineal Custom: Binaria vs Galopante (lineal en ns)
        if 'bin' in dfs and 'gal' in dfs:
            fig, ax = plt.subplots(figsize=(8, 5.2))
            for k in ['bin', 'gal']:
                df = dfs[k]
                st = FAMILY_CUSTOM_STYLES[k]
                yerr = get_clamped_yerr(df['t_mean'], df['t_stdev'], 1.0, is_log_y=False)
                ax.errorbar(df['n'], df['t_mean'], yerr=yerr,
                            label=st['label'], color=st['color'],
                            marker=st['marker'], linestyle=st['linestyle'],
                            capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
                ax.fill_between(df['n'],
                                np.maximum(0, df['t_mean'] - df['t_stdev']),
                                df['t_mean'] + df['t_stdev'],
                                color=st['color'], alpha=0.12)

            ax.set_xscale('log')
            ax.set_ylim(bottom=0)
            ax.set_xlabel('Tamano de la secuencia ($n$)')
            ax.set_ylabel('Tiempo promedio de ejecucion [ns]')
            ax.set_title('Exp 1: Contraste Custom Sublineal (Binaria vs Galopante)')
            ax.legend(loc='best', framealpha=0.9)
            save_figure(fig, target_dirs, "fig_size_custom_bin_vs_gal")



def plot_experimento_2_pos(data_dir, output_dir):
    print("\n==================================================================")
    print("[Experimento 2] Variacion por Posicion del Objetivo (k)")
    print("==================================================================")

    pos_dir = os.path.join(data_dir, 'Search_Pos') if os.path.exists(os.path.join(data_dir, 'Search_Pos')) else os.path.join(data_dir, 'search_pos')
    exp2_dir = os.path.join(output_dir, 'Experimento_2_Posicion')
    target_dirs = [exp2_dir]

    dfs = {}
    for key in ['seq', 'seq_stl', 'bin', 'bin_stl', 'gal', 'gal_stl']:
        path = os.path.join(pos_dir, f"{key}.csv")
        df = load_csv(path)
        if df is not None:
            dfs[key] = df
        else:
            print(f"  [Aviso] No se encontro {path}")

    if not dfs:
        print("  [Salto] No hay datos disponibles para Experimento 2.")
        return

    # --- 1. Comparaciones 1 a 1 (Custom vs STL) ---
    pairs = [('seq', 'seq_stl'), ('bin', 'bin_stl'), ('gal', 'gal_stl')]
    for c_key, s_key in pairs:
        if c_key not in dfs or s_key not in dfs:
            continue

        cfg = PAIRWISE_CONFIG[c_key]
        df_c = dfs[c_key]
        df_s = dfs[s_key]
        scale = cfg['scale_pos']
        unit_label = cfg['unit_pos']

        k_max = max(df_c['k'].max(), df_s['k'].max())
        pct_c = (df_c['k'] / k_max) * 100.0
        pct_s = (df_s['k'] / k_max) * 100.0

        fig, ax = plt.subplots(figsize=(8, 5.2))
        yerr_c = get_clamped_yerr(df_c['t_mean'], df_c['t_stdev'], scale, is_log_y=False)
        yerr_s = get_clamped_yerr(df_s['t_mean'], df_s['t_stdev'], scale, is_log_y=False)

        # Custom
        ax.errorbar(pct_c, df_c['t_mean'] * scale, yerr=yerr_c,
                    label=cfg['custom_label'], color=cfg['custom_color'],
                    marker=cfg['custom_marker'], linestyle='-',
                    capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
        ax.fill_between(pct_c,
                        np.maximum(0, (df_c['t_mean'] - df_c['t_stdev']) * scale),
                        (df_c['t_mean'] + df_c['t_stdev']) * scale,
                        color=cfg['custom_color'], alpha=0.12)

        # STL
        ax.errorbar(pct_s, df_s['t_mean'] * scale, yerr=yerr_s,
                    label=cfg['stl_label'], color=cfg['stl_color'],
                    marker=cfg['stl_marker'], linestyle='--',
                    capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
        ax.fill_between(pct_s,
                        np.maximum(0, (df_s['t_mean'] - df_s['t_stdev']) * scale),
                        (df_s['t_mean'] + df_s['t_stdev']) * scale,
                        color=cfg['stl_color'], alpha=0.12)

        ax.set_ylim(bottom=0)
        ax.set_xlabel('Posicion relativa del objetivo ($k / n$) [%]')
        ax.set_ylabel(unit_label)
        ax.set_title(f"Exp 2: {cfg['title']} ($N = 10^7$)")
        ax.legend(loc='best', framealpha=0.9)
        save_figure(fig, target_dirs, f"fig_pos_{c_key}_vs_stl")

    # --- 2. Contraste entre Algoritmos Custom (Propios) ---
    avail_custom = [k for k in ['seq', 'bin', 'gal'] if k in dfs]
    if avail_custom:
        # A. Panoramica Global Custom vs k (Log-Y)
        fig, ax = plt.subplots(figsize=(8, 5.2))
        for k in avail_custom:
            df = dfs[k]
            st = FAMILY_CUSTOM_STYLES[k]
            pct = (df['k'] / df['k'].max()) * 100.0
            yerr = get_clamped_yerr(df['t_mean'], df['t_stdev'], 1.0, is_log_y=True)
            ax.errorbar(pct, df['t_mean'], yerr=yerr,
                        label=st['label'], color=st['color'],
                        marker=st['marker'], linestyle=st['linestyle'],
                        capsize=3, alpha=0.9, linewidth=1.5, markersize=4)

        ax.set_yscale('log')
        ax.set_xlabel('Posicion relativa del objetivo ($k / n$) [%]')
        ax.set_ylabel('Tiempo promedio [ns] (Escala Log)')
        ax.set_title('Exp 2: Contraste entre Algoritmos Propios vs Posicion ($N = 10^7$)')
        ax.legend(loc='center right', framealpha=0.9)
        save_figure(fig, target_dirs, "fig_pos_custom_all")

        # B. Detalle Sublineal Custom: Binaria vs Galopante (lineal en ns)
        if 'bin' in dfs and 'gal' in dfs:
            fig, ax = plt.subplots(figsize=(8, 5.2))
            for k in ['bin', 'gal']:
                df = dfs[k]
                st = FAMILY_CUSTOM_STYLES[k]
                pct = (df['k'] / df['k'].max()) * 100.0
                yerr = get_clamped_yerr(df['t_mean'], df['t_stdev'], 1.0, is_log_y=False)
                ax.errorbar(pct, df['t_mean'], yerr=yerr,
                            label=st['label'], color=st['color'],
                            marker=st['marker'], linestyle=st['linestyle'],
                            capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
                ax.fill_between(pct,
                                np.maximum(0, df['t_mean'] - df['t_stdev']),
                                df['t_mean'] + df['t_stdev'],
                                color=st['color'], alpha=0.12)

            ax.set_ylim(bottom=0)
            ax.set_xlabel('Posicion relativa del objetivo ($k / n$) [%]')
            ax.set_ylabel('Tiempo promedio de ejecucion [ns]')
            ax.set_title('Exp 2: Contraste Custom Sublineal (Binaria vs Galopante)')
            ax.legend(loc='best', framealpha=0.9)
            save_figure(fig, target_dirs, "fig_pos_custom_bin_vs_gal")




def plot_experimento_3_heaps(data_dir, output_dir):
    """
    Experimento 3: Evalua las operaciones sobre Colas de Prioridad.
    Contrasta Binary Heap (STL) vs Binomial Heap (Nodos) en 5 operaciones.
    """
    print("\n==================================================================")
    print("[Experimento 3] Colas de Prioridad (Heaps)")
    print("==================================================================")

    heaps_dir = os.path.join(data_dir, 'Heaps') if os.path.exists(os.path.join(data_dir, 'Heaps')) else os.path.join(data_dir, 'heaps')
    exp3_dir = os.path.join(output_dir, 'Experimento_3_Heaps')
    target_dirs = [exp3_dir]

    operations = [
        ('build',       'Operacion build_heap (Construccion desde Vector)', True,  True),
        ('top',         'Operacion top (Consulta de Elemento Prioritario)', True,  False),
        ('insert',      'Operacion insert (Insercion de Clave / Push)',    True,  False),
        ('extract_min', 'Operacion extract_min (Extraccion de Raiz / Pop)',True,  False),
        ('meld',        'Operacion meld (Fusion o Union de dos Heaps)',    True,  True)
    ]

    for op_name, title, log_x, log_y in operations:
        bin_path = os.path.join(heaps_dir, f"binary_{op_name}.csv")
        bino_path = os.path.join(heaps_dir, f"binomial_{op_name}.csv")

        df_bin = load_csv(bin_path)
        df_bino = load_csv(bino_path)

        if df_bin is None and df_bino is None:
            print(f"  [Salto] No hay datos para operacion heap: {op_name}")
            continue

        fig, ax = plt.subplots(figsize=(8, 5.2))

        if df_bin is not None:
            style = HEAP_STYLES['binary']
            yerr_bin = get_clamped_yerr(df_bin['t_mean'], df_bin['t_stdev'], 1.0, is_log_y=log_y)
            ax.errorbar(df_bin['n'], df_bin['t_mean'], yerr=yerr_bin,
                        label=style['label'], color=style['color'],
                        marker=style['marker'], linestyle=style['linestyle'],
                        capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
            ax.fill_between(df_bin['n'],
                            np.maximum(0, df_bin['t_mean'] - df_bin['t_stdev']),
                            df_bin['t_mean'] + df_bin['t_stdev'],
                            color=style['color'], alpha=0.12)

        if df_bino is not None:
            style = HEAP_STYLES['binomial']
            yerr_bino = get_clamped_yerr(df_bino['t_mean'], df_bino['t_stdev'], 1.0, is_log_y=log_y)
            ax.errorbar(df_bino['n'], df_bino['t_mean'], yerr=yerr_bino,
                        label=style['label'], color=style['color'],
                        marker=style['marker'], linestyle=style['linestyle'],
                        capsize=3, alpha=0.9, linewidth=1.6, markersize=5)
            ax.fill_between(df_bino['n'],
                            np.maximum(0, df_bino['t_mean'] - df_bino['t_stdev']),
                            df_bino['t_mean'] + df_bino['t_stdev'],
                            color=style['color'], alpha=0.12)

        if log_x:
            ax.set_xscale('log')
        if log_y:
            ax.set_yscale('log')
        else:
            ax.set_ylim(bottom=0)

        ax.set_xlabel('Tamano del heap ($n$)')
        ax.set_ylabel('Tiempo promedio de ejecucion [ns]')
        ax.set_title(f'Exp 3: Heaps - {title}')
        ax.legend(loc='best', framealpha=0.9)
        save_figure(fig, target_dirs, f"fig_heaps_{op_name}")


def main():
    parser = argparse.ArgumentParser(description="Generador de figuras para Boletin 01 (EDAA).")
    parser.add_argument('--data-dir', default=None, help='Ruta a la carpeta data/')
    parser.add_argument('--output-dir', default=None, help='Ruta para guardar las figuras')
    parser.add_argument('--all', action='store_true', help='Graficar todos los experimentos (por defecto)')
    parser.add_argument('--exp1', action='store_true', help='Graficar solo Experimento 1 (Tamano n)')
    parser.add_argument('--exp2', action='store_true', help='Graficar solo Experimento 2 (Posicion k)')
    parser.add_argument('--exp3', action='store_true', help='Graficar solo Experimento 3 (Heaps)')
    parser.add_argument('--search', action='store_true', help='Graficar solo experimentos de busqueda (Exp 1 y 2)')
    parser.add_argument('--heaps', action='store_true', help='Graficar solo heaps (Exp 3)')
    parser.add_argument('--pdf', action='store_true', help='Exportar tambien en PDF (por defecto solo PNG)')
    args = parser.parse_args()

    global SAVE_PDF
    if args.pdf:
        SAVE_PDF = True

    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    data_dir = args.data_dir if args.data_dir else os.path.join(base_dir, 'data')
    output_dir = args.output_dir if args.output_dir else os.path.join(base_dir, 'plots')

    os.makedirs(output_dir, exist_ok=True)

    print(f"Directorio de datos: {data_dir}")
    print(f"Directorio base de graficos: {output_dir}")

    # Determinar que experimentos ejecutar
    run_all = args.all or (not args.exp1 and not args.exp2 and not args.exp3 and not args.search and not args.heaps)
    run_exp1 = run_all or args.exp1 or args.search
    run_exp2 = run_all or args.exp2 or args.search
    run_exp3 = run_all or args.exp3 or args.heaps

    if run_exp1:
        plot_experimento_1_size(data_dir, output_dir)

    if run_exp2:
        plot_experimento_2_pos(data_dir, output_dir)

    if run_exp3:
        plot_experimento_3_heaps(data_dir, output_dir)

    print("\n[Listo] Proceso de generacion de graficos finalizado exitosamente.")


if __name__ == '__main__':
    main()
