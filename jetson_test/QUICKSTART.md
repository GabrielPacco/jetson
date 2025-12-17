# 🚀 Quick Start - DQN Lego Robot

Guía ultra-rápida para poner en marcha el proyecto en 30 minutos.

---

## ⚡ Comandos Secuenciales (Copy-Paste)

### 1️⃣ Clonar y Configurar (5 min)

```bash
# En la Jetson AGX Xavier
cd ~
git clone https://github.com/GabrielPacco/jetson_test.git
cd jetson_test/Yetson

# Instalar dependencias
chmod +x scripts/setup_jetson.sh
./scripts/setup_jetson.sh

# IMPORTANTE: Cerrar sesión y volver a entrar después
logout
```

**Después de volver a conectar:**

```bash
# Cargar variables de entorno
source ~/.bashrc

# Verificar CUDA
nvcc --version
```

---

### 2️⃣ Configurar Robot (2 min)

```bash
# Obtener MAC address del robot
hcitool scan
# Copia la dirección: XX:XX:XX:XX:XX:XX

# Editar configuración
cd ~/jetson_test/Yetson
nano configs/hyperparameters.yaml

# Cambiar línea:
# bluetooth_address: "00:1A:7D:DA:71:13"  # Por tu MAC real
# Guardar: Ctrl+O, Enter, Ctrl+X
```

---

### 3️⃣ Compilar (5 min)

```bash
cd ~/jetson_test/Yetson
chmod +x scripts/build.sh
./scripts/build.sh
```

**✅ Esperado:** `Build Complete!` al final

---

### 4️⃣ Test Bluetooth (2 min)

```bash
cd ~/jetson_test/Yetson/build

# Reemplazar XX:XX:XX:XX:XX:XX con tu MAC
./test_bluetooth XX:XX:XX:XX:XX:XX
```

**✅ Esperado:** `Tests passed: 6 / 6` y `[SUCCESS] All tests passed!`

**❌ Si falla:** Verificar que:
- Robot está encendido
- Bluetooth activo en robot
- Batería cargada

---

### 5️⃣ Entrenar (2-4 horas)

```bash
cd ~/jetson_test/Yetson/build
./train
```

**Monitorear en otra terminal:**
```bash
# Terminal 2
ssh nvidia@<IP_JETSON>
tail -f ~/jetson_test/Yetson/training.log

# Terminal 3 (GPU)
watch -n 1 nvidia-smi
```

**Detener:** `Ctrl+C` (se guarda automáticamente)

---

### 6️⃣ Ejecutar Modelo (Inferencia)

```bash
cd ~/jetson_test/Yetson/build
./inference models/dqn_best.pt
```

**Detener:** `Ctrl+C`

---

## 📝 Comandos de Referencia Ultra-Rápida

```bash
# ========== UBICACIONES ==========
cd ~/jetson_test/Yetson                    # Directorio principal
cd ~/jetson_test/Yetson/build              # Ejecutables
cd ~/jetson_test/Yetson/models             # Modelos guardados
cd ~/jetson_test/Yetson/configs            # Configuración

# ========== BLUETOOTH ==========
hcitool scan                               # Escanear robots
./test_bluetooth XX:XX:XX:XX:XX:XX         # Test completo

# ========== BUILD ==========
./scripts/build.sh                         # Compilar todo
cd build && make -j$(nproc)                # Solo recompilar

# ========== TRAIN ==========
./train                                    # Config por defecto
./train ../configs/hyperparameters.yaml    # Config custom
tail -f ../training.log                    # Ver log

# ========== INFERENCE ==========
./inference models/dqn_best.pt             # Mejor modelo
./inference models/dqn_final.pt            # Modelo final

# ========== ARCHIVOS ==========
ls -lh models/                             # Ver modelos
cat training.log                           # Ver log completo
cat training_metrics.csv                   # Ver métricas
```

---

## 🔧 Solución Rápida de Problemas

| Problema | Solución |
|----------|----------|
| Errores de compilación | Ver `VERIFICACION_COMPILACION.md` (guía completa) |
| Bluetooth no conecta | `sudo systemctl restart bluetooth` |
| LibTorch not found | `ls /usr/local/libtorch` (debe existir) |
| Permission denied BT | `sudo usermod -a -G bluetooth $USER && logout` |
| CUDA out of memory | Reducir batch_size a 32 en config |
| Tests 5/6 passed | Verificar batería del robot |
| Loss no baja | Entrenar más episodios (1000+) |

---

## ✅ Checklist Mínimo

**Antes de entrenar:**
- [ ] `git clone` exitoso
- [ ] `setup_jetson.sh` sin errores
- [ ] MAC configurado en YAML
- [ ] Build completo (train, inference, test_bluetooth)
- [ ] Test Bluetooth: 6/6 passed
- [ ] Robot con >80% batería

**Entrenamiento:**
- [ ] Primeros 10 episodios muestran exploración
- [ ] Epsilon decae gradualmente
- [ ] Loss comienza a bajar
- [ ] Modelos se guardan en `models/`

**Post-entrenamiento:**
- [ ] `dqn_best.pt` existe
- [ ] Inferencia carga modelo
- [ ] Robot navega autónomamente

---

## 🎯 Resultados Esperados

**Después de 500 episodios:**
- Recompensa promedio: 150-195
- Epsilon final: ~0.05
- Loss: <0.01
- Tasa de colisiones: <10%

**Inferencia:**
- Robot evita obstáculos
- Navegación fluida
- Recompensa >150 por episodio

---

## 📞 Si Algo Falla

1. **Errores de compilación:** `VERIFICACION_COMPILACION.md` (troubleshooting completo)
2. **Guía detallada:** `INSTRUCCIONES_EJECUCION.md` (paso a paso)
3. **Logs:** `cat training.log | grep ERROR`
4. **Issues:** https://github.com/GabrielPacco/jetson_test/issues

---

## ⏱️ Timeline Típico

| Paso | Tiempo | Total Acumulado |
|------|--------|-----------------|
| Clonar + Setup | 5-10 min | 10 min |
| Configurar robot | 2 min | 12 min |
| Compilar | 5-10 min | 22 min |
| Test Bluetooth | 2 min | 24 min |
| **PRIMER RUN COMPLETO** | **~25 min** | **25 min** |
| Entrenar (500 eps) | 2-4 horas | 2-4 horas |
| Inferencia | Continuo | - |

**Meta:** Sistema funcionando en <30 minutos, entrenado en <4 horas.

---

**¡Buena suerte!** 🤖⚡
