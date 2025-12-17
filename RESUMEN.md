# Resumen del Sistema

Sistema completo de control robótico con DQN.

---

## Componentes

### 1. Jetson Nano (C++)
- **DQN en C++**: Algoritmo completo con epsilon-greedy
- **Captura de cámara**: OpenCV
- **Envío UDP**: Socket nativo
- **Frecuencia**: 5 Hz (configurable)

**Archivos**:
- `jetson_cpp/main.cpp` - Loop principal
- `jetson_cpp/dqn_agent.cpp` - Algoritmo DQN
- `jetson_cpp/CMakeLists.txt` - Build system

### 2. Laptop Windows (Python)
- **Bridge UDP→USB**: Recibe acciones, envía al EV3
- **Control EV3**: Motores A y D con python-ev3dev2
- **Safety**: Timeout automático (0.5s sin recibir → STOP)
- **Logs**: Timestamp de todas las acciones

**Archivos**:
- `laptop/bridge.py` - Puente principal
- `laptop/ev3_controller.py` - Control motores
- `laptop/test_sender.py` - Testing sin Jetson

### 3. EV3 Robot
- **Motor A**: Rueda izquierda
- **Motor D**: Rueda derecha
- **Motor C**: No usado (reservado)

---

## Flujo de Datos

```
1. Cámara (Jetson) → captura frame
2. DQN (C++) → procesa imagen → selecciona acción (0-4)
3. UDP (Jetson → Laptop) → envía acción
4. Bridge (Laptop) → valida y relay
5. EV3 (USB) → ejecuta movimiento
```

**Latencia total**: <100ms (depende de red)

---

## Acciones

| ID | Nombre | Motor A | Motor D |
|----|--------|---------|---------|
| 0 | STOP | 0% | 0% |
| 1 | FORWARD | +40% | +40% |
| 2 | TURN_LEFT | -30% | +30% |
| 3 | TURN_RIGHT | +30% | -30% |
| 4 | BACKWARD | -40% | -40% |

Velocidades configurables en `config.py`.

---

## Instalación Rápida

### Jetson
```bash
cd jetson_cpp
mkdir build && cd build
cmake .. && make -j4
```

### Laptop
```cmd
pip install python-ev3dev2
```

---

## Ejecución

### 1. Laptop (primero)
```cmd
cd laptop
python bridge.py
```

### 2. Jetson
```bash
cd jetson_cpp/build
./dqn_agent
```

---

## Integrar tu Modelo

Tienes 3 opciones según tu framework:

### Opción A: PyTorch
1. Exporta a TorchScript: `model.save('model.pt')`
2. Copia a Jetson: `models/dqn_model.pt`
3. Modifica `dqn_agent.cpp` para cargar PyTorch
4. Recompila con Torch

Ver `JETSON_SETUP.md` para código completo.

### Opción B: TensorRT (más rápido)
1. Convierte a ONNX
2. Convierte ONNX a TensorRT (.trt)
3. Usa TensorRT API en C++

### Opción C: ONNX Runtime
Balance entre A y B.

---

## Testing sin Modelo

El código funciona **sin modelo entrenado**:
- Usa acciones **aleatorias** con epsilon-greedy
- Útil para probar comunicación y motores
- Decae epsilon (reduce aleatoriedad con el tiempo)

---

## Configuración

Todo centralizado:

**Jetson** (`main.cpp` líneas 19-23):
```cpp
const std::string LAPTOP_IP = "192.168.1.100";
const int UDP_PORT = 5000;
const int ACTION_FREQUENCY = 5;
```

**Laptop** (`config.py` líneas 10, 31-36):
```python
LAPTOP_IP = "192.168.1.100"
SPEED_FORWARD = 40
SPEED_TURN = 30
```

---

## Logs

Bridge genera `bridge_log_TIMESTAMP.txt`:
```
[10:30:45.123] ← Recibido: 1 desde 192.168.1.50:12345
[10:30:45.125] ✓ Acción 1 (FORWARD) ejecutada
[10:30:50.234] ⚠ TIMEOUT (0.52s) → STOP
```

---

## Dependencias

### Jetson
- Ubuntu 18.04/20.04
- OpenCV 4.x
- CMake 3.10+
- g++ 7.x+
- (Opcional) PyTorch C++ / TensorRT

### Laptop
- Windows 10/11
- Python 3.8+
- python-ev3dev2

### EV3
- Firmware LEGO original (sin microSD)
- Cables en puertos A y D

---

## Documentación

- **README.md**: Guía general
- **JETSON_SETUP.md**: Instrucciones completas Jetson (compilar, integrar modelo)
- **QUICKSTART.md**: 3 pasos rápidos para testing
- **Este archivo**: Resumen ejecutivo

---

## Próximos Pasos

1. **Test comunicación**: Laptop + test_sender.py
2. **Test Jetson sin modelo**: Acciones aleatorias
3. **Integrar modelo**: PyTorch/TensorRT
4. **Calibrar velocidades**: Ajustar SPEED_FORWARD/TURN
5. **Training**: Recolectar datos, entrenar, mejorar

---

**Sistema completo y funcional. Listo para tu DQN.**
