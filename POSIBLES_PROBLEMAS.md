# Posibles Problemas al Compilar y Ejecutar

## PROBLEMAS DE COMPILACIÓN (Jetson)

### 1. Error: "Torch not found"
**Causa:** CMake no encuentra LibTorch
**Solución:**
```bash
# Encontrar ruta real de LibTorch
find /usr -name "libtorch" 2>/dev/null

# Usar ruta encontrada en cmake
cmake -DCMAKE_PREFIX_PATH=/usr/local/lib/python3.8/dist-packages/torch ..
```

### 2. Error: "undefined reference to torch::..."
**Causa:** Linking incorrecto con LibTorch
**Solución:**
```bash
# Limpiar build y recompilar
cd jetson_cpp
rm -rf build
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/ruta/a/libtorch -DCMAKE_BUILD_TYPE=Release ..
make clean
make -j4
```

### 3. Error: "C++17 required"
**Causa:** Compilador muy antiguo
**Solución:**
```bash
# Verificar versión de g++
g++ --version

# Debe ser >= 7.0
# Si no, actualizar: sudo apt install g++-9
```

### 4. Warnings sobre "deprecated declarations"
**Causa:** API de LibTorch cambia entre versiones
**Solución:** Son warnings, el código debe compilar. Si falla, ajustar código a nueva API.

---

## PROBLEMAS DE EJECUCIÓN (Laptop - Bridge)

### 5. Error: "ModuleNotFoundError: No module named 'ev3_dc'"
**Causa:** Librería ev3-dc no instalada
**Solución:**
```cmd
pip install ev3-dc
```

**NOTA IMPORTANTE:** Si `pip install ev3-dc` falla, la librería correcta podría ser:
```cmd
pip install python-ev3dev2
```

Editar `laptop/bridge.py` y `laptop/ev3_controller.py` para usar `python-ev3dev2` en vez de `ev3-dc`.

### 6. Error: "No se pudo conectar al EV3"
**Causa:** EV3 no conectado o no reconocido por USB
**Diagnóstico:**
```cmd
# En Windows, abrir Device Manager
# Buscar "LEGO EV3" o dispositivo USB desconocido
```

**Soluciones:**
- Verificar que EV3 esté encendido
- Reconectar cable USB
- Probar otro puerto USB
- Instalar drivers LEGO (descargar de LEGO.com)

### 7. Error: "No se pudo inicializar Gyro Sensor"
**Causa:** Sensor no conectado al Puerto 2, o tipo de sensor incorrecto
**Solución:**
- Verificar físicamente que el Gyro Sensor esté en Puerto 2
- En el EV3, ir a menú "View" → "Port View" y verificar que Puerto 2 muestra "Gyro"

**Si el sensor está en otro puerto:**
Editar `laptop/ev3_controller.py`:
```python
# Cambiar de PORT_2 a PORT_1, PORT_3, o PORT_4
self.gyro_sensor = ev3.Sensor(ev3.PORT_1, ev3_obj=self.ev3_brick)
```

### 8. Error al leer sensores: "AttributeError: 'Sensor' object has no attribute 'read'"
**Causa:** API de ev3-dc diferente a la asumida
**Solución:** Editar `laptop/ev3_controller.py` línea ~169:
```python
# Probar diferentes métodos de lectura
value = self.gyro_sensor.read()  # Intentar primero
# O:
value = self.gyro_sensor.value   # Propiedad en vez de método
# O:
value = self.gyro_sensor.angle()  # Método específico de gyro
```

**Para descubrir API correcta:**
```python
# Agregar en __init__ después de crear sensor:
print(dir(self.gyro_sensor))  # Ver métodos disponibles
```

---

## PROBLEMAS DE EJECUCIÓN (Jetson - Entrenamiento/Inferencia)

### 9. Error: "CUDA out of memory"
**Causa:** GPU sin memoria suficiente
**Solución:** El código automáticamente usa CPU como fallback. Verificar en logs:
```
[Device] cuda  # GPU funcionando
[Device] cpu   # Fallback a CPU (más lento pero funciona)
```

### 10. Error: "Connection refused" al ejecutar train_robot
**Causa:** Bridge no está corriendo en laptop, o IP incorrecta
**Solución:**
```bash
# Verificar IP de la laptop
# En laptop (Windows):
ipconfig

# En Jetson, usar esa IP:
./train_robot 192.168.1.XXX 100
```

**Verificar que bridge esté corriendo:**
- En laptop debe verse: `[OK] Escuchando en UDP puerto 5000`

### 11. Warning: "No se recibieron sensores válidos"
**Causa:** Bridge no envía sensores, o timeout muy corto
**Solución 1:** Verificar que bridge muestre mensajes de sensores enviados

**Solución 2:** Aumentar timeout en `jetson_cpp/main.cpp` línea ~142:
```cpp
// Cambiar de 300ms a 500ms
tv_recv.tv_usec = 500000;  // 500ms
```

### 12. Entrenamiento muy lento (>5 min por episodio)
**Causa:** Pausa de 200ms entre acciones es muy larga, o robot tarda en ejecutar
**Solución:** Reducir pausa en `train_robot.cpp` línea ~548:
```cpp
// Cambiar de 200ms a 100ms
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

**CUIDADO:** Pausas muy cortas pueden causar que el robot no complete acciones.

### 13. Robot ejecuta acciones pero estado siempre es cero
**Causa:** Sensores no se están leyendo correctamente
**Diagnóstico:** Ver logs del bridge, debe mostrar:
```
-> Sensores enviados a 192.168.1.XXX:XXXXX: 12.50,-3.20,0,1
```

**Si no se ve ese mensaje:**
- El método `read_sensors()` en `ev3_controller.py` está fallando
- Ver logs de errores en bridge
- Probar diferentes APIs de lectura (ver problema #8)

---

## PROBLEMAS DE COMUNICACIÓN UDP

### 14. Firewall bloqueando UDP puerto 5000
**Causa:** Windows Firewall bloquea Python
**Solución:**
```
1. Windows Security → Firewall & network protection
2. Allow an app through firewall
3. Buscar Python o agregar: C:\Python3X\python.exe
4. Permitir Private y Public networks
```

### 15. Jetson y Laptop en redes diferentes
**Causa:** No pueden comunicarse por UDP
**Verificación:**
```bash
# En Jetson, hacer ping a laptop
ping <laptop_ip>

# Debe responder. Si no:
# - Conectar ambos a la misma WiFi
# - O conectar por Ethernet directo
```

---

## PROBLEMAS DE FORMATO DE DATOS

### 16. Error: "Formato de sensores inválido"
**Causa:** Bridge envía formato diferente al esperado
**Esperado:** `"gyro_angle,gyro_rate,touch_front,touch_side"`
**Ejemplo:** `"12.50,-3.20,0,1"`

**Diagnóstico:** Ver logs del Jetson, debe mostrar:
```
[WARNING] Formato de sensores inválido: <datos recibidos>
```

**Solución:** Verificar que `bridge.py` línea ~123 use formato CSV correcto:
```python
msg = f"{sensor_data['gyro_angle']:.2f},{sensor_data['gyro_rate']:.2f},{sensor_data['touch_front']},{sensor_data['touch_side']}"
```

---

## PROBLEMAS ESPECÍFICOS DEL GYRO BOY

### 17. Robot se cae inmediatamente durante entrenamiento
**Causa:** Gyro Boy requiere auto-balanceo activo, código actual NO implementa control PID
**Limitación:** El código actual solo lee el gyro pero no implementa control de balanceo

**Opciones:**
1. **Usar robot con ruedas estándar (NO auto-balanceo)** - Recomendado para este proyecto
2. **Modificar rewards para que el robot aprenda a balancearse** (difícil, requiere muchos episodios)
3. **Implementar PID manual para balanceo** (fuera del scope del DQN)

**IMPORTANTE:** Si el profesor espera auto-balanceo, el proyecto actual necesitaría:
- Más episodios de entrenamiento (500-1000+)
- Rewards más complejos enfocados en mantener ángulo = 0
- Posiblemente reducir max_steps para que episodios terminen rápido al caerse

### 18. Gyro sensor devuelve siempre 0
**Causa:** Sensor no calibrado o en modo incorrecto
**Solución:**
```python
# En ev3_controller.py, después de crear sensor:
# Intentar resetear/calibrar
self.gyro_sensor.reset()  # Si el método existe
```

**O verificar en EV3:**
- Menú → View → Port View → Puerto 2
- Debe mostrar valores cambiantes al mover el robot

---

## CHECKLIST PRE-EJECUCIÓN

Antes de ejecutar `train_robot`, verificar:

### En el EV3:
- [ ] EV3 encendido
- [ ] Gyro Sensor conectado al Puerto 2
- [ ] Gyro Sensor funcionando (Port View muestra valores)
- [ ] Baterías cargadas
- [ ] Motores A y D conectados

### En la Laptop (Windows):
- [ ] `pip install ev3-dc` ejecutado
- [ ] EV3 conectado por USB
- [ ] Device Manager muestra "LEGO EV3"
- [ ] `python bridge.py` corriendo sin errores
- [ ] Se ve mensaje: "[OK] Gyro Sensor inicializado"
- [ ] Se ve mensaje: "Bridge operativo"

### En el Jetson:
- [ ] Compilación exitosa (`make -j4` sin errores)
- [ ] 3 executables creados (train_simulation, train_robot, jetson_dqn)
- [ ] Laptop IP correcta
- [ ] Jetson y Laptop en misma red
- [ ] Ping a laptop funciona

### Red:
- [ ] Ambos dispositivos en misma WiFi/LAN
- [ ] Firewall permite Python (puerto 5000)
- [ ] `ping <laptop_ip>` desde Jetson funciona

---

## COMANDOS DE DIAGNÓSTICO

### En Laptop (Windows):
```cmd
# Ver conexiones USB
devmgmt.msc

# Ver IP de laptop
ipconfig

# Probar bridge manualmente
cd laptop
python bridge.py
```

### En Jetson:
```bash
# Verificar LibTorch
find /usr -name "libtorch" 2>/dev/null

# Verificar compilación
cd jetson_cpp/build
ls -lh train_robot jetson_dqn train_simulation

# Probar ping a laptop
ping <laptop_ip>

# Ver logs de errores
./train_robot 192.168.1.100 10 2>&1 | tee training_output.log
```

---

## SOLUCIONES RÁPIDAS

**Si nada funciona:**
1. Usar `train_simulation` primero para verificar que DQN compila y funciona
2. Probar `jetson_dqn` en modo `random` para verificar comunicación UDP:
   ```bash
   ./jetson_dqn <laptop_ip> -p random
   ```
3. Verificar que motores funcionan sin DQN (ejecutar test de `ev3_controller.py`)

**Test mínimo de comunicación:**
```bash
# En Jetson, enviar paquete UDP manualmente
echo "1" | nc -u <laptop_ip> 5000
```
Si bridge recibe y mueve el robot, la comunicación funciona.
