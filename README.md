# DQN Jetson → EV3 Control

Control robótico distribuido: **Jetson (DQN C++)** → **Laptop (Puente Python)** → **EV3 (Motores A/D)**

---

## Arquitectura

```
Jetson Nano          Laptop Windows        EV3 Robot
┌────────────┐      ┌─────────────┐      ┌──────────┐
│ DQN (C++)  │ UDP  │ Bridge (Py) │ USB  │ Motor A  │
│ + Cámara   │─────>│ Safety      │─────>│ Motor D  │
│ OpenCV     │      │ Relay       │      └──────────┘
└────────────┘      └─────────────┘
 Acción 0-4          Logs/Timeout        A=izq, D=der
```

**Acciones**:
- 0: STOP
- 1: FORWARD (A=+, D=+)
- 2: TURN_LEFT (A=-, D=+)
- 3: TURN_RIGHT (A=+, D=-)
- 4: BACKWARD (A=-, D=-)

**Safety**: Si laptop no recibe del Jetson por >0.5s → STOP automático

---

## Setup (5 min)

### 1. Red
- Conecta Jetson y Laptop al mismo router
- Anota IP de laptop: `ipconfig` en Windows
- Edita `config.py` línea 10 con esa IP

### 2. USB
- Conecta EV3 a laptop por cable USB mini
- Enciende EV3
- Verifica en Device Manager (Windows): "LEGO EV3"

### 3. Software
```cmd
pip install python-ev3dev2
```

---

## Testing (paso a paso)

### Test 1: Motores EV3
```cmd
cd laptop
python ev3_controller.py
```
Confirma que A y D giran.

### Test 2: Bridge + Test Local
**Terminal 1**:
```cmd
cd laptop
python bridge.py
```

**Terminal 2**:
```cmd
cd laptop
python test_sender.py
```
Elige opción `1`, presiona teclas 0-4.

### Test 3: Jetson → Laptop (completo)

**Ver JETSON_SETUP.md para instrucciones detalladas de Jetson.**

Resumen:

**En Jetson**:
```bash
cd ~/jetson_cpp/build
cmake ..
make -j4
./dqn_agent
```

**En Laptop**:
```cmd
cd laptop
python bridge.py
```

Verás acciones del DQN. EV3 se mueve.

---

## Integrar tu DQN

**El DQN está en C++**. Ver `JETSON_SETUP.md` sección "Integrar tu Modelo DQN".

Opciones:
1. **PyTorch C++**: Exporta tu modelo a TorchScript (.pt)
2. **TensorRT**: Convierte a .trt (más rápido en Jetson)
3. **ONNX Runtime**: Balance entre velocidad y simplicidad

**Sin modelo**: El código usa acciones aleatorias (útil para testing).

---

## Calibrar

### Velocidades
Edita `config.py` líneas 31-36:
```python
SPEED_FORWARD = 40  # Más alto = más rápido
SPEED_TURN = 30     # Más alto = giros bruscos
```

### Corregir sentido
Si va al revés, edita `config.py` líneas 54-58:
```python
INVERT_FORWARD = True  # Invierte adelante/atrás
INVERT_TURNS = True    # Invierte izq/der
```

---

## Archivos

```
Cloud_Final/
├── config.py              # Configuración Python (laptop)
│
├── jetson_cpp/            # DQN en C++ (JETSON NANO)
│   ├── main.cpp           # Loop principal: Cámara → DQN → UDP
│   ├── dqn_agent.cpp      # Algoritmo DQN completo
│   ├── CMakeLists.txt     # Build con cmake
│   └── models/            # Tus modelos entrenados (.pt, .onnx)
│
├── laptop/                # Puente Python (LAPTOP WINDOWS)
│   ├── bridge.py          # Puente UDP→USB + safety
│   ├── ev3_controller.py  # Control motores EV3
│   └── test_sender.py     # Testing local sin Jetson
│
├── JETSON_SETUP.md        # Instrucciones Jetson (compilar, ejecutar)
├── README.md              # Este archivo
└── QUICKSTART.md          # 3 pasos rápidos
```

---

## Logs

Bridge genera `bridge_log_*.txt`:
```
[10:30:45.123] ← Recibido: 1 desde 192.168.1.50
[10:30:45.125] ✓ Acción 1 (FORWARD) ejecutada
[10:30:46.234] ⚠ TIMEOUT → STOP
```

---

## Motor C

Ignorado por ahora. Cuando A/D funcionen, decide si necesitas moverlo y actualiza:
1. `config.py` (añadir acciones 5-6)
2. `ev3_controller.py` (conectar OUTPUT_C)
3. `jetson_sender.py` (extender ACTIONS)

---

## Troubleshooting

**"ev3dev2 no encontrado"** → `pip install python-ev3dev2`

**"No conecta EV3"** → USB, enciende EV3, Device Manager

**"No recibe UDP"** → Firewall Windows, permite Python puerto 5000

**"Va al revés"** → `config.py` INVERT_FORWARD = True

---

**Todo listo. Integra tu DQN y prueba.**
