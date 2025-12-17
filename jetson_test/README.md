# DQN Lego Robot - Navegación Autónoma con Evasión de Obstáculos

Sistema de Deep Q-Network (DQN) implementado en C++ con LibTorch para entrenar un robot Lego a navegar autónomamente evitando obstáculos. Diseñado para ejecutarse en NVIDIA Jetson AGX Xavier Development Kit.

## Características

- ✅ **Algoritmo DQN completo** con replay buffer y target network
- ✅ **Aceleración CUDA** para entrenamiento eficiente en Jetson
- ✅ **Comunicación Bluetooth RFCOMM** con el robot Lego
- ✅ **Entrenamiento en hardware real** (no simulación)
- ✅ **Interfaz modular** siguiendo el patrón OpenAI Gym
- ✅ **Logging y métricas** de entrenamiento
- ✅ **Checkpoint automático** de modelos

## Especificaciones del Sistema

### Robot Lego
- **Sensores**: Giróscopo/acelerómetro + sensores de contacto (frontal y lateral)
- **Espacio de estados**: Vector 4D [orientation_x, orientation_y, contact_front, contact_side]
- **Espacio de acciones**: 4 acciones discretas {adelante, atrás, izquierda, derecha}
- **Comunicación**: Bluetooth RFCOMM (BlueZ)
- **Tarea**: Navegación con evasión de obstáculos

### Algoritmo DQN
- **Red neuronal**: 4 → 128 → 128 → 4 (fully connected)
- **Función de activación**: ReLU
- **Optimizador**: Adam (lr=0.001)
- **Replay buffer**: 10,000 transiciones
- **Batch size**: 64
- **Target network**: Actualización cada 10 episodios
- **Exploración**: ε-greedy con decay exponencial (1.0 → 0.05)

## Requisitos del Sistema

### Hardware
- NVIDIA Jetson AGX Xavier Development Kit
- Robot Lego con:
  - Giróscopo/acelerómetro
  - Sensores de contacto (frontal y lateral)
  - Módulo Bluetooth
  - Baterías/alimentación suficiente para entrenamiento prolongado

### Software
- JetPack 4.x o superior (Ubuntu 18.04/20.04 ARM64)
- CUDA 11.x
- LibTorch 2.0+ (C++ API de PyTorch para ARM64)
- BlueZ (Bluetooth stack de Linux)
- libyaml-cpp-dev (parser YAML)
- CMake 3.18+
- GCC/G++ con soporte C++17

## Instalación

### 1. Clonar el Repositorio

```bash
cd ~
git clone https://github.com/GabrielPacco/jetson_test.git
cd jetson_test/Yetson
```

### 2. Configurar la Jetson AGX Xavier

Ejecutar el script de instalación automática:

```bash
chmod +x scripts/setup_jetson.sh
./scripts/setup_jetson.sh
```

Este script instalará:
- Herramientas de desarrollo (build-essential, cmake, git)
- Bibliotecas Bluetooth (bluez, libbluetooth-dev)
- Parser YAML (libyaml-cpp-dev)
- LibTorch 2.0+ para ARM64 con CUDA 11.x

**Nota**: Después de la instalación, debes:
1. Cerrar sesión y volver a entrar (para permisos de bluetooth)
2. Ejecutar `source ~/.bashrc` para cargar variables de entorno

### 3. Configurar el Robot

1. **Obtener la dirección MAC del robot**:
```bash
hcitool scan
```

2. **Editar la configuración**:
Abrir `configs/hyperparameters.yaml` y actualizar:
```yaml
robot:
  bluetooth_address: "XX:XX:XX:XX:XX:XX"  # Tu dirección MAC
```

## Compilación

### Opción 1: Script Automático

```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

### Opción 2: Manual

```bash
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc)
ctest --output-on-failure
```

Los ejecutables se generarán en `build/`:
- `train` - Aplicación de entrenamiento
- `inference` - Aplicación de inferencia
- `test_bluetooth` - Utilidad de prueba Bluetooth

## Uso

### 🎮 Opción A: Solo Probar DQN (Sin Robot - Simulación)

Si quieres probar el algoritmo DQN **sin el robot físico**, usa el modo simulación:

```bash
cd build
./train_simulation
```

**Ver guía completa:** [`SOLO_DQN.md`](SOLO_DQN.md)

**Ventajas:**
- ✅ No requiere Bluetooth ni robot
- ✅ Muy rápido (~10 minutos para 500 episodios)
- ✅ Valida que el algoritmo DQN funciona
- ✅ Prueba instalación de LibTorch y CUDA

---

### 🤖 Opción B: Entrenamiento con Robot Lego Real

### 1. Probar Conexión Bluetooth

Antes de entrenar, verifica que la comunicación Bluetooth funciona:

```bash
cd build
./test_bluetooth XX:XX:XX:XX:XX:XX
```

Esto ejecutará:
- Test de conexión
- Test de lectura de sensores
- Test de envío de comandos (adelante, atrás, izquierda, derecha)
- Modo interactivo (opcional)

**Salida esperada**:
```
[Test 1] Testing connection...
[PASS] Connected successfully

[Test 2] Testing sensor reading...
[PASS] Sensor data received:
  Gyroscope: x=0.123, y=-0.056, z=0.001
  Contact sensors: front=0, side=0
  Timestamp: 12345 ms

...

Tests passed: 6 / 6
[SUCCESS] All tests passed!
```

### 2. Entrenamiento

**Modo básico** (usa configuración por defecto):
```bash
cd build
./train
```

**Con archivo de configuración custom**:
```bash
./train /path/to/custom_config.yaml
```

**Monitoreo durante el entrenamiento**:
```bash
# En otra terminal:
tail -f training.log
```

**Salida esperada**:
```
[Episode   10] Reward:   45.50 | Epsilon: 0.900 | Loss: 0.0234
  Mean reward (100 eps): 42.30

[Episode   20] Reward:   78.20 | Epsilon: 0.810 | Loss: 0.0189
  Mean reward (100 eps): 56.75

...

[Episode  500] Reward:  195.00 | Epsilon: 0.050 | Loss: 0.0012
  Mean reward (100 eps): 187.45
```

**Modelos guardados**:
- `models/dqn_best.pt` - Mejor modelo durante entrenamiento
- `models/dqn_final.pt` - Modelo al finalizar
- `models/dqn_checkpoint_N.pt` - Checkpoints cada 50 episodios

### 3. Inferencia (Ejecución Autónoma)

Cargar un modelo entrenado y ejecutar el robot autónomamente:

```bash
cd build
./inference models/dqn_best.pt
```

**Con dirección MAC específica**:
```bash
./inference models/dqn_best.pt XX:XX:XX:XX:XX:XX
```

**Detener la ejecución**:
Presionar `Ctrl+C` para parar de forma segura (el robot se detendrá automáticamente).

## Estructura del Proyecto

```
Yetson/
├── CMakeLists.txt              # Configuración de CMake
├── README.md                   # Este archivo
├── .gitignore                  # Archivos ignorados por git
├── configs/
│   └── hyperparameters.yaml    # Configuración de hiperparámetros
├── include/                    # Headers (.h)
│   ├── dqn/
│   │   ├── agent.h            # Agente DQN
│   │   ├── network.h          # Red neuronal
│   │   ├── replay_buffer.h    # Buffer de experiencia
│   │   └── types.h            # Tipos y estructuras
│   ├── environment/
│   │   ├── environment_interface.h  # Interfaz abstracta
│   │   └── lego_robot_env.h        # Entorno del robot
│   ├── communication/
│   │   ├── bluetooth_manager.h     # Gestión Bluetooth
│   │   └── protocol.h              # Protocolo de comunicación
│   └── utils/
│       ├── logger.h           # Logger
│       ├── metrics.h          # Métricas de entrenamiento
│       └── config_parser.h    # Parser YAML
├── src/                       # Implementaciones (.cpp)
│   ├── dqn/
│   ├── environment/
│   ├── communication/
│   └── utils/
├── apps/                      # Aplicaciones principales
│   ├── train.cpp             # Entrenamiento
│   ├── inference.cpp         # Inferencia
│   └── test_bluetooth.cpp    # Test Bluetooth
├── scripts/
│   ├── setup_jetson.sh       # Setup de Jetson
│   └── build.sh              # Script de compilación
└── models/                   # Modelos entrenados (.pt)
```

## Configuración Avanzada

### Modificar Hiperparámetros

Editar `configs/hyperparameters.yaml`:

```yaml
training:
  num_episodes: 1000          # Más episodios para mejor convergencia
  learning_rate: 0.0005       # LR más bajo para entrenamiento más estable
  epsilon_decay: 0.997        # Decay más lento para más exploración

network:
  hidden_dim1: 256            # Red más grande
  hidden_dim2: 256

replay:
  batch_size: 128             # Batch más grande (más memoria)
  capacity: 20000             # Buffer más grande
```

### Función de Recompensa

Ajustar pesos de recompensa en el config:

```yaml
reward:
  forward_success: 2.0        # Premiar más movimiento adelante
  collision_penalty: -2.0     # Penalizar más colisiones
  backward_penalty: -0.2
  turn_reward: 0.1            # Pequeña recompensa por girar
  orientation_bonus: 1.0      # Bonificación por orientación estable
```

## Protocolo de Comunicación Bluetooth

### Paquete de Comando (Jetson → Robot)
```
[Header][Action][Duration][Checksum]
  0xAA    uint8   uint8     uint8

Action codes:
  0 - Adelante
  1 - Atrás
  2 - Izquierda
  3 - Derecha

Duration: milisegundos (típicamente 100)
Checksum: XOR de bytes anteriores
```

### Paquete de Sensores (Robot → Jetson)
```
[Header][Gyro_X][Gyro_Y][Gyro_Z][Contact_F][Contact_S][Timestamp][Checksum]
  0xBB   int16   int16   int16    uint8      uint8      uint32     uint8

Gyro values: -32768 a 32767 (se normalizan a -1.0 a 1.0)
Contact: 0 (sin contacto) o 1 (contacto detectado)
Timestamp: milisegundos desde inicio del robot
```

## Solución de Problemas

### Error: "Bluetooth library not found"
```bash
sudo apt install libbluetooth-dev
```

### Error: "Permission denied" al conectar Bluetooth
```bash
sudo usermod -a -G bluetooth $USER
# Cerrar sesión y volver a entrar
```

### Error: "LibTorch not found"
Verificar que LibTorch está instalado:
```bash
ls /usr/local/libtorch
```

Si no existe, ejecutar `scripts/setup_jetson.sh` nuevamente.

### Robot no responde después de conectar
1. Verificar batería del robot
2. Probar con `test_bluetooth` para diagnóstico
3. Verificar que el robot está esperando conexiones Bluetooth
4. Intentar emparejar manualmente con `bluetoothctl`

### Entrenamiento muy lento
- Verificar que CUDA está habilitado:
```bash
python3 -c "import torch; print(torch.cuda.is_available())"
```
- Reducir batch_size o tamaño de red en config
- Asegurarse de que no hay otros procesos usando la GPU

### Pérdida no disminuye
- Aumentar `num_episodes`
- Probar con `learning_rate` más bajo (e.g., 0.0001)
- Verificar función de recompensa (debe haber señal clara)
- Aumentar `buffer_capacity` para mejor decorrelación

## Métricas de Éxito

### Entrenamiento
- ✅ Recompensa promedio aumenta en primeros 200 episodios
- ✅ Tasa de colisión disminuye a <10%
- ✅ Epsilon decae correctamente a 0.05
- ✅ Pérdida converge

### Rendimiento
- ✅ Latencia de inferencia <50ms por acción
- ✅ Entrenamiento completo en 2-4 horas (500 episodios)
- ✅ Uso de GPU <2GB
- ✅ Comunicación Bluetooth >95% uptime

### Despliegue
- ✅ Robot opera autónomamente 5+ minutos
- ✅ Evita obstáculos efectivamente
- ✅ Recuperación automática de errores

## Próximos Pasos

### Optimizaciones Avanzadas
- **Double DQN**: Reducir sobreestimación de Q-values
- **Dueling DQN**: Mejor estimación de valores de estado
- **Prioritized Experience Replay**: Aprender más eficientemente

### Transfer Learning
- Entrenar primero en simulación (Gazebo, PyBullet)
- Fine-tune en robot real
- Exportar modelos de Python a C++

### Mejoras de Robustez
- Domain randomization en recompensas
- Curriculum learning (tareas progresivamente más difíciles)
- Multi-task learning

## Contribuir

Contribuciones son bienvenidas. Por favor:
1. Fork del repositorio
2. Crear branch para feature (`git checkout -b feature/nueva-funcionalidad`)
3. Commit cambios (`git commit -am 'Agregar nueva funcionalidad'`)
4. Push al branch (`git push origin feature/nueva-funcionalidad`)
5. Crear Pull Request

## Licencia

[Especificar licencia aquí]

## Autores

- [Tu nombre/equipo]
- Curso de Robótica - [Universidad/Institución]

## Referencias

- [Playing Atari with Deep Reinforcement Learning](https://arxiv.org/abs/1312.5602) (Mnih et al., 2013)
- [Human-level control through deep reinforcement learning](https://www.nature.com/articles/nature14236) (Mnih et al., 2015)
- [LibTorch Documentation](https://pytorch.org/cppdocs/)
- [BlueZ - Linux Bluetooth Stack](http://www.bluez.org/)

## Contacto

Para preguntas o soporte:
- GitHub: https://github.com/GabrielPacco/jetson_test
- Issues: https://github.com/GabrielPacco/jetson_test/issues

---

**Nota**: Este proyecto fue desarrollado como parte del laboratorio de Robótica. El código está optimizado para NVIDIA Jetson AGX Xavier pero puede adaptarse para otras plataformas ARM64 con CUDA.
