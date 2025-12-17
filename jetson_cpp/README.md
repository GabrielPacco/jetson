# Jetson DQN Agent - Control de EV3 vía UDP

Sistema para controlar el robot EV3 desde el Jetson Nano, enviando comandos por UDP a un bridge en la laptop.

## Estado Actual del Proyecto

### ✅ COMPLETO - Infraestructura Básica

1. **Laptop Bridge** (`../laptop/`)
   - ✅ Servidor UDP funcionando correctamente
   - ✅ Control del EV3 por USB con ev3-dc
   - ✅ Watchdog de seguridad (STOP automático si no recibe comandos)
   - ✅ Logging completo de todas las acciones
   - ✅ **PROBADO Y FUNCIONANDO**

2. **Código Jetson Básico** (`./`)
   - ✅ Cliente UDP para enviar acciones
   - ✅ Política Random para testing
   - ✅ Arquitectura modular (Policy pattern)
   - ✅ CMake configurado (sin dependencias pesadas)
   - ✅ Compilación básica lista

### 🚧 EN DESARROLLO - Integración DQN

1. **Análisis del Repositorio jetson_test**
   - ✅ Código completo con LibTorch analizado
   - ✅ Estructura DQN profesional identificada
   - ⚠️ **FALTA**: Adaptar para UDP (actualmente usa Bluetooth)
   - ⚠️ **FALTA**: Integrar con el código actual

2. **Modelo DQN**
   - ❌ **NO ENTRENADO**: No hay modelo .pt disponible
   - ❌ **NO INTEGRADO**: Falta implementar DQNPolicy class
   - ❌ **LIBTORCH**: Pendiente instalar y configurar en Jetson

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

## Compilación y Ejecución

### Requisitos Mínimos (Modo Random)

- Jetson Nano (JetPack 4.x+)
- GCC/G++ con C++14
- CMake 3.10+
- pthread

### Compilar

```bash
cd jetson_cpp
mkdir -p build
cd build
cmake ..
make
```

### Ejecutar (Modo Random - Testing)

```bash
# Asegúrate de que el bridge esté corriendo en la laptop
# En otra terminal de la laptop:
#   cd laptop
#   python bridge.py

# En el Jetson:
./jetson_dqn <laptop_ip>

# Ejemplo:
./jetson_dqn 192.168.1.100
```

**Salida esperada:**
```
=========================================================================
  Jetson DQN Agent - Control de EV3 vía UDP
=========================================================================
Laptop Bridge:    192.168.1.100:5000
Frecuencia:       5 Hz
Política:         random
Presiona Ctrl+C para detener
=========================================================================

[Policy] Usando política aleatoria (testing mode)
[UDP] Listo para enviar a 192.168.1.100:5000
[Init] Enviando STOP inicial...

[Running] Iniciando loop de control...
=========================================================================
[17:30:45.123] Action: 1 (FORWARD)
[17:30:45.323] Action: 2 (TURN_LEFT)
[17:30:45.523] Action: 0 (STOP)
...
```

---

## Próximos Pasos (Roadmap)

### Fase 1: Testing Básico ✅ ACTUAL

**Objetivo:** Verificar que toda la comunicación funciona

- [x] Compilar código Jetson
- [x] Ejecutar con política Random
- [ ] **PROBAR**: Ejecutar Jetson + Bridge + EV3 juntos
- [ ] Verificar que el EV3 responde a comandos del Jetson
- [ ] Medir latencia UDP Jetson → Laptop

**Tiempo estimado:** 1-2 horas

---

### Fase 2: Integración DQN Básica ⏳ SIGUIENTE

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

### Fase 3: Entrenamiento en el EV3 Real 🎯 FINAL

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

### 🔴 Crítico (Sin esto no funciona DQN)

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

### 🟡 Importante (Para mejor rendimiento)

4. **Feedback de sensores**
   - Actualmente es "open-loop" (sin feedback)
   - Agregar lectura de sensores del EV3
   - Enviar estado de vuelta al Jetson

5. **Sistema de recompensas**
   - Definir qué comportamientos premiar
   - Implementar cálculo de rewards

### 🟢 Opcional (Mejoras)

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

### Test 1: Comunicación UDP ⏳
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
