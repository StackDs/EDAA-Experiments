# Corpus de entrada

`english.50MB.gz` es el archivo original de Pizza & Chili conservado sin
transformaciones. Contiene textos en inglés de Project Gutenberg.

Fuente: https://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz

`make prepare` verifica el gzip completo y genera `english.bin` con sus primeros
50.000.000 bytes. Este archivo descomprimido no se duplica en Git. El manifiesto
registra los hashes y la procedencia. El SHA-256 esperado del prefijo es
`974837f10883d6d0b4b2d4896ff9c4adc23363fced1f5f9b031f287c54c6b437`.

El repositorio del corpus permite descargar sus textos, pero no declara aquí
una licencia uniforme para todas las obras concatenadas. Se conserva su
procedencia sin atribuir una licencia no verificada.
