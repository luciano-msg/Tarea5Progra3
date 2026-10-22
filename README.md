# Tarea #5 — Patrones de Diseño (CS2013 Programación III, 2026-2)

| Ejercicio | Patrón | Carpeta |
|---|---|---|
| 1 | Singleton — Transaction Manager concurrente | `ej1-singleton/` |
| 2 | Abstract Factory — Motor multiplataforma (OpenGL / Vulkan) | `ej2-abstract-factory/` |
| 3 | Composite — Expresiones matemáticas (evaluar y derivar) | `ej3-composite/` |
| 4 | Iterator — Recorrido BFS / DFS de grafo dirigido | `ej4-iterator/` |

## Compilar y ejecutar

Con CMake:

```bash
cmake -S . -B build
cmake --build build
./build/ej1_singleton
./build/ej2_abstract_factory
./build/ej3_composite
./build/ej4_iterator
```

O directo con g++/clang++ (C++17):

```bash
g++ -std=c++17 -pthread ej1-singleton/main.cpp -o ej1 && ./ej1
g++ -std=c++17 ej2-abstract-factory/main.cpp -o ej2 && ./ej2
g++ -std=c++17 ej3-composite/main.cpp -o ej3 && ./ej3
g++ -std=c++17 ej4-iterator/main.cpp -o ej4 && ./ej4
```

## Decisiones de diseño

### 1. Singleton
- Singleton de Meyers (`static` local en `getInstance()`): desde C++11 su inicialización es thread-safe.
- Constructor privado; copia y movimiento eliminados.
- `deposit`/`withdraw` toman un `std::lock_guard<std::mutex>`: verificación de saldo, modificación y registro de la `Transaction` ocurren como una sola sección crítica (atómica).
- Los retiros sin fondos se rechazan y quedan registrados como `RECHAZADA`.
- Cada hilo deposita 100 y retira 50, así que el balance final es `1000 + 10 * 50 = 1500`. Se incluye además una prueba de estrés (100 hilos × 1000 operaciones) y la impresión de la dirección del manager desde otro hilo para comprobar que es única.

### 2. Abstract Factory
- `RenderFactory` declara `createWindow`, `createButton`, `createShader` y devuelve `std::unique_ptr` a las interfaces `Window`, `Button`, `Shader`.
- `OpenGLFactory` y `VulkanFactory` crean productos concretos con comportamiento distinto.
- La plataforma se elige solo por la fábrica concreta que se instancia: no hay `if` ni `switch`. Cambiar el `unique_ptr<RenderFactory>` en tiempo de ejecución hace que los nuevos objetos sean de la nueva plataforma.
- `renderFrame(const RenderFactory&)` muestra un cliente que solo conoce la interfaz.

### 3. Composite
- `Expression` es el componente; `Constant` y `Variable` son hojas; `BinaryOperation` (`Add`, `Subtract`, `Multiply`, `Divide`, `Power`) y `UnaryFunction` (`Sin`, `Cos`, `Exp`, `Ln`, `Negate`) son compuestos.
- Las derivadas siguen reglas reales: suma, producto, cociente, potencia (exponente constante, base constante y caso general `f^g`) y regla de la cadena en `UnaryFunction`.
- Sin `dynamic_cast`: la simplificación (`0 + e`, `1 * e`, `e^1`, plegado de constantes) usa el método virtual `isConstant()`.
- `toString()` usa la precedencia de cada nodo para poner solo los paréntesis necesarios.
- El `main` verifica cada derivada simbólica contra diferencias finitas centradas.

Salida para la expresión del enunciado:

```
f(x) = (x^2 + 3 * x) * sin(x)
f'(x) = (2 * x + 3) * sin(x) + (x^2 + 3 * x) * cos(x)
f(2) = 9.09297
```

### 4. Iterator
- `Graph` es dueño de los nodos (`std::map<int, std::unique_ptr<Node>>`) y no expone la estructura interna: no hay getters de nodos ni de adyacencias; solo los iteradores (clases `friend`) pueden recorrerla.
- `GraphIterator` define `hasNext()`, `next()` y `reset()`. `BFSIterator` usa cola y `DFSIterator` usa pila (iterativo, sin recursión).
- Ambos llevan un conjunto de visitados, así que los grafos con ciclos (incluidos self-loops) terminan.
- El iterador se cambia en tiempo de ejecución reasignando el `std::unique_ptr<GraphIterator>`; la función `traverse(GraphIterator&)` funciona con cualquier estrategia.
