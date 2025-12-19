# Sistema DQN para Control de Robot EV3

**Deep Q-Network (DQN) implementado en C++/CUDA sobre Jetson Xavier para control autónomo del robot LEGO Mindstorms EV3**

---

## 🎯 Proyecto Final - Deep Reinforcement Learning

Sistema de control inteligente que combina:
- ✅ **DQN en C++ con CUDA** (Jetson Xavier)
- ✅ **Aprendizaje y inferencia en Jetson** (no en PC externo)
- ✅ **Comunicación UDP** (Jetson → Laptop → EV3)
- ✅ **Control de robot físico** LEGO Mindstorms EV3

---

## 📐 Arquitectura del Sistema

```
┌──────────────────────────────────┐
│ JETSON XAVIER (jetson_cpp)       │
│                                  │
│  ┌─────────┐    ┌─────────────┐ │
│  │ Estado  │───>│  DQN Agent  │ │     UDP
│  │ (4D)    │    │  LibTorch   │─┼────────┐
│  └─────────┘    │  CUDA       │ │        │
│                 └─────────────┘ │        │
└──────────────────────────────────┘        │
                                            ▼
                                 ┌──────────────────┐
                                 │ LAPTOP (bridge)  │
                                 │   Python         │
                                 └────────┬─────────┘
                                          │ USB
                                          ▼
                                 ┌──────────────────┐
                                 │  EV3 ROBOT       │
                                 │  Motores A/D     │
                                 └──────────────────┘
```

**Flujo de Datos:**
1. **Estado** → DQN Agent (red neuronal)
2. **Acción** (0-4) → Enviada por UDP
3. **Bridge** traduce → Comando EV3
4. **Motores** ejecutan movimiento

**Acciones Disponibles:**
- `0`: STOP
- `1`: FORWARD (Motor A+D adelante)
- `2`: TURN_LEFT (A atrás, D adelante)
- `3`: TURN_RIGHT (A adelante, D atrás)
- `4`: BACKWARD (Motor A+D atrás)

**Safety:** Watchdog en bridge - STOP automático si no recibe comandos >0.5s

---

## 🚀 Inicio Rápido

### 1️⃣ Laptop (Windows) - Bridge Python

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

### 2️⃣ Jetson Xavier - Entrenamiento + Inferencia DQN

**Prerequisito:** LibTorch instalado (verificar ruta con `find /usr -name "libtorch" 2>/dev/null`)

**Paso 1: Compilar**
```bash
cd jetson_cpp
mkdir -p build && cd build

# IMPORTANTE: Reemplaza /ruta/a/libtorch con la ruta REAL en tu Jetson
cmake -DCMAKE_PREFIX_PATH=/ruta/a/libtorch -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

**Paso 2: Entrenar modelo (REQUERIDO por el proyecto)** ⭐
```bash
# Entrenar DQN en simulación (500 episodios, ~10-15 min)
./train_simulation 500

# Genera modelos en:
# - models/dqn_simulation_best.pt
# - models/dqn_simulation_final.pt
```

**Paso 3: Ejecutar inferencia con modelo entrenado**
```bash
# Con modelo entrenado (RECOMENDADO)
./jetson_dqn <laptop_ip> -p dqn -m models/dqn_simulation_best.pt

# Ejemplos:
./jetson_dqn 192.168.1.100 -p dqn -m models/dqn_simulation_best.pt
```

**Ver documentación completa:** `jetson_cpp/README.md`

---

## 📂 Estructura del Proyecto

```
Cloud_Final/
├── README.md              # Este archivo
├── config.py              # Configuración compartida
│
├── jetson_cpp/            # 🧠 DQN en C++ (JETSON XAVIER)
│   ├── include/dqn/       # Headers del agente DQN
│   │   ├── agent.h        # Agente DQN completo
│   │   ├── network.h      # Red neuronal (LibTorch)
│   │   ├── replay_buffer.h
│   │   └── types.h
│   ├── src/dqn/           # Implementación DQN
│   │   ├── agent.cpp      # ✓ Código probado
│   │   ├── network.cpp    # ✓ Código probado
│   │   └── replay_buffer.cpp
│   ├── main.cpp           # DQN + UDP integrado
│   ├── CMakeLists.txt     # Build con LibTorch
│   └── README.md          # Documentación detallada
│
└── laptop/                # 🌉 Bridge Python (LAPTOP WINDOWS)
    ├── bridge.py          # Servidor UDP → EV3
    └── ev3_controller.py  # Control de motores
```

**Tamaño total:** ~100KB (limpiado de archivos innecesarios)

---

## ⚙️ Componentes Principales

### 1. DQN Agent (Jetson Xavier)
- **Lenguaje:** C++17
- **Framework:** LibTorch (PyTorch C++ API)
- **Device:** CUDA (GPU) o CPU (auto-detect)
- **Arquitectura Red:** 4 → 128 → 128 → 5 (fully connected)
- **Estado:** Vector 4D `[gyro_x, gyro_y, contact_front, contact_side]`
- **Acciones:** 5 discretas `{STOP, FORWARD, LEFT, RIGHT, BACKWARD}`

### 2. Bridge (Laptop Windows)
- **Lenguaje:** Python 3
- **Protocolo:** UDP (puerto 5000)
- **Hardware:** USB → EV3 con ev3-dc library
- **Safety:** Watchdog timer (0.5s timeout)

### 3. EV3 Robot
- **Firmware:** LEGO original (NO ev3dev)
- **Motores:** A (izquierda) + D (derecha)
- **Conexión:** USB a laptop

---

## 🔧 Configuración

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

## 🐛 Troubleshooting

| Problema | Solución |
|----------|----------|
| `ev3dev2 not found` | `pip install python-ev3dev2` |
| EV3 no conecta | Verificar USB, encender EV3, Device Manager |
| No recibe UDP | Firewall Windows, permitir Python puerto 5000 |
| LibTorch not found | Verificar `/usr/local/libtorch` existe |
| CUDA out of memory | Automáticamente usa CPU como fallback |
| Robot va al revés | Editar `config.py` flags INVERT |

---

## 📖 Documentación Adicional

- **Jetson Xavier setup completo:** `jetson_cpp/README.md`
- **Detalles de implementación DQN:** Ver headers en `jetson_cpp/include/dqn/`
- **Configuración avanzada:** `config.py` (comentarios inline)

---

## ✅ Requisitos del Proyecto

- [x] Robot físico LEGO Mindstorms EV3
- [x] DQN implementado en C++ con CUDA
- [x] Aprendizaje e inferencia en Jetson Xavier
- [x] Control autónomo del robot
- [x] Código en repositorio GitHub
- [x] Documentación técnica completa

---

**Proyecto listo para compilar y ejecutar en Jetson Xavier + EV3**
