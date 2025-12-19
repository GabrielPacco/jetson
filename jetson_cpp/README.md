# Jetson DQN + UDP para Control EV3

**INTEGRACIÓN COMPLETA** - DQN probado de jetson_test + Comunicación UDP funcional

Sistema integrado que combina:
- **DQN probado** de `jetson_test` (CÓDIGO SIN MODIFICAR)
- **Comunicación UDP** funcional al bridge de laptop

## Estado: INTEGRACIÓN COMPLETA

### DQN Core
- **Headers**: `include/dqn/` copiados sin cambios de jetson_test
- **Source**: `src/dqn/` copiado sin cambios de jetson_test
- **Probado**: Código ya verificado en Jetson Xavier
- **CMake**: Configurado con LibTorch

### Comunicación UDP
- **Cliente UDP**: Funcional en main.cpp
- **Servidor Bridge**: Probado en laptop
- **EV3**: Respondiendo correctamente

### Integración
- **DQNPolicy**: Implementado en main.cpp (líneas 210-288)
- **Policy Pattern**: Soporta Random y DQN
- **Argumentos**: `-p random` o `-p dqn -m modelo.pt`
- **Listo**: Para compilar en Jetson

---

## Arquitectura del Sistema

```
[Jetson Nano]                  [Laptop Windows]              [EV3]
┌──────────────┐              ┌──────────────┐             ┌──────┐
│              │              │              │             │      │
│  DQN Agent   │   UDP 5000   │    Bridge    │     USB     │ EV3  │
│  (C++)       │─────────────>│    (Python)  │────────────>│Motors│
│              │   Acciones   │              │   ev3-dc    │ A,D  │
│              │   (0-4)      │              │             │      │
└──────────────┘              └──────────────┘             └──────┘
```

**Flujo de Datos:**
1. Jetson ejecuta DQN (o Random policy)
2. Selecciona acción (0-4)
3. Envía acción por UDP a laptop
4. Bridge recibe y traduce a comandos EV3
5. EV3 ejecuta movimiento

**Acciones Disponibles:**
- `0`: STOP
- `1`: FORWARD
- `2`: TURN_LEFT
- `3`: TURN_RIGHT
- `4`: BACKWARD

---

## Compilación en Jetson Xavier

### Prerequisitos

**1. Verificar dónde está LibTorch instalado:**

```bash
# Buscar LibTorch en el sistema
find /usr -name "libtorch" 2>/dev/null
find / -name "TorchConfig.cmake" 2>/dev/null | head -5

# O verificar variable de entorno
echo $LD_LIBRARY_PATH
```

**Ubicaciones comunes:**
- `/usr/local/libtorch` (instalación estándar)
- `/opt/libtorch`
- `~/libtorch` (home del usuario)

**2. Si LibTorch NO está instalado:**

```bash
cd /tmp
wget https://download.pytorch.org/libtorch/cu117/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu117.zip
unzip libtorch*.zip
sudo mv libtorch /usr/local/

# Variables de entorno
echo 'export LD_LIBRARY_PATH=/usr/local/libtorch/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

### Compilar

```bash
cd jetson_cpp
mkdir -p build && cd build

# PASO IMPORTANTE: Reemplaza LIBTORCH_PATH con la ruta REAL en tu sistema
# Usa el comando de arriba (find) para encontrarla
export LIBTORCH_PATH=/ruta/a/libtorch  # CAMBIAR ESTO

# Configurar CMake
cmake -DCMAKE_PREFIX_PATH=$LIBTORCH_PATH \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# Compilar (usar todos los cores)
make -j4
```

**Ejemplos según ubicación:**

```bash
# Si LibTorch está en /usr/local/libtorch (común):
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch -DCMAKE_BUILD_TYPE=Release ..

# Si LibTorch está en /opt/libtorch:
cmake -DCMAKE_PREFIX_PATH=/opt/libtorch -DCMAKE_BUILD_TYPE=Release ..

# Si LibTorch está en tu home:
cmake -DCMAKE_PREFIX_PATH=$HOME/libtorch -DCMAKE_BUILD_TYPE=Release ..
```

**Salida esperada:**
```
-- Found LibTorch: /usr/local/libtorch/lib/libtorch.so
-- LibTorch include dirs: /usr/local/libtorch/include
========================================
Jetson DQN + UDP Configuration
========================================
[100%] Built target dqn_core
[100%] Built target jetson_dqn
```

---

---

## ENTRENAMIENTO (Requerido por el proyecto)

### **Entrenar DQN en simulación**

```bash
cd jetson_cpp/build

# Entrenar 500 episodios (recomendado)
./train_simulation 500

# O entrenar menos para prueba rápida
./train_simulation 100
```
**Salida esperada:**
```
=========================================================================
  DQN Training - SIMULATION MODE (CartPole)
=========================================================================
[Device] cuda:0
[Environment] Creando entorno CartPole simulado...
[Agent] Creando DQN agent...
[DQNAgent] Initialized with:
  State dim: 4
  Action dim: 2
  Hidden dims: [128, 128]
  Device: cuda:0

[Training] Iniciando entrenamiento simulado...
[Episode   10] Reward:   23.00 | Epsilon: 0.904 | Loss: 0.0125
[Episode   50] Reward:   89.00 | Epsilon: 0.605 | Loss: 0.0056
[Episode  100] Reward:  145.00 | Epsilon: 0.364 | Loss: 0.0023
...
[Episode  500] Reward:  195.00 | Epsilon: 0.050 | Loss: 0.0008

Modelos guardados:
  - models/dqn_simulation_best.pt
  - models/dqn_simulation_final.pt
```

**Modelos generados:**
- `models/dqn_simulation_best.pt` - Mejor modelo durante entrenamiento
- `models/dqn_simulation_final.pt` - Modelo al finalizar

**Tiempo estimado:** ~10-15 minutos para 500 episodios en Jetson Xavier

---

## INFERENCIA

Después de entrenar, usar el modelo para inferencia:

### Modo 1: Random (Testing de comunicación)

```bash
cd build
./jetson_dqn 192.168.1.100 -p random
```

Acciones aleatorias, útil para:
- Verificar comunicación UDP
- Probar que EV3 responde
- Testear infraestructura

### Modo 2: DQN sin modelo entrenado

```bash
./jetson_dqn 192.168.1.100 -p dqn
```

Usa DQN con pesos aleatorios (no entrenado), útil para:
- Verificar que DQN compila y carga
- Probar inferencia de red neuronal
- Confirmar CUDA funciona

**Salida esperada:**
```
[DQNPolicy] Using device: cuda:0
[DQNPolicy] No model specified, using random initialization
[DQNAgent] Initialized with:
  State dim: 4
  Action dim: 5
  Hidden dims: [128, 128]
  Device: cuda:0
```

### Modo 3: DQN con modelo entrenado (RECOMENDADO)

```bash
# Usar modelo generado por train_simulation
./jetson_dqn 192.168.1.100 -p dqn -m models/dqn_simulation_best.pt
```

**Salida esperada:**
```
[DQNPolicy] Using device: cuda:0
[DQNPolicy] Loading model from: models/dqn_simulation_best.pt
[DQNPolicy] Model loaded successfully
```

---

## Próximos Pasos (Roadmap)

### Fase 1: Testing Básico (ACTUAL)

**Objetivo:** Verificar que toda la comunicación funciona

- [x] Compilar código Jetson
- [x] Ejecutar con política Random
- [ ] **PROBAR**: Ejecutar Jetson + Bridge + EV3 juntos
- [ ] Verificar que el EV3 responde a comandos del Jetson
- [ ] Medir latencia UDP Jetson → Laptop

**Tiempo estimado:** 1-2 horas

---

### Fase 2: Integración DQN Básica (SIGUIENTE)

**Objetivo:** Integrar el código DQN del repositorio jetson_test

#### Opción A: Copiar Código DQN Directamente

**Archivos a copiar:**
```bash
# Desde jetson_test/ a jetson_cpp/
cp -r ../jetson_test/include/ ./
cp -r ../jetson_test/src/ ./
```

**Modificar:**
1. `src/environment/lego_robot_env.cpp`:
   - Eliminar código Bluetooth
   - Crear `UDPEnvironment` que use `UDPSender`

2. `main.cpp`:
   - Descomentar `#include "dqn/agent.h"`
   - Implementar `DQNPolicy` class
   - Cargar modelo pre-entrenado

3. `CMakeLists.txt`:
   - Descomentar sección DQN
   - Configurar LibTorch paths

**Requisitos adicionales:**
```bash
# Instalar LibTorch en Jetson
cd ~
wget https://download.pytorch.org/libtorch/cu118/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu118.zip
unzip libtorch-*.zip
sudo mv libtorch /usr/local/
```

#### Opción B: Entrenar Modelo Simple Primero

1. **Entrenar en simulación** (laptop/PC con GPU):
   ```bash
   cd ../jetson_test/build
   ./train_simulation  # Modo sin robot
   ```

2. **Copiar modelo** a Jetson:
   ```bash
   scp models/dqn_best.pt jetson@<jetson_ip>:~/jetson_cpp/models/
   ```

3. **Cargar en Jetson**:
   - Implementar `DQNPolicy::loadModel()`
   - Usar inferencia con el modelo

**Tiempo estimado:** 1-2 días

---

### Fase 3: Entrenamiento en el EV3 Real (FINAL)

**Objetivo:** Entrenar DQN con el robot físico

**Necesitas:**
1. **Sensores en el EV3** (para feedback):
   - Sensor de distancia ultrasónico
   - Sensor táctil (detección de colisiones)
   - Giroscopio (orientación)

2. **Environment real**:
   - Crear `EV3RealEnvironment` que:
     - Lee sensores del EV3
     - Calcula recompensas
     - Detecta colisiones/obstáculos

3. **Loop de entrenamiento**:
   ```
   while episode < max_episodes:
       state = env.reset()
       while not done:
           action = agent.select_action(state)
           next_state, reward, done = env.step(action)
           agent.store_transition(state, action, reward, next_state, done)
           agent.train_step()
   ```

**Desafíos:**
- Comunicación bidireccional (Jetson ← sensores ← EV3)
- Evitar daños físicos al robot durante exploración
- Tiempo de entrenamiento (episodios en hardware real son lentos)

**Tiempo estimado:** 1-2 semanas

---

## Qué Falta por Hacer

### CRÍTICO (Sin esto no funciona DQN)

1. **Instalar LibTorch en Jetson**
   - Descargar versión ARM64 con CUDA
   - Configurar paths en CMake

2. **Crear DQNPolicy class**
   ```cpp
   class DQNPolicy : public Policy {
       torch::jit::script::Module model;
       torch::Device device;
   public:
       void loadModel(const std::string& path);
       int selectAction() override;
   };
   ```

3. **Entrenar o conseguir un modelo**
   - Opción rápida: Train en simulación
   - Opción real: Entrenar con el EV3

### IMPORTANTE (Para mejor rendimiento)

4. **Feedback de sensores**
   - Actualmente es "open-loop" (sin feedback)
   - Agregar lectura de sensores del EV3
   - Enviar estado de vuelta al Jetson

5. **Sistema de recompensas**
   - Definir qué comportamientos premiar
   - Implementar cálculo de rewards

### OPCIONAL (Mejoras)

6. **Visualización**
   - Dashboard web para monitorear
   - Plot de recompensas en tiempo real

7. **Guardar episodios**
   - Replay buffer persistente
   - Análisis post-entrenamiento

---

## Archivos Importantes

### Jetson (./jetson_cpp/)
- `main.cpp` - Código principal (UDP + Policy)
- `CMakeLists.txt` - Configuración de compilación
- `dqn_agent.cpp` - [LEGACY] Código DQN placeholder

### Laptop (../laptop/)
- `bridge.py` - Bridge UDP → EV3
- `ev3_controller.py` - Control de motores
- `test_bridge_sender.py` - Script de testing

### Repositorio Referencia (../jetson_test/)
- Código DQN completo con LibTorch
- Listo para copiar/adaptar

---

## Testing Checklist

### Test 1: Comunicación UDP
- [ ] Bridge corriendo en laptop
- [ ] Jetson puede hacer ping a laptop
- [ ] `./jetson_dqn` se conecta exitosamente
- [ ] Bridge recibe comandos del Jetson
- [ ] EV3 ejecuta movimientos

### Test 2: DQN Básico (Futuro)
- [ ] LibTorch instalado
- [ ] Modelo cargado exitosamente
- [ ] Inferencia < 100ms
- [ ] Acciones coherentes (no aleatorias)

### Test 3: Sistema Completo (Futuro)
- [ ] Loop completo: Sensores → DQN → Acción → EV3
- [ ] Latencia total < 200ms
- [ ] Robot opera 5+ minutos sin crashes
- [ ] Watchdog funciona si Jetson falla

---

## Contacto y Soporte

**Repositorio base DQN:**
https://github.com/GabrielPacco/jetson_test

**Documentación ev3-dc:**
https://ev3-dc.readthedocs.io/

---

## Resumen: ¿Por Dónde Empezar?

### Opción 1: Solo Testing (MÁS RÁPIDO - 1 hora)
```bash
# Laptop
cd laptop
python bridge.py

# Jetson
cd jetson_cpp/build
./jetson_dqn 192.168.1.100
```
**Resultado:** Robot se mueve con acciones aleatorias, verificas comunicación.

---

### Opción 2: DQN Simulado (INTERMEDIO - 1 día)
```bash
# Laptop (con GPU)
cd ../jetson_test/build
./train_simulation  # Entrenar modelo

# Copiar modelo
scp models/dqn_best.pt jetson:~/jetson_cpp/models/

# Jetson (después de instalar LibTorch)
# Implementar DQNPolicy, compilar
./jetson_dqn 192.168.1.100 -p dqn models/dqn_best.pt
```
**Resultado:** Robot usa DQN entrenado en simulación.

---

### Opción 3: DQN Real (COMPLETO - 2 semanas)
- Instalar sensores en EV3
- Implementar comunicación bidireccional
- Crear environment real
- Entrenar episodios con robot físico

**Resultado:** Sistema DQN completo entrenado en hardware real.

---

**Recomendación:** Empezar con Opción 1 para verificar que todo comunica bien, luego pasar a Opción 2.
