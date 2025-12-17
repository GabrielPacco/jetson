# Instrucciones de Ejecución - Jetson Nano

Guía para compilar y ejecutar el DQN en C++ en Jetson Nano.

---

## Prerequisitos

### 1. Instalar dependencias

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    pkg-config
```

### 2. Verificar OpenCV

```bash
pkg-config --modversion opencv4
# Debe mostrar versión (ej: 4.5.0)
```

Si no está instalado:
```bash
sudo apt-get install -y python3-opencv
```

---

## Compilación

### 1. Copiar archivos al Jetson

Desde tu PC, copia la carpeta `jetson_cpp/` al Jetson:

```bash
scp -r jetson_cpp/ usuario@jetson-ip:/home/usuario/
```

O usa USB/pendrive.

### 2. Compilar

En el Jetson:

```bash
cd ~/jetson_cpp
mkdir build
cd build
cmake ..
make -j4
```

Deberías ver:
```
[100%] Linking CXX executable dqn_agent
[100%] Built target dqn_agent
```

---

## Configuración

### 1. Editar IP de la laptop

Edita `main.cpp` línea 19:

```cpp
const std::string LAPTOP_IP = "192.168.1.100";  // TU IP AQUÍ
```

**Obtener IP de laptop**:
- Windows: `ipconfig` → busca "IPv4 Address"
- Linux: `ip addr show`

### 2. Verificar conexión de red

```bash
ping 192.168.1.100  # Reemplaza con tu IP
```

Debe responder. Si no:
- Conecta Jetson y laptop al mismo router
- Verifica firewall de Windows

### 3. Recompilar si editaste config

```bash
cd build
make -j4
```

---

## Ejecución

### Ejecutar DQN

```bash
cd ~/jetson_cpp/build
./dqn_agent
```

Verás:
```
========================================
  DQN Agent - Jetson → EV3
========================================
Laptop: 192.168.1.100:5000
Frecuencia: 5 Hz
Modelo: ./models/dqn_model.pt
Presiona Ctrl+C para detener

[DQN] Inicializado: 7056 estados, 5 acciones
[WARN] Archivo de modelo no encontrado, usando random
[UDP] Conectado a 192.168.1.100:5000
[CAMERA] Cámara 0 abierta
[SENT] 2 (TURN_LEFT) → 192.168.1.100:5000
[SENT] 1 (FORWARD) → 192.168.1.100:5000
...
```

### Detener

Presiona `Ctrl+C`:
```
[INFO] Señal de interrupción recibida, deteniendo...
[INFO] Enviando STOP final...
[SENT] 0 (STOP) → 192.168.1.100:5000
[INFO] Programa terminado
```

---

## Integrar tu Modelo DQN

### Opción 1: PyTorch C++

**1. Exportar modelo a TorchScript (en tu PC donde entrenas)**:

```python
import torch

# Tu modelo entrenado
model = YourDQN()
model.load_state_dict(torch.load('dqn_weights.pth'))
model.eval()

# Exportar a TorchScript
example = torch.rand(1, 84, 84)  # Ejemplo de entrada
traced = torch.jit.trace(model, example)
traced.save('dqn_model.pt')
```

**2. Copiar modelo al Jetson**:

```bash
mkdir -p ~/jetson_cpp/models
scp dqn_model.pt usuario@jetson-ip:~/jetson_cpp/models/
```

**3. Habilitar PyTorch en CMakeLists.txt**:

Descomentar líneas 16-17:
```cmake
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
```

Y línea 31:
```cmake
target_link_libraries(dqn_agent ${OpenCV_LIBS} ${TORCH_LIBRARIES})
```

**4. Modificar dqn_agent.cpp**:

```cpp
// En la clase DQNAgent, añadir miembro:
#include <torch/script.h>

private:
    torch::jit::script::Module model;

// En loadModel():
bool loadModel(const std::string& model_path) {
    try {
        model = torch::jit::load(model_path);
        model.eval();
        model_loaded = true;
        std::cout << "[DQN] Modelo cargado exitosamente" << std::endl;
        return true;
    } catch (const c10::Error& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return false;
    }
}

// En getBestAction():
int getBestAction(const std::vector<float>& state) {
    torch::Tensor input = torch::from_blob(
        (void*)state.data(),
        {1, 1, 84, 84},
        torch::kFloat32
    );

    torch::NoGradGuard no_grad;
    torch::Tensor output = model.forward({input}).toTensor();

    int action = output.argmax(1).item<int>();
    return action;
}
```

**5. Recompilar**:

```bash
cd build
cmake ..
make -j4
```

### Opción 2: TensorRT (más rápido)

**1. Convertir modelo a ONNX** (en tu PC):

```python
import torch
import torch.onnx

model = YourDQN()
model.load_state_dict(torch.load('dqn_weights.pth'))
model.eval()

dummy_input = torch.randn(1, 1, 84, 84)
torch.onnx.export(model, dummy_input, "dqn_model.onnx",
                  export_params=True,
                  opset_version=11,
                  input_names=['input'],
                  output_names=['output'])
```

**2. Convertir ONNX a TensorRT** (en Jetson):

```bash
/usr/src/tensorrt/bin/trtexec \
    --onnx=dqn_model.onnx \
    --saveEngine=dqn_model.trt \
    --fp16  # Usar FP16 para velocidad
```

**3. Modificar código** (más complejo, consultar docs de TensorRT).

### Opción 3: ONNX Runtime (balance)

Más simple que TensorRT, más rápido que PyTorch.

---

## Sin Modelo (Testing)

Si no tienes modelo entrenado, el código usa **acciones aleatorias**:

```
[WARN] Archivo de modelo no encontrado, usando random
```

Útil para probar la comunicación Jetson → Laptop → EV3.

---

## Verificar que Funciona

### 1. En Laptop (Windows)

Ejecutar bridge:
```cmd
cd laptop
python bridge.py
```

### 2. En Jetson

Ejecutar DQN:
```bash
cd ~/jetson_cpp/build
./dqn_agent
```

### 3. Confirmar

En laptop deberías ver:
```
← Recibido: 1 desde 192.168.1.50:XXXXX
✓ Acción 1 (FORWARD) ejecutada
```

Y el EV3 moviéndose.

---

## Troubleshooting

**"No se pudo abrir cámara 0"**
- Verifica que hay cámara conectada: `ls /dev/video*`
- Prueba con CAMERA_ID diferente (1, 2, etc.)
- O comenta el código de cámara (usará estado dummy)

**"No se pudo crear socket UDP"**
- Verifica permisos: `sudo ./dqn_agent`
- Verifica IP con `ifconfig`

**"Error enviando"**
- Verifica que laptop corre `bridge.py` primero
- Prueba ping: `ping 192.168.1.100`
- Revisa firewall de Windows

**"OpenCV not found"**
```bash
sudo apt-get install libopencv-dev
```

**Compilación lenta**
- Normal en Jetson (4 cores)
- Usa `-j4` en make

**Modelo no carga**
- Verifica ruta: `ls ~/jetson_cpp/models/`
- Verifica formato (debe ser `.pt` para PyTorch)

---

## Optimización

### Reducir latencia

Edita `main.cpp` línea 21:
```cpp
const int ACTION_FREQUENCY = 10;  // Aumentar a 10 Hz
```

### Reducir uso de CPU

Edita `CMakeLists.txt` línea 7:
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
```

### Usar GPU (si tienes modelo con CUDA)

Añadir en `main.cpp`:
```cpp
torch::DeviceType device = torch::kCUDA;
model.to(device);
input = input.to(device);
```

---

## Logs

Para guardar logs:

```bash
./dqn_agent 2>&1 | tee dqn_log.txt
```

Ver logs en tiempo real:
```bash
tail -f dqn_log.txt
```

---

## Próximos Pasos

1. **Test sin modelo**: Verificar comunicación con acciones aleatorias
2. **Integrar cámara**: Confirmar que captura frames
3. **Cargar modelo**: Seguir Opción 1, 2 o 3 según tu framework
4. **Calibrar**: Ajustar epsilon, frecuencia según comportamiento

---

**El DQN está listo para recibir tu modelo entrenado.**
