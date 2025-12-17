# 🔍 Verificación de Compilación - Jetson AGX Xavier

Este documento contiene los pasos de verificación para asegurar que el código compile correctamente en la Jetson AGX Xavier.

---

## ✅ Checklist Pre-Compilación

### 1. Obtener Últimos Cambios

```bash
cd ~/Documents/jetson_test/Yetson
git pull origin main
```

**Verificar que se obtuvieron estos commits recientes:**
- Fix: Corregir errores de compilación LibTorch en ARM64 (commit c3adb43)
- Correcciones aplicadas:
  - `#include <string>` añadido a `protocol.h`
  - Inicialización de `torch::nn::Linear` corregida en `network.h/cpp`
  - `buffer_.reserve()` eliminado de `replay_buffer.cpp`

---

### 2. Verificar Dependencias

```bash
# LibTorch
ls -lh /usr/local/libtorch/
# Debe mostrar directorios: include/, lib/, share/

# Bluetooth
dpkg -l | grep libbluetooth-dev
# Debe mostrar: ii  libbluetooth-dev

# YAML-CPP
dpkg -l | grep yaml-cpp
# Debe mostrar: ii  libyaml-cpp-dev

# CUDA
nvcc --version
# Debe mostrar: Cuda compilation tools, release 11.x
```

**Si falta alguna dependencia:**
```bash
cd ~/Documents/jetson_test/Yetson
./scripts/setup_jetson.sh
```

---

### 3. Limpiar Build Anterior (IMPORTANTE)

```bash
cd ~/Documents/jetson_test/Yetson
rm -rf build/
mkdir build
```

**¿Por qué limpiar?** Los cambios en headers requieren recompilación completa para evitar errores de caché de CMake.

---

## 🛠️ Proceso de Compilación

### Paso 1: Configurar CMake

```bash
cd ~/Documents/jetson_test/Yetson/build

cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      ..
```

**Salida esperada:**
```
-- Found LibTorch: /usr/local/libtorch/lib/libtorch.so;...
-- Found Bluetooth library: /usr/lib/aarch64-linux-gnu/libbluetooth.so
-- Found yaml-cpp: yaml-cpp
========================================
DQN Lego Robot Configuration Summary
========================================
  Build type: Release
  C++ compiler: /usr/bin/c++
  C++ standard: 17
  ...
========================================
-- Configuring done
-- Generating done
```

**❌ Si falla aquí:**
- Verificar que `/usr/local/libtorch` existe
- Verificar que `libbluetooth-dev` está instalado
- Verificar que `libyaml-cpp-dev` está instalado

---

### Paso 2: Compilar train_simulation (SIN Bluetooth)

Primero compilar solo el ejecutable de simulación, que NO requiere hardware:

```bash
make train_simulation -j$(nproc)
```

**Salida esperada:**
```
Scanning dependencies of target dqn_lib
[  4%] Building CXX object CMakeFiles/dqn_lib.dir/src/dqn/network.cpp.o
[  8%] Building CXX object CMakeFiles/dqn_lib.dir/src/dqn/replay_buffer.cpp.o
[ 12%] Building CXX object CMakeFiles/dqn_lib.dir/src/dqn/agent.cpp.o
...
[ 96%] Building CXX object CMakeFiles/train_simulation.dir/apps/train_simulation.cpp.o
[100%] Linking CXX executable train_simulation
[100%] Built target train_simulation
```

**⚠️ Errores Comunes y Soluciones:**

| Error | Causa | Solución |
|-------|-------|----------|
| `'string' in namespace 'std' does not name a type` | Falta `#include <string>` | Ya corregido en commit c3adb43, hacer `git pull` |
| `static assertion failed: You are trying to default construct a module` | Inicialización incorrecta de `torch::nn::Linear` | Ya corregido en commit c3adb43, hacer `git pull` |
| `'class std::deque' has no member named 'reserve'` | `deque` no tiene `reserve()` | Ya corregido en commit c3adb43, hacer `git pull` |
| `undefined reference to 'torch::...'` | LibTorch no encontrado | Verificar CMAKE_PREFIX_PATH |
| `cannot find -lbluetooth` | Bluetooth library no encontrada | `sudo apt install libbluetooth-dev` |

---

### Paso 3: Probar train_simulation

```bash
./train_simulation 10
```

**Salida esperada:**
```
=========================================================================
  DQN Training - SIMULATION MODE (CartPole)
  Sin robot físico - Solo prueba de algoritmo
=========================================================================
[Device] cuda:0
[Environment] Creando entorno CartPole simulado...
[Agent] Creando DQN agent...
[DQNAgent] Initialized with:
  State dim: 4
  Action dim: 2
  Hidden dims: [128, 128]
  Device: cuda:0
  Learning rate: 0.001
  Gamma: 0.99
  Epsilon: 1 -> 0.01

[Training] Iniciando entrenamiento simulado...
  Episodios: 10
  Objetivo: Recompensa promedio >= 195
=========================================================================
Episode 1 | Reward: 15.00 | Epsilon: 0.9950 | Loss: 0.0023
...
```

**✅ Si esto funciona, DQN está compilado correctamente.**

---

### Paso 4: Compilar Todos los Ejecutables

```bash
cd ~/Documents/jetson_test/Yetson/build
make -j$(nproc)
```

Esto compila:
- `train` (requiere robot Lego)
- `train_simulation` (ya compilado)
- `inference` (requiere robot Lego)
- `test_bluetooth` (requiere robot Lego)
- Tests unitarios

**Salida esperada:**
```
[100%] Built target dqn_lib
[100%] Built target train
[100%] Built target train_simulation
[100%] Built target inference
[100%] Built target test_bluetooth
[100%] Built target test_network
[100%] Built target test_replay_buffer
[100%] Built target test_environment
```

---

## 🧪 Tests Unitarios (Opcional)

```bash
cd ~/Documents/jetson_test/Yetson/build

# Test de red neuronal
./test_network
# Esperado: [PASS] QNetwork forward pass test
#           [PASS] QNetwork device transfer test

# Test de replay buffer
./test_replay_buffer
# Esperado: [PASS] ReplayBuffer basic operations test
#           [PASS] ReplayBuffer sampling test

# Test de protocolo
./test_environment
# Esperado: [PASS] CommandPacket test
#           [PASS] SensorPacket test
```

---

## 📊 Verificación Final

### Archivos Generados

```bash
ls -lh ~/Documents/jetson_test/Yetson/build/

# Debe mostrar:
# -rwxr-xr-x train
# -rwxr-xr-x train_simulation
# -rwxr-xr-x inference
# -rwxr-xr-x test_bluetooth
# -rwxr-xr-x test_network
# -rwxr-xr-x test_replay_buffer
# -rwxr-xr-x test_environment
# -rw-r--r-- libdqn_lib.a
```

### Tamaño Aproximado

```bash
du -sh ~/Documents/jetson_test/Yetson/build/

# Esperado: ~200-300 MB
```

---

## 🎯 Próximos Pasos

### Si la Compilación fue Exitosa

**Opción 1: Entrenar en Simulación (SIN robot)**
```bash
cd ~/Documents/jetson_test/Yetson/build
./train_simulation 500  # 500 episodios
```
Tiempo estimado: 10-20 minutos
Objetivo: Recompensa promedio >= 195

**Opción 2: Probar con Robot Lego (CON robot físico)**
```bash
# 1. Configurar MAC del robot
nano ~/Documents/jetson_test/Yetson/configs/hyperparameters.yaml
# Cambiar: bluetooth_address: "XX:XX:XX:XX:XX:XX"

# 2. Test Bluetooth
cd ~/Documents/jetson_test/Yetson/build
./test_bluetooth XX:XX:XX:XX:XX:XX

# 3. Entrenar con robot
./train
```

---

## 🐛 Debugging

### Ver Logs de Compilación Completos

```bash
cd ~/Documents/jetson_test/Yetson/build
cmake .. 2>&1 | tee cmake.log
make VERBOSE=1 -j$(nproc) 2>&1 | tee make.log
```

Los archivos `cmake.log` y `make.log` contendrán toda la salida para análisis.

### Verificar Versiones

```bash
# CMake
cmake --version
# Esperado: >= 3.18

# GCC
g++ --version
# Esperado: >= 7.5

# CUDA
nvcc --version
# Esperado: 11.x

# LibTorch
ls /usr/local/libtorch/build-version
cat /usr/local/libtorch/build-version
# Esperado: 2.0.0 o superior
```

---

## 📞 Solución de Problemas

### Error: "LibTorch not found"

```bash
# Verificar que existe
ls /usr/local/libtorch/

# Si no existe, descargar manualmente
cd /tmp
wget https://download.pytorch.org/libtorch/cu118/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu118.zip
unzip libtorch-cxx11-abi-shared-with-deps-2.0.0+cu118.zip
sudo mv libtorch /usr/local/
```

### Error: "CUDA out of memory" durante entrenamiento

```bash
# Verificar memoria GPU disponible
nvidia-smi

# Reducir batch_size en configs/hyperparameters.yaml
nano ~/Documents/jetson_test/Yetson/configs/hyperparameters.yaml
# Cambiar: batch_size: 32  (en vez de 64)
```

### Error: "Permission denied" al usar Bluetooth

```bash
# Añadir usuario al grupo bluetooth
sudo usermod -a -G bluetooth $USER
sudo systemctl restart bluetooth

# Cerrar sesión y volver a entrar
logout
```

---

## ✅ Checklist Final

Antes de proceder al entrenamiento, verificar:

- [ ] Git pull exitoso (commit c3adb43 o posterior)
- [ ] Todas las dependencias instaladas
- [ ] Build limpio (rm -rf build/)
- [ ] CMake configura sin errores
- [ ] train_simulation compila exitosamente
- [ ] train_simulation ejecuta sin errores (al menos 10 episodios)
- [ ] Todos los ejecutables compilados (make -j)
- [ ] Tests unitarios pasan (opcional)
- [ ] GPU disponible (nvidia-smi muestra memoria libre)

---

## 📝 Registro de Cambios

| Fecha | Cambio | Commit |
|-------|--------|--------|
| 2024-xx-xx | Fix compilación LibTorch ARM64 | c3adb43 |
| 2024-xx-xx | Añadir modo simulación | - |
| 2024-xx-xx | Implementación inicial | - |

---

**¡Éxito!** Si todos los pasos anteriores funcionaron, el sistema está listo para entrenar. 🚀
