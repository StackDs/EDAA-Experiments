# Corpus de entrada

`english.50MB.gz` es el archivo original de Pizza & Chili conservado sin
transformaciones. Contiene textos en inglés de Project Gutenberg.

Fuente: https://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz

`make prepare` verifica el gzip completo y genera `english.bin` con sus primeros
10.000.000 bytes. Este archivo descomprimido no se duplica en Git. El manifiesto
registra los hashes y la procedencia. El SHA-256 esperado del prefijo es
`ab426bd5952843c190cc9705f94dfe42a1fbbc53dcb06f480ebd21facea6d9f7`.

El repositorio del corpus permite descargar sus textos, pero no declara aquí
una licencia uniforme para todas las obras concatenadas. Se conserva su
procedencia sin atribuir una licencia no verificada.
