# 🎮 Ejecutar Solo DQN (Sin Robot Lego)

Guía para probar el algoritmo DQN en **modo simulación** sin necesidad del robot físico.

---

## ✨ Características

- ✅ **Sin Bluetooth** - No requiere robot Lego
- ✅ **Sin hardware** - Solo simulación por software
- ✅ **Rápido** - Prueba el algoritmo DQN en minutos
- ✅ **CartPole** - Problema clásico de control
- ✅ **Mismo código DQN** - Misma implementación que usarás con el robot

---

## 🚀 Instalación en Jetson (Versión Simplificada)

### 1. Clonar Repositorio

```bash
cd ~
git clone https://github.com/GabrielPacco/jetson_test.git
cd jetson_test/Yetson
```

### 2. Instalar Solo LibTorch (No Bluetooth)

```bash
# Actualizar sistema
sudo apt update

# Instalar herramientas básicas
sudo apt install -y build-essential cmake wget unzip

# Descargar LibTorch para Jetson
cd /tmp
wget https://download.pytorch.org/libtorch/cu117/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu117.zip
unzip libtorch*.zip
sudo mv libtorch /usr/local/

# Variables de entorno
echo 'export LD_LIBRARY_PATH=/usr/local/libtorch/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# Volver al proyecto
cd ~/jetson_test/Yetson
```

**⚠️ NOTA:** No necesitas instalar Bluetooth ni yaml-cpp para la simulación.

### 3. Compilar Solo Simulación

```bash
cd ~/jetson_test/Yetson
mkdir build
cd build

# Configurar CMake (solo simulación)
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# Compilar solo la aplicación de simulación
make train_simulation -j$(nproc)
```

---

## 🎯 Ejecutar DQN en Simulación

### Comando Básico

```bash
cd ~/jetson_test/Yetson/build
./train_simulation
```

### Con Episodios Personalizados

```bash
# 100 episodios (rápido, ~2 minutos)
./train_simulation 100

# 500 episodios (completo, ~10 minutos)
./train_simulation 500

# 1000 episodios (más entrenamiento)
./train_simulation 1000
```

---

## 📊 Salida Esperada

```
=========================================================================
  DQN Training - SIMULATION MODE (CartPole)
  Sin robot físico - Solo prueba de algoritmo
=========================================================================
[Device] cuda:0
[Environment] Creando entorno CartPole simulado...
[CartPoleEnv] Entorno de simulación inicializado (sin hardware)
[Agent] Creando DQN agent...
[DQNAgent] Initialized with:
  State dim: 4
  Action dim: 2
  Hidden dims: [128, 128]
  Device: cuda:0

[Training] Iniciando entrenamiento simulado...
  Episodios: 500
  Objetivo: Recompensa promedio >= 195
=========================================================================

[Episode   10] Reward:   23.00 | Epsilon: 0.904 | Loss: 0.0125
  Reward medio (100 eps): 23.00

[Episode   20] Reward:   45.00 | Epsilon: 0.818 | Loss: 0.0089
  Reward medio (100 eps): 34.50

...

[Episode  250] Reward:  195.00 | Epsilon: 0.105 | Loss: 0.0012
  Reward medio (100 eps): 187.30

🎉 RESUELTO en episodio 251!
   Recompensa promedio: 195.20

=========================================================================
  Entrenamiento Simulado Completado
=========================================================================
Mejor recompensa: 500.00
Epsilon final: 0.104

Modelos guardados:
  - models/dqn_simulation_best.pt
  - models/dqn_simulation_final.pt
=========================================================================
```

---

## 📈 Métricas de Éxito

| Métrica | Valor Objetivo | Significado |
|---------|----------------|-------------|
| Recompensa promedio | ≥ 195 | Episodio se considera "resuelto" |
| Mejor recompensa | ~500 | Máximo posible (episodio completo) |
| Epsilon final | ~0.01-0.1 | Exploración mínima alcanzada |
| Loss | < 0.01 | Red neuronal convergió |

---

## 🔍 Diferencias vs Robot Real

| Aspecto | Simulación | Robot Lego Real |
|---------|-----------|-----------------|
| **Entorno** | CartPole (software) | Robot físico con sensores |
| **Estados** | 4D (posición, velocidad, ángulo, velocidad angular) | 4D (orientación X/Y, contactos) |
| **Acciones** | 2 (izquierda, derecha) | 4 (adelante, atrás, izq, der) |
| **Comunicación** | Ninguna | Bluetooth RFCOMM |
| **Velocidad** | Muy rápido (~10 min para 500 eps) | Lento (~2-4 horas para 500 eps) |
| **Hardware** | Solo GPU/CPU | Jetson + Robot Lego |
| **Objetivo** | Equilibrar poste vertical | Navegar evitando obstáculos |

---

## 💾 Archivos Generados

Después de ejecutar `train_simulation`:

```bash
# Ver modelos guardados
ls -lh models/
# dqn_simulation_best.pt   - Mejor modelo durante entrenamiento
# dqn_simulation_final.pt  - Modelo al finalizar

# Ver log de entrenamiento
cat simulation_training.log

# Ver métricas CSV
cat simulation_metrics.csv
```

---

## 🎓 Uso del Modelo Entrenado

Aunque el modelo fue entrenado en CartPole, sirve para:

1. **Verificar que el algoritmo DQN funciona**
2. **Probar que LibTorch está instalado correctamente**
3. **Confirmar que la GPU funciona (CUDA)**
4. **Ver cómo converge el entrenamiento**

**⚠️ NOTA:** El modelo de simulación NO funcionará con el robot real (diferentes estados/acciones).

---

## 🔧 Troubleshooting Simulación

### Error: "LibTorch not found"

```bash
# Verificar instalación
ls /usr/local/libtorch

# Si no existe, reinstalar:
cd /tmp
wget https://download.pytorch.org/libtorch/cu117/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu117.zip
unzip libtorch*.zip
sudo mv libtorch /usr/local/
```

### Error: "CUDA out of memory"

```bash
# Editar train_simulation.cpp línea 25-26:
# Cambiar hidden_dim1 y hidden_dim2 de 128 a 64

# Recompilar:
cd ~/jetson_test/Yetson/build
make train_simulation
```

### Entrenamiento no converge

```bash
# Ejecutar con más episodios
./train_simulation 1000

# O ajustar learning rate (requiere editar código)
```

---

## 📝 Comandos Rápidos de Referencia

```bash
# ============== COMPILACIÓN ==============
cd ~/jetson_test/Yetson/build
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch -DCMAKE_BUILD_TYPE=Release ..
make train_simulation -j$(nproc)

# ============== EJECUCIÓN ==============
./train_simulation              # 500 episodios (default)
./train_simulation 100          # Rápido
./train_simulation 1000         # Más entrenamiento

# ============== MONITOREO ==============
tail -f simulation_training.log  # Ver log en tiempo real
watch -n 1 nvidia-smi           # Ver uso de GPU

# ============== ARCHIVOS ==============
ls -lh models/                  # Ver modelos
cat simulation_metrics.csv      # Ver métricas
```

---

## ⏱️ Tiempo de Ejecución Estimado

| Episodios | Tiempo (Jetson AGX Xavier) | Uso Típico |
|-----------|---------------------------|------------|
| 100 | ~2 minutos | Prueba rápida |
| 500 | ~10 minutos | Entrenamiento estándar |
| 1000 | ~20 minutos | Entrenamiento extendido |

**Nota:** Mucho más rápido que con robot real (sin delays de Bluetooth).

---

## ✅ Checklist de Validación

**Antes de ejecutar:**
- [ ] Jetson AGX Xavier conectada
- [ ] LibTorch instalado en `/usr/local/libtorch`
- [ ] Proyecto clonado y compilado
- [ ] Ejecutable `train_simulation` existe en `build/`

**Durante la ejecución:**
- [ ] Muestra "cuda:0" (si hay GPU)
- [ ] Recompensa aumenta gradualmente
- [ ] Loss disminuye
- [ ] Epsilon decae de 1.0 hacia 0.01

**Después de ejecutar:**
- [ ] Archivos `.pt` generados en `models/`
- [ ] `simulation_training.log` creado
- [ ] Recompensa promedio >= 195 (idealmente)

---

## 🎯 Próximos Pasos

Una vez que la simulación funciona:

1. **Verificar DQN funciona** ✅
2. **Instalar dependencias de Bluetooth** (para robot real)
3. **Compilar versión completa** (`make train`)
4. **Conectar robot Lego** vía Bluetooth
5. **Entrenar con robot real** usando `./train`

Ver `INSTRUCCIONES_EJECUCION.md` para el flujo completo con robot físico.

---

## 📞 Soporte

- **Simulación no funciona:** Verificar LibTorch instalado
- **Quiero usar robot real:** Ver `INSTRUCCIONES_EJECUCION.md`
- **Más información DQN:** Ver `README.md`
- **Issues GitHub:** https://github.com/GabrielPacco/jetson_test/issues

---

**¡Perfecto para probar DQN sin hardware!** 🎮🚀
