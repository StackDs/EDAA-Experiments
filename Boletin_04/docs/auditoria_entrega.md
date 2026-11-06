# Auditoría de entrega del Boletín 04

Fecha: 6 de noviembre de 2026.
Referencia: `Boletin4.md` y sesión `official-20261106-185540-Gmo0WTDo`.

## Resultado

No se detectaron discrepancias numéricas entre las tablas del informe y las
mediciones oficiales. Las inconsistencias documentales identificadas se
corrigieron sin alterar las mediciones, los generadores ni las estructuras.
El PDF final tiene 15 páginas, incluyendo portada, índice y referencias.

## Cumplimiento del enunciado

| Requisito | Evidencia |
| --- | --- |
| Implementar u obtener Sparse Table y Segment Tree | `include/range_queries/`, fuentes atribuidas al repositorio del curso |
| Explicar el código utilizado y citar su procedencia | Secciones de análisis teórico e implementación y referencias |
| Devolver el índice del mínimo | Pares valor/índice, desempate por menor índice y pruebas contra referencia lineal |
| Comparar construcción y consultas con distintos tamaños | Seis tamaños, seis longitudes, gráficos y tablas de resultados |
| Analizar asignaciones de memoria | Pasada independiente, memoria retenida y picos de construcción y mantenimiento |
| Comparar ventajas y desventajas | Sección «¿Qué opción utilizar?» y contraste de las cinco hipótesis |
| Generalizar a las operaciones solicitadas | Mínimo, máximo, suma, producto, XOR, AND, OR, MCD y MCM; propiedades, cotas y límites |
| Portada con nombre completo y matrícula | Bryan Eliseo Aguirre Fuentes, 2024443402 |
| Extensión de 5 a 15 páginas | 15 páginas físicas; referencias en página independiente |
| Código fuente en directorio comprimido y nombre de entrega | `aguirre_bryan_04.zip`, siguiendo la estructura del ZIP del Boletín 3 |

La generalización es teórica: no se presenta como una evaluación experimental
de operaciones diferentes de RMQ. Los ajustes necesarios para generalizar el
árbol están descritos, pero no se aplicaron al código medido.

## Verificaciones experimentales

- Cobertura completa: 88.704 muestras temporales y 72 registros de memoria.
  Desglose: 1.152 construcciones, 6.912 lotes estáticos, 11.520 mantenimientos
  y 69.120 lotes de consultas dinámicas.
- Seis tamaños: 1.000, 4.000, 16.000, 64.000, 256.000 y 1.000.000.
  Semillas: 1729, 2718 y 31415. Cada configuración conserva 32 repeticiones.
- Las 216 calibraciones estáticas alcanzaron el objetivo de 5 ms.
- Los 11.520 ciclos derivados contienen un cambio y 1.000 consultas; su
  tiempo es mantenimiento más consultas, sin repetir el mantenimiento por rango.
- Se recalcularon las medias por repetición, los resúmenes por semilla y
  los agregados gráficos, contrastándolos con los publicados. La dispersión
  presentada es la desviación muestral entre tres medias por semilla, no un
  intervalo de confianza ni 96 observaciones independientes.
- Se comprobaron los 120 valores numéricos de las cuatro tablas del informe,
  incluyendo redondeo y conversión entre ns, microsegundos, ms y MiB.
- Las muestras publicadas y las 18 figuras PNG coinciden con las de la sesión
  oficial. Se revisaron generación, referencias, límites del cronometraje,
  alternancia de métodos, reconstrucción, actualización y agregación de ciclos.
- `make check` terminó correctamente: encabezados independientes, generadores,
  RMQ, empates, actualizaciones, AddressSanitizer/UndefinedBehaviorSanitizer,
  validación de argumentos y pipeline pequeño con estadísticas y gráficos.
- `make report` compiló sin advertencias de LaTeX sobre desbordamientos o
  referencias indefinidas. Se comprobó el texto extraído del PDF y su maquetación.

Las comprobaciones nuevas usan datos pequeños y temporales: no reemplazan la
sesión oficial ni pretenden reproducir sus tiempos.

## Correcciones realizadas

1. Kernel y compilador alineados con `session.json`: Linux
   `7.2.14-arch1-1` y GCC 16.2.1, en lugar de la referencia genérica a kernel 6.x.
2. Metodología y presentación descritas como trabajo realizado, conservando
   explícitamente las hipótesis como predicciones previas.
3. README actualizado: resultados completos y dependencias para usar el código
   fuera del entorno virtual compartido del repositorio.
4. Aclaración de que el tiempo de consulta incluye el recorrido del lote y
   el cálculo de checksum, sin descontar ese costo común.
5. Inclusión explícita de los picos de construcción con un millón de elementos:
   320,44 MiB para Sparse Table y 30,52 MiB para Segment Tree.
6. Maquetación compactada sin reducir el tamaño de fuente ni eliminar ejercicios,
   conservando las referencias en página aparte y el límite de 15 páginas.
7. Corrección de portabilidad en `scripts/session_metadata.py`: una extracción
   sin repositorio Git interrumpía la preparación de experimentos. El registro
   de commit y estado ahora es opcional (`null` si no está disponible), con
   pruebas para Git disponible, ausente y carpeta sin commits. No se cambió
   el cronometraje ni el código de las estructuras.

## Salvedades de trazabilidad y alcance

Las huellas del motor de medición, generadores, estructuras y orquestación Bash
coinciden con las registradas en la sesión oficial. Las huellas históricas de
`plots/plot_experiments.py` y `scripts/check.py` **no coinciden** con las versiones
actuales; no se modificaron los metadatos para ocultarlo. El recálculo con el
analizador actual reproduce los CSV publicados y las sumas de ciclos. El ZIP
incluye la versión actual del código y los metadatos originales, no una copia
certificada de aquellas dos versiones históricas.
También difiere `scripts/session_metadata.py` por la corrección de portabilidad
descrita arriba; no se modificaron las huellas de las sesiones ya realizadas.

La afinidad amplia en `session.json` describe el proceso de orquestación; los
ejecutables de tiempos y memoria se lanzan mediante `taskset -c 2`, como indica
el script de la sesión. No se interpreta ese campo como una captura de la
afinidad interna de cada ejecutable.

Las conclusiones describen la máquina, implementaciones y cargas estudiadas:
no prueban significancia estadística, un umbral universal ni tiempos exactos
reproducibles. Las cotas de la generalización suponen agregados y combinaciones
de costo constante; el informe advierte sobre desbordamiento y precisión arbitraria.

## Composición de la entrega

La carpeta raíz del ZIP se llama `aguirre_bryan_04/` y contiene el PDF con ese
nombre y el proyecto `Boletin_04/`, siguiendo el patrón de la entrega anterior.
Se incluyen fuentes C++, Bash, Python y LaTeX, Makefile, README, enunciado,
`requirements.txt`, resultados por experimento y las sesiones oficial y piloto.
Se conserva también `report/Boletin04.pdf` para mantener válido el enlace del
README. Se excluyen toda la carpeta `docs`, ejecutables, entornos virtuales,
cachés, bloqueos y planes históricos locales. La integridad y los archivos extraídos se
verifican antes de entregarlo.

El fallo posterior `No package metadata was found for pandas` se reprodujo
con `python3` del sistema: no tenía instalada esa distribución. El entorno
virtual usado en las pruebas sí dispone de las tres dependencias. Se añadió
`requirements.txt` y se documentó tanto la instalación como el parámetro
`PYTHON` para seleccionar el entorno local en una extracción independiente.
No se instalaron paquetes automáticamente ni se alteraron las mediciones oficiales.
