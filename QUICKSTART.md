# Quick Start - 3 Pasos

## 1. Configurar IPs

Edita `config.py` línea 10:
```python
LAPTOP_IP = "TU_IP_AQUI"  # Pon la IP de tu laptop Windows
```

**Obtener IP de laptop**:
```cmd
ipconfig
```
Busca "IPv4 Address" de tu conexión (ej: 192.168.1.100)

---

## 2. Test Local (laptop sola)

### Terminal 1: Bridge
```cmd
cd laptop
python bridge.py
```

### Terminal 2: Test Sender
```cmd
cd laptop
python test_sender.py
```
Elige opción `1` (interactivo) y prueba acciones con teclado.

**Deberías ver**: EV3 moviéndose según las teclas que presionas.

---

## 3. Test Completo (Jetson → Laptop → EV3)

### En Laptop:
```cmd
cd laptop
python bridge.py
```

### En Jetson:
```bash
cd jetson
python3 jetson_sender.py
```

**Deberías ver**: EV3 moviéndose con acciones del DQN.

---

## Detener Todo
Presiona `Ctrl+C` en cualquier terminal.

---

## Si algo falla

**"ModuleNotFoundError: ev3dev2"**
→ `pip install python-ev3dev2`

**"No se pudo conectar al EV3"**
→ Verifica USB, enciende EV3

**"No recibe mensajes"**
→ Verifica firewall, permite Python en puerto 5000

---

**Listo. Ahora integra tu DQN editando `jetson/jetson_sender.py`**
