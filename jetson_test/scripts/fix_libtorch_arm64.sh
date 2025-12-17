#!/bin/bash
# ==============================================================================
# Script de Corrección: Instalar LibTorch ARM64 para Jetson AGX Xavier
# ==============================================================================
# Este script arregla el error "file in wrong format" reemplazando LibTorch
# x86_64 con la versión correcta para ARM64.
# ==============================================================================

set -e  # Exit on error

echo "========================================================================="
echo "  Corrección de LibTorch para Jetson AGX Xavier (ARM64)"
echo "========================================================================="
echo ""

# Verificar que estamos en ARM64
ARCH=$(uname -m)
if [ "$ARCH" != "aarch64" ]; then
    echo "ERROR: Este script es solo para arquitectura ARM64 (aarch64)"
    echo "Arquitectura detectada: $ARCH"
    exit 1
fi

echo "✓ Arquitectura correcta detectada: $ARCH"
echo ""

# Verificar CUDA
if ! command -v nvcc &> /dev/null; then
    echo "ERROR: CUDA no detectado. Instala CUDA primero."
    exit 1
fi

CUDA_VERSION=$(nvcc --version | grep "release" | sed 's/.*release //' | sed 's/,.*//')
echo "✓ CUDA detectado: $CUDA_VERSION"
echo ""

# ==============================================================================
# Paso 1: Eliminar LibTorch incorrecto
# ==============================================================================
echo "[1/4] Eliminando LibTorch incorrecto..."
if [ -d "/usr/local/libtorch" ]; then
    echo "Eliminando /usr/local/libtorch..."
    sudo rm -rf /usr/local/libtorch
    echo "✓ Eliminado"
else
    echo "✓ /usr/local/libtorch no existe (limpio)"
fi
echo ""

# ==============================================================================
# Paso 2: Instalar dependencias
# ==============================================================================
echo "[2/4] Instalando dependencias..."
sudo apt update
sudo apt install -y \
    python3-pip \
    libopenblas-dev \
    libopenmpi-dev \
    libjpeg-dev \
    zlib1g-dev \
    libpython3-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev

echo "✓ Dependencias instaladas"
echo ""

# ==============================================================================
# Paso 3: Instalar PyTorch de NVIDIA para Jetson
# ==============================================================================
echo "[3/4] Instalando PyTorch de NVIDIA para Jetson..."

# Determinar versión de JetPack
# JetPack 5.x usa CUDA 11.4, JetPack 4.x usa CUDA 10.2

if [[ "$CUDA_VERSION" == 11.* ]]; then
    echo "Detectado JetPack 5.x (CUDA 11.x)"
    PYTORCH_WHEEL="https://developer.download.nvidia.com/compute/redist/jp/v50/pytorch/torch-2.0.0+nv23.05-cp38-cp38-linux_aarch64.whl"
elif [[ "$CUDA_VERSION" == 10.* ]]; then
    echo "Detectado JetPack 4.x (CUDA 10.x)"
    PYTORCH_WHEEL="https://nvidia.box.com/shared/static/ssf2v7pf5i245fk4i0q926hy4imzs2ph.whl"
else
    echo "ERROR: Versión de CUDA no soportada: $CUDA_VERSION"
    echo "Visite: https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048"
    exit 1
fi

echo "Descargando e instalando PyTorch..."
pip3 install --no-cache-dir "$PYTORCH_WHEEL"

if [ $? -ne 0 ]; then
    echo "ERROR: Fallo al instalar PyTorch."
    echo "Intenta instalación manual desde:"
    echo "https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048"
    exit 1
fi

echo "✓ PyTorch instalado"
echo ""

# ==============================================================================
# Paso 4: Crear symlink a LibTorch
# ==============================================================================
echo "[4/4] Creando symlink a LibTorch..."

# Obtener ruta de PyTorch instalado
TORCH_PATH=$(python3 -c "import torch; print(torch.__path__[0])" 2>/dev/null)

if [ -z "$TORCH_PATH" ]; then
    echo "ERROR: No se pudo localizar PyTorch instalado"
    exit 1
fi

echo "PyTorch encontrado en: $TORCH_PATH"

# Crear symlink
sudo ln -sf "$TORCH_PATH" /usr/local/libtorch

if [ ! -L /usr/local/libtorch ]; then
    echo "ERROR: Fallo al crear symlink"
    exit 1
fi

echo "✓ Symlink creado: /usr/local/libtorch -> $TORCH_PATH"
echo ""

# ==============================================================================
# Verificación
# ==============================================================================
echo "========================================================================="
echo "  Verificación"
echo "========================================================================="

# Verificar que libtorch.so existe
if [ ! -f "/usr/local/libtorch/lib/libtorch.so" ]; then
    echo "✗ ERROR: /usr/local/libtorch/lib/libtorch.so no encontrado"
    exit 1
fi

# Verificar arquitectura
echo -n "Arquitectura de LibTorch: "
file /usr/local/libtorch/lib/libtorch.so | grep -o "aarch64\|ARM aarch64\|x86-64\|x86_64"

file /usr/local/libtorch/lib/libtorch.so | grep -q "aarch64"
if [ $? -eq 0 ]; then
    echo "✓ CORRECTO: LibTorch es ARM64 (aarch64)"
else
    echo "✗ ERROR: LibTorch NO es ARM64"
    echo "Salida de 'file':"
    file /usr/local/libtorch/lib/libtorch.so
    exit 1
fi

# Verificar version de PyTorch
TORCH_VERSION=$(python3 -c "import torch; print(torch.__version__)" 2>/dev/null)
echo "✓ PyTorch version: $TORCH_VERSION"

# Verificar CUDA disponible en PyTorch
CUDA_AVAILABLE=$(python3 -c "import torch; print(torch.cuda.is_available())" 2>/dev/null)
if [ "$CUDA_AVAILABLE" = "True" ]; then
    echo "✓ CUDA disponible en PyTorch"
else
    echo "⚠ WARNING: CUDA no disponible en PyTorch (puede funcionar solo en CPU)"
fi

echo ""
echo "========================================================================="
echo "  ✓ LibTorch ARM64 instalado correctamente"
echo "========================================================================="
echo ""
echo "Próximos pasos:"
echo "  1. Actualizar variables de entorno:"
echo "     echo 'export LD_LIBRARY_PATH=/usr/local/libtorch/lib:\$LD_LIBRARY_PATH' >> ~/.bashrc"
echo "     source ~/.bashrc"
echo ""
echo "  2. Recompilar proyecto:"
echo "     cd ~/Documents/jetson_test/Yetson"
echo "     rm -rf build/ && mkdir build && cd build"
echo "     cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch -DCMAKE_BUILD_TYPE=Release .."
echo "     make train_simulation -j\$(nproc)"
echo ""
echo "========================================================================="
