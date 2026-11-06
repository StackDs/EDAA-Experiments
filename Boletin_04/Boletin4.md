

<!-- Start of picture text -->
 B Departamento de Ingenieria Informatica<br>0, y Ciencias de la Computacion<br>RY Universidad de Concepcion<br><!-- End of picture text -->

**Estructuras de Datos y Algoritmos Avanzados (2026-2)** 

Profesor: Jos´e Fuentes Ayudante: Leonardo Lovera 

# **Bolet´ın 4 Range Queries** 

## **Objetivos** 

Los objetivos del laboratorio son: 

- Implementar y entender el c´odigo de estructuras de datos de complejidad media para el problema de Range Queries. 

- Evaluar experimentalmente estructuras de datos para el problema de Range Queries. 

## **Descripci´on** 

Dentro del contenido te´orico de la asignatura, se present´o el problema de _Range Minimum Query_ (RMQ). En este problema, dado un arreglo de enteros _A_ y una serie de operaciones de consultas ( _L, R_ ), se busca responder para cada consulta el ´ındice del elemento m´ınimo en el subarreglo _A_ [ _L..R_ ]. Este problema tiene muchas aplicaciones, y por lo tanto tambi´en se han desarrollado m´ultiples soluciones. En este bolet´ın se explorar´an dos variantes al problema: la est´atica, en la cu´al los valores del arreglo _A_ se mantienen fijos, y la variante din´amica, en la cual se debe soportar adem´as la operaci´on _update_ , en la cu´al se modifica el contenido del arreglo _A_ . Para cada una de estas variantes se analizar´a una soluci´on, las cuales son _Sparse Table_ y _Segment Tree_ respectivamente. 

## **Ejercicios** 

1. Implementar u obtener la implementaci´on de un _Segment Tree_ y de una _Sparse Table_ . Se sugiere utilizar las implementaciones disponibles en el repositorio oficial del curso. 

2. Comparar experimentalmente ambas soluciones para arreglos de diferentes tama˜nos. Se deben comparar: 1) tiempo de construcci´on 2) tiempo de consulta (considerar rangos aleatorios de distintos tama˜nos). Adem´as, se 

1 

debe hacer un breve an´alisis con respecto al espacio utilizado (considere las asignaciones de memoria en los m´etodos constructores). Los resultados se deben reportar a trav´es de tablas y gr´aficas. 

3. Responda a la siguiente pregunta. ¿Cu´ales son las principales ventajas y desventajas de cada una de las alternativas propuestas? 

4. Calcular el m´ınimo en un rango _A_ [ _L..R_ ] no es lo ´unico interesante que se puede hacer sobre el mismo. Por ejemplo, puede ser interesante querer saber el m´aximo, la suma, el producto, el XOR, el AND, el OR, el m´aximo com´un divisor, el m´ınimo com´un m´ultiplo, etc. de los elementos del rango _A_ [ _L..R_ ] (todas estas operaciones se pueden realizar sobre enteros no negativos). ¿Es posible adaptar las estructuras previas para responder a las consultas definidas por las operaciones anteriores? Si no, explicar cu´ales son las limitaciones que lo impiden; si s´ı, explicar si se mantienen las garant´ıas te´oricas temporales y espaciales que se tienen para estas estructuras cuando la consulta es de m´ınimo; adem´as, generalizar los resultados obtenidos a cualquier operaci´on _binaria f_ : determinar las propiedades que esta operaci´on debe tener para poder usar las estructuras y qu´e tipo de garant´ıas te´oricas ofrecen las propiedades.<sup>1</sup> 

## **Observaci´on** 

## **Normas de entrega** 

Antes del viernes 6 de noviembre, se deben enviar todos los ejercicios resueltos mediante la plataforma CANVAS. 

Se debe considerar: 

- Archivo PDF correspondiente a un breve informe acerca del bolet´ın. La portada debe indicar el nombre completo y n´umero de matr´ıcula. 

- El informe del bolet´ın debe tener entre 5 y 15 p´aginas. 

- En el caso de usar c´odigo del repositorio, referenciarlo y explicarlo en el informe. 

- Todos los archivos del c´odigo fuente dentro de un directorio comprimido. 

- **IMPORTANTE:** Los archivos debe llamarse `apellido` ~~`n`~~ `ombre 04.formato` 

> 1Propiedades relevantes son las de ´algebra: asociatividad, existencia de un elemento neutro, existencia de inversos, conmutatividad, idempotencia, etc. 

2 

