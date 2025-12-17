"""
Configuración común para todo el sistema
Edita estos valores según tu setup
"""

# ============================================================================
# RED (Jetson ↔ Laptop)
# ============================================================================

# IP de la laptop Windows (donde corre el bridge)
LAPTOP_IP = "192.168.1.100"  # CAMBIAR a la IP real de tu laptop

# Puerto UDP para comunicación
UDP_PORT = 5000

# ============================================================================
# SEGURIDAD
# ============================================================================

# Timeout: si bridge no recibe del Jetson en este tiempo, envía STOP
TIMEOUT_SECONDS = 0.5

# ============================================================================
# VELOCIDADES EV3 (% de potencia, 0-100)
# ============================================================================

# Velocidad para FORWARD y BACKWARD
SPEED_FORWARD = 40  # Empieza bajo, sube si es muy lento

# Velocidad para giros (TURN_LEFT, TURN_RIGHT)
SPEED_TURN = 30     # Baja si patina, sube si gira muy lento

# ============================================================================
# DQN (Jetson)
# ============================================================================

# Frecuencia de envío de acciones (Hz)
ACTION_FREQUENCY = 5  # 5 acciones por segundo

# Tabla de acciones (NO CAMBIAR sin actualizar todo el código)
ACTIONS = {
    0: "STOP",
    1: "FORWARD",
    2: "TURN_LEFT",
    3: "TURN_RIGHT",
    4: "BACKWARD"
}

# ============================================================================
# LOGGING
# ============================================================================

# Nivel de verbosidad (DEBUG, INFO, WARNING, ERROR)
LOG_LEVEL = "INFO"

# Guardar logs a archivo
SAVE_LOGS = True

# ============================================================================
# CALIBRACIÓN MOTORES
# ============================================================================

# Si el robot va hacia atrás cuando debería ir adelante, cambia a True
INVERT_FORWARD = False

# Si gira al revés, cambia a True
INVERT_TURNS = False

# ============================================================================
# OBTENER IP AUTOMÁTICAMENTE (para testing)
# ============================================================================

def get_local_ip():
    """Obtiene IP local de la máquina"""
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


# Para testing local (ambos en la misma máquina)
LOCALHOST = "127.0.0.1"
