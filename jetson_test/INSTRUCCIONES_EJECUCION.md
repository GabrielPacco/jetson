# Instrucciones de Ejecución - DQN Lego Robot en Jetson AGX Xavier

Guía paso a paso para configurar, compilar y ejecutar el proyecto DQN en la Jetson AGX Xavier.

---

## 📋 Pre-requisitos

Antes de comenzar, asegúrate de tener:

- ✅ NVIDIA Jetson AGX Xavier Development Kit con JetPack 4.x+
- ✅ Robot Lego con Bluetooth, giróscopo y sensores de contacto
- ✅ Conexión a internet en la Jetson (para descargar dependencias)
- ✅ Cable USB o conexión SSH a la Jetson
- ✅ Robot Lego encendido y con batería cargada

---

## 🚀 Paso 1: Transferir el Proyecto a la Jetson

### Opción A: Desde GitHub (Recomendado)

```bash
# En la Jetson AGX Xavier
cd ~
git clone https://github.com/GabrielPacco/jetson_test.git
cd jetson_test/Yetson
```

### Opción B: Transferir archivos manualmente

```bash
# Desde tu PC Windows (usando SCP o FileZilla)
# Copiar la carpeta completa "Yetson" a la Jetson

# En la Jetson, navegar al directorio
cd ~/jetson_test/Yetson  # o donde hayas copiado el proyecto
```

---

## 🔧 Paso 2: Configurar el Entorno en la Jetson

### 2.1 Ejecutar Script de Instalación

```bash
# Dar permisos de ejecución al script
chmod +x scripts/setup_jetson.sh

# Ejecutar el script (tomará 10-15 minutos)
./scripts/setup_jetson.sh
```

**¿Qué instala este script?**
- Herramientas de desarrollo (gcc, g++, cmake, git)
- Bibliotecas Bluetooth (bluez, libbluetooth-dev)
- Parser YAML (libyaml-cpp-dev)
- LibTorch 2.0+ para ARM64 con CUDA 11.x

### 2.2 Configurar Permisos y Variables de Entorno

```bash
# Cerrar sesión para aplicar permisos de Bluetooth
logout

# Volver a conectarse a la Jetson y ejecutar:
source ~/.bashrc
```

**Verificar instalación de CUDA:**
```bash
nvcc --version
# Deberías ver: Cuda compilation tools, release 11.x

python3 -c "import torch; print(torch.cuda.is_available())"
# Debería mostrar: True (si PyTorch está instalado, opcional)
```

---

## 📱 Paso 3: Configurar el Robot Lego

### 3.1 Obtener la Dirección MAC del Robot

```bash
# Escanear dispositivos Bluetooth cercanos
hcitool scan
```

**Salida esperada:**
```
Scanning ...
    00:1A:7D:DA:71:13    LEGO Robot
```

**Copia la dirección MAC** (XX:XX:XX:XX:XX:XX)

### 3.2 Configurar el Proyecto

```bash
# Editar archivo de configuración
nano configs/hyperparameters.yaml

# O usar otro editor:
# vim configs/hyperparameters.yaml
# gedit configs/hyperparameters.yaml
```

**Modificar la línea:**
```yaml
robot:
  bluetooth_address: "00:1A:7D:DA:71:13"  # Reemplazar con tu MAC
```

**Guardar y cerrar:**
- En nano: `Ctrl+O` (guardar), `Enter`, `Ctrl+X` (salir)
- En vim: `Esc`, `:wq`, `Enter`

---

## 🔨 Paso 4: Compilar el Proyecto

```bash
# Dar permisos al script de compilación
chmod +x scripts/build.sh

# Compilar (tomará 5-10 minutos)
./scripts/build.sh
```

**Salida esperada al final:**
```
========================================================================
  Build Complete!
========================================================================

Executables in /home/nvidia/jetson_test/Yetson/build:
  - train              : Training application
  - inference          : Inference application
  - test_bluetooth     : Bluetooth testing utility
```

**Si hay errores:**
- Verificar que LibTorch está en `/usr/local/libtorch`
- Ejecutar: `ls /usr/local/libtorch` (debe mostrar directorios bin, lib, include)
- Si no existe, volver al Paso 2.1

---

## 🧪 Paso 5: Probar Conexión Bluetooth

**IMPORTANTE:** Siempre probar la conexión ANTES de entrenar.

```bash
cd build

# Reemplazar XX:XX:XX:XX:XX:XX con tu dirección MAC
./test_bluetooth XX:XX:XX:XX:XX:XX
```

### Resultados Esperados

**✅ Conexión exitosa:**
```
[Test 1] Testing connection...
[PASS] Connected successfully

[Test 2] Testing sensor reading...
[PASS] Sensor data received:
  Gyroscope: x=0.123, y=-0.056, z=0.001
  Contact sensors: front=0, side=0
  Timestamp: 12345 ms

[Test] Sending command: forward...
[PASS] Command sent successfully

...

Tests passed: 6 / 6
[SUCCESS] All tests passed!

Enter interactive mode? (y/n):
```

**Si todos los tests pasan:**
- Presionar `n` (no entrar a modo interactivo por ahora)
- Continuar al Paso 6

**❌ Si falla la conexión:**

```bash
# Verificar que Bluetooth está habilitado
sudo systemctl status bluetooth

# Si no está activo:
sudo systemctl start bluetooth

# Emparejar manualmente (opcional):
bluetoothctl
> power on
> scan on
> pair XX:XX:XX:XX:XX:XX
> trust XX:XX:XX:XX:XX:XX
> quit

# Volver a intentar test_bluetooth
```

**❌ Si falla lectura de sensores:**
- Verificar que el robot está encendido
- Verificar batería del robot
- Verificar que el robot está enviando datos (firmware correcto)

---

## 🎓 Paso 6: Entrenar el Modelo DQN

### 6.1 Preparación

```bash
# Asegurarse de estar en el directorio build
cd ~/jetson_test/Yetson/build

# Verificar que el robot está listo:
# - Encendido
# - Batería cargada (mínimo 80% para 500 episodios)
# - En un espacio abierto con obstáculos
```

### 6.2 Iniciar Entrenamiento

```bash
# Iniciar entrenamiento (usa config por defecto)
./train
```

**O con configuración personalizada:**
```bash
./train ../configs/hyperparameters.yaml
```

### 6.3 Monitorear Progreso

**Opción 1: En la misma terminal**
El programa mostrará progreso cada 10 episodios:
```
[Episode   10] Reward:   45.50 | Epsilon: 0.900 | Loss: 0.0234
  Mean reward (100 eps): 42.30

[Episode   20] Reward:   78.20 | Epsilon: 0.810 | Loss: 0.0189
  Mean reward (100 eps): 56.75
```

**Opción 2: Monitorear el archivo de log (en otra terminal SSH):**
```bash
# Conectarse a la Jetson en otra sesión SSH
ssh nvidia@<IP_JETSON>

# Monitorear log en tiempo real
tail -f ~/jetson_test/Yetson/training.log
```

**Opción 3: Monitorear uso de GPU:**
```bash
# En otra terminal
watch -n 1 nvidia-smi
```

### 6.4 Duración y Expectativas

- **Tiempo estimado**: 2-4 horas para 500 episodios
- **Uso de GPU**: ~1-2 GB de memoria
- **Uso de CPU**: 30-50%
- **Checkpoints automáticos**: Cada 50 episodios en `models/dqn_checkpoint_N.pt`

### 6.5 Detener el Entrenamiento (si es necesario)

```bash
# Presionar Ctrl+C para detener de forma segura
# El modelo se guardará automáticamente antes de salir
```

---

## 🏁 Paso 7: Revisar Resultados del Entrenamiento

```bash
# Ver archivos generados
ls -lh models/

# Deberías ver:
# dqn_best.pt           - Mejor modelo durante entrenamiento
# dqn_final.pt          - Modelo al finalizar
# dqn_checkpoint_50.pt  - Checkpoints periódicos
# dqn_checkpoint_100.pt
# ...
```

**Ver métricas de entrenamiento:**
```bash
# Ver últimas 50 líneas del log
tail -n 50 training.log

# Ver archivo de métricas CSV
cat training_metrics.csv

# O transferir a PC para graficar:
scp nvidia@<IP_JETSON>:~/jetson_test/Yetson/training_metrics.csv .
```

---

## 🤖 Paso 8: Ejecutar el Robot Autónomamente (Inferencia)

### 8.1 Preparación

```bash
# Asegurarse de que el robot está:
# - Encendido
# - Con Bluetooth activo
# - En un espacio abierto
# - Con batería suficiente (mínimo 50%)

cd ~/jetson_test/Yetson/build
```

### 8.2 Ejecutar Modelo Entrenado

```bash
# Usar el mejor modelo
./inference models/dqn_best.pt
```

**O especificar dirección MAC:**
```bash
./inference models/dqn_best.pt XX:XX:XX:XX:XX:XX
```

### 8.3 Observar el Comportamiento

**Salida esperada:**
```
[Episode 1] Starting new episode...
  [Step 1] Action: forward | Reward: 1.00
  [Step 2] Action: forward | Reward: 1.00
  [Step 3] Action: left | Reward: 0.00
  [Step 4] Action: forward | Reward: 1.00
  ...

[Episode 1] Episode ended
  Total reward: 195.00
  Total steps: 195
  Info: step=195, action=forward, collision=false
```

### 8.4 Detener la Ejecución

```bash
# Presionar Ctrl+C para detener de forma segura
# El robot se detendrá automáticamente
```

---

## 📊 Paso 9: Evaluar el Desempeño

### Métricas de Éxito

**✅ Entrenamiento Exitoso:**
- Recompensa promedio (100 eps) > 150
- Tasa de colisiones < 10%
- Epsilon decae a 0.05
- Pérdida converge a valores bajos (~0.001-0.01)

**✅ Inferencia Exitosa:**
- Robot navega sin colisiones por 1+ minutos
- Evita obstáculos consistentemente
- Recompensa promedio por episodio > 150

### Si el Desempeño No es Bueno

**Problema: Robot colisiona frecuentemente**
```yaml
# Aumentar penalización por colisión
reward:
  collision_penalty: -2.0  # Era -1.0
```

**Problema: Robot solo gira, no avanza**
```yaml
# Aumentar recompensa por avanzar
reward:
  forward_success: 2.0  # Era 1.0
  turn_reward: -0.1     # Penalizar giros excesivos
```

**Problema: Entrenamiento no converge**
```yaml
training:
  num_episodes: 1000        # Más episodios
  learning_rate: 0.0005     # LR más bajo
  epsilon_decay: 0.997      # Decay más lento
```

**Re-entrenar con nueva configuración:**
```bash
nano ../configs/hyperparameters.yaml  # Editar
./train  # Re-entrenar
```

---

## 🔍 Solución de Problemas Comunes

### Error: "Bluetooth library not found"

```bash
sudo apt update
sudo apt install libbluetooth-dev
cd ~/Yetson/build
cmake .. && make
```

### Error: "Permission denied" al conectar Bluetooth

```bash
# Verificar grupo bluetooth
groups | grep bluetooth

# Si no está, añadir:
sudo usermod -a -G bluetooth $USER
logout
# Volver a conectar
```

### Error: "Could not connect to robot"

```bash
# 1. Verificar que el robot está encendido
# 2. Verificar Bluetooth del robot está activo
# 3. Escanear nuevamente:
hcitool scan

# 4. Reiniciar Bluetooth de la Jetson:
sudo systemctl restart bluetooth

# 5. Intentar nuevamente
cd ~/Yetson/build
./test_bluetooth XX:XX:XX:XX:XX:XX
```

### Error: "LibTorch not found"

```bash
# Verificar instalación
ls /usr/local/libtorch

# Si no existe, reinstalar:
cd /tmp
wget https://download.pytorch.org/libtorch/cu117/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu117.zip
unzip libtorch*.zip
sudo mv libtorch /usr/local/

# Recompilar
cd ~/jetson_test/Yetson/build
rm -rf *
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Error: "CUDA out of memory"

```yaml
# Reducir batch size en config
replay:
  batch_size: 32  # Era 64

# Reducir tamaño de red
network:
  hidden_dim1: 64  # Era 128
  hidden_dim2: 64  # Era 128
```

### Robot se detiene durante entrenamiento

```bash
# Verificar batería
# Verificar conexión Bluetooth (puede haberse perdido)

# Reiniciar entrenamiento desde checkpoint:
# El entrenamiento guardó checkpoints automáticamente
# Cargar último checkpoint y continuar
```

---

## 📚 Comandos de Referencia Rápida

```bash
# ============== CONFIGURACIÓN INICIAL ==============
hcitool scan                                    # Escanear Bluetooth
nano configs/hyperparameters.yaml              # Editar config
./scripts/setup_jetson.sh                      # Instalar dependencias
./scripts/build.sh                             # Compilar

# ============== TESTING ==============
cd build
./test_bluetooth XX:XX:XX:XX:XX:XX             # Test Bluetooth

# ============== ENTRENAMIENTO ==============
./train                                         # Entrenar (config por defecto)
./train ../configs/custom.yaml                  # Entrenar (config custom)
tail -f ../training.log                         # Monitorear log
watch -n 1 nvidia-smi                          # Monitorear GPU

# ============== INFERENCIA ==============
./inference models/dqn_best.pt                 # Ejecutar mejor modelo
./inference models/dqn_final.pt                # Ejecutar modelo final
./inference models/dqn_checkpoint_500.pt       # Ejecutar checkpoint

# ============== ARCHIVOS GENERADOS ==============
ls -lh models/                                  # Ver modelos guardados
cat training.log                                # Ver log completo
cat training_metrics.csv                        # Ver métricas CSV
```

---

## ✅ Checklist de Ejecución Completa

**Antes de entrenar:**
- [ ] Jetson AGX Xavier configurada (Paso 1-2)
- [ ] Proyecto compilado sin errores (Paso 4)
- [ ] Dirección MAC del robot configurada (Paso 3)
- [ ] Test Bluetooth exitoso (6/6 tests) (Paso 5)
- [ ] Robot con batería >80%
- [ ] Espacio abierto preparado con obstáculos

**Durante el entrenamiento:**
- [ ] Monitorear primeros 50 episodios
- [ ] Verificar que pérdida disminuye
- [ ] Verificar que robot explora (epsilon alto al inicio)
- [ ] Checkpoints guardándose cada 50 episodios

**Después del entrenamiento:**
- [ ] Revisar training.log
- [ ] Verificar dqn_best.pt existe
- [ ] Probar inferencia con modelo entrenado
- [ ] Evaluar comportamiento del robot

**Si todo funciona:**
- [ ] Robot navega autónomamente
- [ ] Evita obstáculos
- [ ] Recompensa promedio >150

---

## 🎯 Siguientes Pasos (Opcional)

1. **Ajustar hiperparámetros** para mejorar desempeño
2. **Entrenar por más episodios** (1000-2000)
3. **Probar en diferentes entornos** (obstáculos variados)
4. **Implementar Double DQN** para mejorar estabilidad
5. **Graficar métricas** usando Python/Matplotlib
6. **Exportar para presentación** (video, gráficas, informe)

---

## 📞 Soporte

Si encuentras problemas no cubiertos aquí:

1. Revisar `README.md` completo
2. Verificar logs en `training.log`
3. Consultar documentación de [LibTorch](https://pytorch.org/cppdocs/)
4. Consultar documentación de [BlueZ](http://www.bluez.org/)

---

**¡Buena suerte con tu proyecto de robótica!** 🚀🤖
