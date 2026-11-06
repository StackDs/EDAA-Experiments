## Alcance y requisitos

Fuente del encargo: [[Boletin4]]. Este documento define el trabajo pendiente; no contiene resultados experimentales.

| Requisito | Trabajo previsto | Evidencia de cumplimiento |
|---|---|---|
| Ejercicio 1 | Implementar Segment Tree y Sparse Table | Código, explicación y pruebas |
| Ejercicio 2: construcción | Medir ambas estructuras para distintos n | Tablas y gráficos |
| Ejercicio 2: consultas | Rangos aleatorios de distintas longitudes | Tablas y gráficos por n y longitud |
| Ejercicio 2: espacio | Analizar asignaciones de los constructores | Fórmulas y bytes reservados |
| Ejercicio 3 | Comparar ventajas y desventajas | Discusión estática/dinámica y costos |
| Ejercicio 4 | Generalizar operaciones y propiedades algebraicas | Tabla y argumentos sobre garantías |
| Entrega | PDF de 5–15 páginas, portada con nombre y matrícula, fuentes comprimidas y referencias | Lista final de verificación |

El enunciado dice «antes del viernes 6 de noviembre»; por el período 2026-2 se interpreta como el 6 de noviembre de 2026. Confirmar la hora en CANVAS y no asumir que se permite entregar durante todo ese viernes. La extracción del nombre de archivo está deteriorada: verificar los separadores exactos de apellido, nombre y sufijo 04.

## Contrato común

- Entrada: arreglo no vacío de enteros; consultas inclusivas [L,R], con índices desde 0 y 0 ≤ L ≤ R < n.
- Salida RMQ: **índice del mínimo**, no solo su valor.
- Empates: devolver el menor índice. Comparar pares (valor, índice), con prioridad lexicográfica.
- Rechazar rangos inválidos explícitamente; no medir validación de entradas distintas para cada estructura.
- Segment Tree: actualización puntual update(i,valor). Es coherente con la variante dinámica descrita; medir actualizaciones es complementario, no una exigencia explícita de la batería del ejercicio 2.
- Sparse Table: estructura estática; no ofrecer una actualización con garantías que la implementación no pueda cumplir.

## Diseño de estructuras

### Segment Tree

Usar inicialmente un árbol iterativo con capacidad B, la menor potencia de dos mayor o igual a n. Reservar 2B posiciones y almacenar índices; el arreglo de valores lo posee la estructura. El combinador accede al arreglo y elige el índice con menor valor, desempatando por índice. Representar posiciones de relleno con un índice inválido que nunca se desreferencie.

- Construir hojas y combinar de abajo hacia arriba: O(n).
- Consulta: combinar segmentos disjuntos respetando su orden; O(log n).
- Actualización puntual: modificar el valor y recalcular ancestros; O(log n).
- Memoria: n valores y 2B índices, más metadatos; O(n).

Para generalizar a operaciones no conmutativas, usar acumuladores izquierdo y derecho y combinarlos en el orden original. No generalizar directamente una consulta que acumule nodos en cualquier orden.

### Sparse Table

Cada celda st[j][i] guarda el índice del mínimo de A[i..i+2^j−1]. Construir desde intervalos unitarios y combinar mitades. Para consultar, elegir j=floor(log2(R−L+1)) y combinar los bloques que empiezan en L y R−2^j+1.

- Construcción: O(n log n).
- Consulta RMQ: O(1), porque repetir elementos no altera el mínimo.
- Memoria: O(n log n).
- Usar logaritmos enteros precalculados o cálculo entero; no logaritmos flotantes.

Elegir almacenamiento rectangular contiguo para facilitar inspección y medición. Si K=floor(log2 n)+1, reservar nK índices; documentar que algunas celdas no se usan. Una variante compacta reserva sumatorio de (n−2^j+1), para j=0,…,K−1; no mezclar ambas fórmulas al reportar.

## Organización prevista

```text
Boletin_04/
├── Makefile
├── README.md
├── include/range_queries/
│   ├── segment_tree.hpp
│   ├── sparse_table.hpp
│   └── argmin.hpp
├── include/utils/range_generator.hpp
├── src/bench_ranges.cpp
├── tests/test_ranges.cpp
├── benchmarks/run_all.sh
├── data/{build,query,memory}/
├── plots/plot_boletin04.py
└── report/Boletin04.tex
```

Revisar la infraestructura de los boletines anteriores antes de reutilizarla. Mantener compilación y ejecución independientes desde esta carpeta.

## Fases y criterios de aceptación

### 1. Implementación y validación

- [ ] Crear un oráculo lineal que devuelva el índice mínimo con la misma regla de empate.
- [ ] Probar n=1, tamaños no potencia de dos, valores iguales, arreglos crecientes/decrecientes y mínimos en los extremos.
- [ ] Verificar todos los intervalos en arreglos pequeños y consultas aleatorias en arreglos mayores.
- [ ] Comparar ambas estructuras con el oráculo sobre el mismo arreglo.
- [ ] Aplicar actualizaciones aleatorias al Segment Tree y al arreglo del oráculo; comparar consultas después de cada modificación.
- [ ] Probar valores límite y rangos inválidos según el contrato.
- [ ] Ejecutar sanitizadores antes de medir; usar compilación optimizada independiente para benchmarks.

Aceptación: no existen discrepancias, accesos inválidos ni errores en empates o relleno del árbol.

### 2. Harness experimental

Propuesta inicial: cinco calentamientos y al menos 20 repeticiones por configuración, ajustables tras un piloto. Generar arreglos y consultas fuera del reloj, usar semillas fijas y reutilizar exactamente las mismas entradas en ambas estructuras. Consumir índices devueltos mediante checksum.

Medir construcción incluyendo las asignaciones que efectúa el constructor; excluir generación del arreglo y destrucción. La propiedad del arreglo debe ser igual en ambas estructuras para no favorecer una mediante referencias y otra mediante copias. Medir consultas con las estructuras ya construidas y por lotes; evitar medir una sola llamada muy corta.

Guardar muestras crudas: estructura, semilla, n, longitud del rango, número de consultas, fase, tiempo total, tiempo por consulta, checksum y bytes reservados. Registrar CPU, RAM, sistema operativo, compilador y flags. Alternar el orden de ejecución.

### 3. Baterías obligatorias

| Batería | Parámetros propuestos | Resultado |
|---|---|---|
| E1: construcción | n=2^10, 2^12, …, 2^22, condicionado por RAM | Tiempo de construcción contra n |
| E2: consultas | mismos n; longitudes distintas por n | Tiempo por consulta contra n y longitud |
| E3: espacio | mismos n | Bytes de constructores y comparación teórica |

Para E2, usar longitudes únicas y válidas de {1,2,8,64,floor(n/100),floor(n/10),floor(n/2),n}. Para cada longitud ℓ, elegir L uniformemente en [0,n−ℓ] y establecer R=L+ℓ−1. No elegir dos extremos uniformes para este experimento: eso no controla la longitud.

Empezar con lotes de 100 000 consultas y ajustar en el piloto; registrar el número final y normalizar por consulta. Incluir longitudes no potencia de dos. Para longitud n existe una única ubicación: declararlo. Usar valores aleatorios con duplicados; añadir arreglos constantes como prueba de empates, sin ampliar innecesariamente toda la batería.

Antes de ejecutar tamaños grandes, estimar memoria. Si se usan enteros e índices de 4 bytes, con n=2^22 una Sparse Table rectangular reserva aproximadamente 368 MiB solo para las celdas; el arreglo, logs y consultas añaden memoria. No ejecutar configuraciones que provoquen intercambio a disco y luego interpretarlas como costo algorítmico ordinario.

### 4. Espacio y gráficos

Contabilizar capacidad reservada, no solo elementos usados: arreglo de valores, tabla o árbol, logs, vectores auxiliares y metadatos relevantes. Separar buffers del benchmark y memoria común de entrada; evitar contar dos veces una copia. El RSS puede complementar, pero no sustituye el análisis de asignaciones exigido.

Producir construcción contra n; consulta contra n para varias longitudes; consulta contra longitud con n fijo; memoria contra n. Usar medianas y dispersión, unidades y número de muestras. Incluir tablas compactas y contrastar con las cotas teóricas.

## Ejercicio 4: plan de análisis algebraico

Definir primero el dominio de valores y el costo de combinar dos resultados. Las cotas siguientes cuentan combinaciones; equivalen a tiempo constante por combinación solo si la operación y su representación tienen costo O(1).

| Operación | Asociativa | Idempotente | Sparse Table clásica con bloques solapados | Segment Tree |
|---|---|---|---|---|
| Mínimo / máximo | Sí | Sí | Consulta O(1) | Consulta O(log n) |
| Suma | Sí | No | No válida; descomposición disjunta O(log n) | Consulta O(log n) |
| Producto | Sí | No | No válida; descomposición disjunta O(log n) | Consulta O(log n) |
| XOR | Sí | No | No válida; descomposición disjunta O(log n) | Consulta O(log n) |
| AND / OR | Sí | Sí | Consulta O(1) | Consulta O(log n) |
| MCD / MCM | Sí | Sí | O(1) combinaciones | O(log n) combinaciones |

### Argumentos que debe contener el informe

1. **Asociatividad:** permite reagrupar segmentos sin alterar el resultado. Sin ella, estas construcciones no responden en general la evaluación de un rango con una semántica de paréntesis distinta.
2. **Idempotencia:** permite combinar dos bloques que se superponen sin duplicar el aporte del solapamiento. Junto con asociatividad sustenta la consulta clásica O(1) de Sparse Table. No aplicar este esquema a suma, producto o XOR.
3. **Orden:** conmutatividad no es necesaria si se preserva el orden de los elementos. Demostrarlo con concatenación como ejemplo asociativo no conmutativo, distinguiendo costo de combinación y tamaño del resultado.
4. **Neutro:** facilita representar rangos vacíos, relleno y acumuladores del Segment Tree. No es estrictamente necesario si se manejan explícitamente resultados ausentes y solo se consultan rangos no vacíos.
5. **Inversos:** no son necesarios para ninguna de las dos estructuras. Si existe una operación de grupo, los prefijos pueden ser una alternativa estática con consultas O(1), respetando el orden de la cancelación; suma y XOR son ejemplos bajo dominios apropiados.
6. **Sparse Table sin idempotencia:** descomponer el rango en bloques disjuntos ofrece O(log n) combinaciones, conservando construcción y espacio O(n log n). Una Disjoint Sparse Table puede ofrecer O(1) para operaciones asociativas, pero es una variante distinta y opcional, no una propiedad de la tabla clásica implementada.
7. **Dominio y desbordamiento:** suma, producto y especialmente MCM pueden exceder enteros de ancho fijo. Definir aritmética exacta, detección de overflow o dominio acotado; no usar silenciosamente módulo porque cambia el problema. MCD no tiene costo constante en un modelo de enteros arbitrariamente grandes, y MCM puede producir resultados de muchos bits. Definir MCM(0,x)=0 y tratar los ceros antes de dividir.
8. **AND:** el neutro depende del universo de bits; para palabras de ancho fijo es la palabra con todos los bits a uno. No atribuir un neutro entero no negativo finito al AND sobre un universo de bits ilimitado.

El ejercicio 4 exige explicación, no implementar ocho baterías adicionales. Como verificación, bastan ejemplos pequeños de operaciones idempotentes, no idempotentes y no conmutativas; dar prioridad al análisis correcto.

## Discusión y entrega

Explicar que Segment Tree ofrece menor espacio asintótico y actualizaciones eficientes; Sparse Table prioriza consultas estáticas rápidas a cambio de más construcción y memoria. No afirmar que una es siempre mejor: considerar n, cantidad de consultas, memoria, actualizaciones y constantes medidas.

- [ ] Informe de 5–15 páginas con portada, estructuras, metodología, tablas, gráficos, ventajas/desventajas y ejercicio algebraico completo.
- [ ] Nombre completo y matrícula en portada.
- [ ] Código externo citado y componentes explicadas; registrar procedencia, revisión y licencia.
- [ ] README con comandos, formato de entradas y política de empates.
- [ ] Comprimido probado desde una copia limpia y nombres de entrega confirmados, con sufijo 04.

## Secuencia conjunta propuesta

Fechas orientativas, priorizando terminar antes del viernes indicado:

- **2 de noviembre de 2026:** contratos, implementaciones base y pruebas de ambos boletines.
- **3 de noviembre:** cerrar corrección y ejecutar pilotos; desarrollar el análisis algebraico.
- **4 de noviembre:** experimentos definitivos, memoria y gráficos; redactar metodología.
- **5 de noviembre:** resultados, conclusiones, revisión de requisitos y empaquetado de ambos trabajos.

Si falta tiempo, eliminar primero extensiones opcionales, no los experimentos obligatorios ni las pruebas de corrección. Mantener separados los dos PDF y los dos paquetes de fuentes.

Plan complementario: [[Boletin_03/Plan de implementación]].
