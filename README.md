# Sistema DQN para Control de Robot EV3

**Deep Q-Network (DQN) implementado en C++/CUDA sobre Jetson Xavier para control autónomo del robot LEGO Mindstorms EV3**

---

## Proyecto Final - Deep Reinforcement Learning

Sistema de control inteligente que combina:
- **DQN en C++ con CUDA** (Jetson Xavier)
- **Aprendizaje y inferencia en Jetson** (no en PC externo)
- **Comunicación UDP** (Jetson → Laptop → EV3)
- **Control de robot físico** LEGO Mindstorms EV3

---

## Arquitectura del Sistema (BIDIRECCIONAL)

```
┌──────────────────────────────────────────────┐
│ JETSON XAVIER (jetson_cpp)                   │
│                                              │
│  ┌─────────┐    ┌─────────────┐             │
│  │ Estado  │───>│  DQN Agent  │             │
│  │ (4D)    │    │  LibTorch   │             │
│  │sensores │<───│  CUDA       │             │
│  └─────────┘    └─────────────┘             │
│       ▲              │                       │
│       │              ▼                       │
│   Sensores      Acciones                    │
│   (UDP)         (UDP)                        │
└───────┼──────────────┼────────────────────────┘
        │              │
        ▼              ▼
┌──────────────────────────────────────────────┐
│ LAPTOP (bridge.py)        BIDIRECCIONAL      │
│                                              │
│  ┌──────────┐          ┌──────────┐         │
│  │ Sensores │<─── USB ─│  EV3 API │         │
│  │  UDP ↑   │          │  USB ↓   │         │
│  └──────────┘          └──────────┘         │
└──────────────────────────┬───────────────────┘
                           │ USB
                           ▼
                 ┌──────────────────────┐
                 │  EV3 ROBOT           │
                 │  - Motores A/D       │
                 │  - Gyro (Puerto 2)   │
                 │  - Touch (opcional)  │
                 └──────────────────────┘
```

**Flujo de Datos (BIDIRECCIONAL):**
1. **DQN Agent** → Acción (0-4) → UDP → Bridge
2. **Bridge** → Comando USB → **EV3 ejecuta** acción
3. **EV3** → Lee sensores (gyro + touch) → **Bridge**
4. **Bridge** → Sensores UDP → **Jetson**
5. **Jetson** → Actualiza estado → **DQN Agent**

**Acciones Disponibles:**
- `0`: STOP
- `1`: FORWARD (Motor A+D adelante)
- `2`: TURN_LEFT (A atrás, D adelante)
- `3`: TURN_RIGHT (A adelante, D atrás)
- `4`: BACKWARD (Motor A+D atrás)

**Safety:** Watchdog en bridge - STOP automático si no recibe comandos >0.5s

---

## Inicio Rápido

### 1. Laptop (Windows) - Bridge Python

**Instalar dependencia:**
```cmd
pip install python-ev3dev2
```

**Conectar EV3:**
- Cable USB a laptop
- Encender EV3
- Verificar en Device Manager: "LEGO EV3"

**Ejecutar bridge:**
```cmd
cd laptop
python bridge.py
```

### 2. Jetson Xavier - Entrenamiento + Inferencia DQN

**Prerequisito:** LibTorch instalado (verificar ruta con `find /usr -name "libtorch" 2>/dev/null`)

**Paso 1: Compilar**
```bash
cd jetson_cpp
mkdir -p build && cd build

# IMPORTANTE: Reemplaza /ruta/a/libtorch con la ruta REAL en tu Jetson
cmake -DCMAKE_PREFIX_PATH=/ruta/a/libtorch -DCMAKE_BUILD_TYPE=Release ..
make -j4

# Se generan 3 executables:
# - train_simulation (entrenamiento en simulación)
# - train_robot (entrenamiento con robot real)
# - jetson_dqn (inferencia)
```

**Paso 2A: Entrenar en SIMULACIÓN (rápido, sin robot)**
```bash
# Entrenar DQN en simulación CartPole (500 episodios, ~10-15 min)
./train_simulation 500

# Genera modelos en:
# - models/dqn_simulation_best.pt
# - models/dqn_simulation_final.pt
```

**Paso 2B: Entrenar con ROBOT REAL (con sensores) - CUMPLE REQUISITO**
```bash
# IMPORTANTE: Antes de ejecutar
# 1. Inicia bridge en laptop: python bridge.py
# 2. Conecta EV3 con giroscopio en Puerto 2

# Entrenar con robot real (100 episodios, ~30-60 min)
./train_robot <laptop_ip> 100

# Ejemplos:
./train_robot 192.168.1.100 100

# Genera modelos en:
# - models/dqn_robot_best.pt
# - models/dqn_robot_final.pt
```

**Paso 3: Ejecutar inferencia con modelo entrenado**
```bash
# Con modelo de simulación
./jetson_dqn <laptop_ip> -p dqn -m models/dqn_simulation_best.pt

# Con modelo de robot real (RECOMENDADO)
./jetson_dqn 192.168.1.100 -p dqn -m models/dqn_robot_best.pt
```

**Ver documentación completa:** `jetson_cpp/README.md`

---

## Estructura del Proyecto

```
Cloud_Final/
├── README.md              # Este archivo
├── config.py              # Configuración compartida
│
├── jetson_cpp/            # DQN en C++ (JETSON XAVIER)
│   ├── include/dqn/       # Headers del agente DQN
│   │   ├── agent.h        # Agente DQN completo
│   │   ├── network.h      # Red neuronal (LibTorch)
│   │   ├── replay_buffer.h
│   │   └── types.h
│   ├── src/dqn/           # Implementación DQN
│   │   ├── agent.cpp      # Código probado
│   │   ├── network.cpp    # Código probado
│   │   └── replay_buffer.cpp
│   ├── main.cpp           # DQN + UDP integrado
│   ├── CMakeLists.txt     # Build con LibTorch
│   └── README.md          # Documentación detallada
│
└── laptop/                # Bridge Python (LAPTOP WINDOWS)
    ├── bridge.py          # Servidor UDP → EV3
    └── ev3_controller.py  # Control de motores
```

**Tamaño total:** ~100KB (limpiado de archivos innecesarios)

---

## Componentes Principales

### 1. DQN Agent (Jetson Xavier)
- **Lenguaje:** C++17
- **Framework:** LibTorch (PyTorch C++ API)
- **Device:** CUDA (GPU) o CPU (auto-detect)
- **Arquitectura Red:** 4 → 128 → 128 → 5 (fully connected)
- **Estado:** Vector 4D `[gyro_angle_norm, gyro_rate_norm, touch_front, touch_side]`
  - `gyro_angle_norm`: Ángulo del giroscopio normalizado [-1, 1]
  - `gyro_rate_norm`: Velocidad angular normalizada [-1, 1]
  - `touch_front`: Sensor táctil frontal {0, 1}
  - `touch_side`: Sensor táctil lateral {0, 1}
- **Acciones:** 5 discretas `{STOP, FORWARD, LEFT, RIGHT, BACKWARD}`
- **Lectura de sensores:** En tiempo real del EV3 vía UDP

### 2. Bridge (Laptop Windows) - BIDIRECCIONAL
- **Lenguaje:** Python 3
- **Protocolo:** UDP bidireccional (puerto 5000)
- **Hardware:** USB → EV3 con ev3-dc library
- **Funciones:**
  - Recibe acciones del Jetson (UDP)
  - Envía comandos al EV3 (USB)
  - Lee sensores del EV3 (USB): Gyro + Touch
  - Envía sensores al Jetson (UDP)
- **Safety:** Watchdog timer (0.5s timeout)

### 3. EV3 Robot (Gyro Boy)
- **Firmware:** LEGO original (NO ev3dev)
- **Motores:** A (izquierda) + D (derecha)
- **Sensores:**
  - Puerto 2: Gyro Sensor (OBLIGATORIO)
  - Puerto 1/4: Touch Sensors (OPCIONAL)
- **Conexión:** USB a laptop

---

## Configuración

### Ajustar velocidades (config.py)
```python
SPEED_FORWARD = 40  # Velocidad adelante/atrás
SPEED_TURN = 30     # Velocidad de giro
```

### Invertir dirección si es necesario
```python
INVERT_FORWARD = True   # Invierte adelante/atrás
INVERT_TURNS = True     # Invierte izquierda/derecha
```

---

## Troubleshooting

| Problema | Solución |
|----------|----------|
| `ev3dev2 not found` | `pip install python-ev3dev2` |
| EV3 no conecta | Verificar USB, encender EV3, Device Manager |
| No recibe UDP | Firewall Windows, permitir Python puerto 5000 |
| LibTorch not found | Verificar `/usr/local/libtorch` existe |
| CUDA out of memory | Automáticamente usa CPU como fallback |
| Robot va al revés | Editar `config.py` flags INVERT |

---

## Documentación Adicional

- **Jetson Xavier setup completo:** `jetson_cpp/README.md`
- **Detalles de implementación DQN:** Ver headers en `jetson_cpp/include/dqn/`
- **Configuración avanzada:** `config.py` (comentarios inline)

---

## Requisitos del Proyecto - CUMPLIDOS

- **Robot físico LEGO Mindstorms EV3** Gyro Boy
- **DQN implementado en C++ con CUDA** (LibTorch en `jetson_cpp/`)
- **Captura de sensores del Gyro Boy** (giroscopio + touch vía USB/UDP)
- **Aprendizaje en Jetson Xavier:**
  - `train_simulation`: Entrenamiento en simulación (CartPole)
  - `train_robot`: Entrenamiento con robot real y sensores (CUMPLE REQUISITO)
- **Inferencia en Jetson Xavier** (`jetson_dqn` con lectura de sensores)
- **Control autónomo del robot** (acciones basadas en estado de sensores)
- **Comunicación bidireccional** (Jetson ↔ Bridge ↔ EV3)
- **Código en repositorio GitHub** (Cloud_Final)
- **Documentación técnica completa** (README.md + jetson_cpp/README.md)

---

**Proyecto listo para compilar y ejecutar en Jetson Xavier + EV3**
