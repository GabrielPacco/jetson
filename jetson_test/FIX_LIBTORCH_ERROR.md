# 🔧 Fix: Error "file in wrong format" en LibTorch

Si obtienes este error durante la compilación:
```
/usr/bin/ld: /usr/local/libtorch/lib/libtorch.so: error adding symbols: file in wrong format
```

**Causa:** LibTorch instalado es para arquitectura x86_64 en lugar de ARM64 (aarch64).

---

## 🚨 Solución Rápida (5-10 minutos)

### Paso 1: Obtener Script de Corrección

```bash
cd ~/Documents/jetson_test/Yetson
git pull origin main
```

### Paso 2: Ejecutar Script de Corrección Automática

```bash
cd ~/Documents/jetson_test/Yetson
chmod +x scripts/fix_libtorch_arm64.sh
./scripts/fix_libtorch_arm64.sh
```

Este script:
- ✅ Elimina LibTorch incorrecto
- ✅ Instala PyTorch de NVIDIA para Jetson (ARM64)
- ✅ Crea symlink a LibTorch
- ✅ Verifica arquitectura correcta

### Paso 3: Actualizar Variables de Entorno

```bash
echo 'export LD_LIBRARY_PATH=/usr/local/libtorch/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

### Paso 4: Recompilar Proyecto

```bash
cd ~/Documents/jetson_test/Yetson
rm -rf build/
mkdir build
cd build

cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch -DCMAKE_BUILD_TYPE=Release ..
make train_simulation -j$(nproc)
```

### Paso 5: Verificar Compilación

```bash
./train_simulation 10
```

**Si funciona, verás:**
```
=========================================================================
  DQN Training - SIMULATION MODE (CartPole)
=========================================================================
[Device] cuda:0
[Environment] Creando entorno CartPole simulado...
...
```

---

## 🔍 Solución Manual (Si el Script Falla)

### Método 1: Instalar PyTorch desde Wheel de NVIDIA

```bash
# 1. Eliminar LibTorch incorrecto
sudo rm -rf /usr/local/libtorch

# 2. Instalar dependencias
sudo apt update
sudo apt install -y python3-pip libopenblas-dev libopenmpi-dev

# 3. Descargar e instalar PyTorch de NVIDIA
# Para JetPack 5.x (CUDA 11.4):
wget https://developer.download.nvidia.com/compute/redist/jp/v50/pytorch/torch-2.0.0+nv23.05-cp38-cp38-linux_aarch64.whl
pip3 install torch-2.0.0+nv23.05-cp38-cp38-linux_aarch64.whl

# Para JetPack 4.6 (CUDA 10.2):
wget https://nvidia.box.com/shared/static/ssf2v7pf5i245fk4i0q926hy4imzs2ph.whl -O torch-1.10.0-cp36-cp36m-linux_aarch64.whl
pip3 install torch-1.10.0-cp36-cp36m-linux_aarch64.whl

# 4. Crear symlink a LibTorch
TORCH_PATH=$(python3 -c "import torch; print(torch.__path__[0])")
sudo ln -s $TORCH_PATH /usr/local/libtorch

# 5. Verificar arquitectura
file /usr/local/libtorch/lib/libtorch.so
# DEBE mostrar: "ELF 64-bit LSB shared object, ARM aarch64"
```

### Método 2: Construir PyTorch desde Fuente (Lento, 2-4 horas)

```bash
# 1. Clonar PyTorch
cd ~
git clone --recursive --branch v2.0.0 https://github.com/pytorch/pytorch.git
cd pytorch

# 2. Instalar dependencias
sudo apt install -y ninja-build
pip3 install -r requirements.txt

# 3. Configurar build
export USE_CUDA=1
export USE_CUDNN=1
export TORCH_CUDA_ARCH_LIST="7.2"  # Para Jetson AGX Xavier
export USE_NCCL=0
export USE_DISTRIBUTED=0

# 4. Compilar (TARDA 2-4 HORAS)
python3 setup.py install

# 5. Crear symlink
TORCH_PATH=$(python3 -c "import torch; print(torch.__path__[0])")
sudo ln -s $TORCH_PATH /usr/local/libtorch
```

---

## ✅ Verificación Post-Instalación

### Comando de Verificación Completa

```bash
# Arquitectura del sistema
echo "Sistema: $(uname -m)"
# Debe mostrar: aarch64

# Arquitectura de LibTorch
echo "LibTorch:"
file /usr/local/libtorch/lib/libtorch.so | grep -o "aarch64\|x86"
# Debe mostrar: aarch64

# Versión de PyTorch
python3 -c "import torch; print(f'PyTorch: {torch.__version__}')"

# CUDA disponible
python3 -c "import torch; print(f'CUDA: {torch.cuda.is_available()}')"
# Debe mostrar: True

# Dependencias faltantes
ldd /usr/local/libtorch/lib/libtorch.so | grep "not found"
# NO debe mostrar nada
```

### Tabla de Verificación

| Check | Comando | Resultado Esperado |
|-------|---------|-------------------|
| Sistema ARM64 | `uname -m` | aarch64 |
| LibTorch ARM64 | `file /usr/local/libtorch/lib/libtorch.so` | ARM aarch64 |
| PyTorch instalado | `python3 -c "import torch"` | Sin errores |
| CUDA disponible | `python3 -c "import torch; print(torch.cuda.is_available())"` | True |
| Symlink correcto | `ls -l /usr/local/libtorch` | → apunta a torch path |

---

## 🐛 Troubleshooting

### Error: "No module named 'torch'"

```bash
# PyTorch no instalado, ejecutar:
./scripts/fix_libtorch_arm64.sh
```

### Error: "CUDA not available"

```bash
# Verificar CUDA
nvcc --version

# Verificar que PyTorch tenga soporte CUDA
python3 -c "import torch; print(torch.version.cuda)"
```

### Error: "cannot find -ltorch"

```bash
# Actualizar LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/libtorch/lib:$LD_LIBRARY_PATH
ldconfig
```

### Compilación sigue fallando

```bash
# Limpiar completamente y recompilar
cd ~/Documents/jetson_test/Yetson
rm -rf build/
rm -rf /tmp/cmake*

mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_VERBOSE_MAKEFILE=ON \
      ..

make train_simulation VERBOSE=1 2>&1 | tee compile.log
```

---

## 📚 Referencias

- **NVIDIA PyTorch for Jetson:**
  https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048

- **PyTorch Jetson Installation Guide:**
  https://docs.nvidia.com/deeplearning/frameworks/install-pytorch-jetson-platform/

- **JetPack SDK:**
  https://developer.nvidia.com/jetpack-sdk-50

---

## 💡 Prevención

Para evitar este problema en el futuro:

1. **Siempre** usa el script `fix_libtorch_arm64.sh` para instalar LibTorch
2. **Nunca** descargues LibTorch directamente desde pytorch.org (es para x86)
3. **Usa** wheels pre-compilados de NVIDIA para Jetson
4. **Verifica** arquitectura con `file` después de instalar

---

## 📊 Comparación de Métodos

| Método | Tiempo | Dificultad | Recomendado |
|--------|--------|------------|-------------|
| Script automático | 5-10 min | Fácil | ✅ Sí |
| Wheel de NVIDIA | 10-15 min | Media | ✅ Sí |
| Construir desde fuente | 2-4 horas | Difícil | ❌ Solo si otros fallan |

---

**¿Sigue sin funcionar?** Abre un issue en:
https://github.com/GabrielPacco/jetson_test/issues

Con la salida de:
```bash
uname -a
nvcc --version
file /usr/local/libtorch/lib/libtorch.so
python3 -c "import torch; print(torch.__version__)"
```
