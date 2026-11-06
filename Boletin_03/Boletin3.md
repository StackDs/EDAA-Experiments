

<!-- Start of picture text -->
 B Departamento de Ingenieria Informatica<br>0, y Ciencias de la Computacion<br>RY Universidad de Concepcion<br><!-- End of picture text -->

**Estructuras de Datos y Algoritmos Avanzados (2026-2)** 

Profesor: Jos´e Fuentes Ayudante: Leonardo Lovera 

# **Bolet´ın 3 String matching** 

## **Objetivos** 

Los objetivos del laboratorio son: 

- Entender el c´odigo de algoritmos y estructuras de datos de complejidad media. 

- Estudiar la documentaci´on de la biblioteca est´andar de C++ y utilizarla. 

- Evaluar experimentalmente implementaciones de algoritmos. 

## **Ejercicios** 

1. Implementar u obtener la implementaci´on de un _Suffix Array_ , el cual debe implementar una operaci´on de b´usqueda que retorne la cantidad de veces que se repite un patr´on en el texto sobre el que se construye la estructura. Se sugiere utilizar el c´odigo dispuesto por el ayudante en esta tarea mediante el repositorio del curso. En caso de utilizar c´odigo que no sea de autor´ıa propia, citar la fuente y comentar brevemente el c´odigo, explicando las componentes m´as importantes del mismo. 

2. Implementar u obtener la implementaci´on de _dos_ algoritmos de b´usqueda de patrones en cadenas de texto, los cuales deben retornar la cantidad de repeticiones de un patr´on en un texto. El algoritmo en cuesti´on puede ser _Knuth-Morris-Pratt_ , _Boyer-Moore_ o _Rabin-Karp_ . En caso de utilizar c´odigo que no sea de autor´ıa propia, citar la fuente y comentar brevemente el c´odigo, explicando las componentes m´as importantes del mismo. 

3. Evaluar experimentalmente el desempe˜no de ambas alternativas. Para esto, realizar un experimento donde se mida el efecto del largo del texto en los tiempos de ejecuci´on, y otro experimento para medir el efecto del largo del patr´on. Adicionalmente, realizar un experimento donde se mida el efecto del n´umero de ocurrencias en el tiempo de b´usqueda. Reportar los resultados a trav´es de gr´aficos y tablas. Se puede utilizar texto 

1 

de origen natural o sint´etico. En el repositorio _Pizza&Chili_ , cuya direcci´on es `http://pizzachili.dcc.uchile.cl/texts.html`<sup>1</sup> , se encuentran disponibles diversos ejemplos y generadores que se pueden utilizar en esta experimentaci´on. Adicionalmente, reportar el consumo de memoria de cada una de estas alternativas. Para el experimento que considera el n´umero de ocurrencias considerar, tambi´en, un texto que solamente tenga un caracter: que el patr´on sea ese caracter y que el patr´on sea otro caracter son casos extremos para el n´umero de ocurrencias (e.g. “AAAA...AAAA” y buscar el patr´on “A” o “B”). 

4. En base a los resultados obtenidos en el ejercicio anterior, adem´as de las complejidades obtenidas de cada alternativa a trav´es del an´alisis asint´otico, argumentar brevemente en qu´e escenarios conviene utilizar una alternativa o la otra. Tenga en cuenta no solamente el costo de la b´usqueda, sino que adem´as de la construcci´on, en los casos pertinentes. 

## **Observaci´on** 

## **Normas de entrega** 

Antes del viernes 6 de noviembre, se deben enviar todos los ejercicios resueltos mediante la plataforma CANVAS. 

Se debe considerar: 

- Archivo PDF correspondiente a un breve informe acerca del bolet´ın. La portada debe indicar el nombre completo y n´umero de matr´ıcula. 

- El informe del bolet´ın debe tener entre 5 y 15 p´aginas. 

- En el caso de usar c´odigo del repositorio, referenciarlo y explicarlo en el informe. 

- Todos los archivos del c´odigo fuente dentro de un directorio comprimido. 

- **IMPORTANTE:** Los archivos debe llamarse `apellido` ~~`n`~~ `ombre 03.formato` 

> 1Algunos de los ficheros que se encuentran en esta web no est´an codificados en ASCII, por lo que puede ser necesario realizar un preprocesado para ajustar la entrada 

2 

