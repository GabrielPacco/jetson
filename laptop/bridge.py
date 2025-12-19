    #!/usr/bin/env python3
"""
Laptop Bridge - Puente BIDIRECCIONAL entre Jetson (UDP) y EV3 (USB)
Corre en Windows

FLUJO:
  Jetson → [UDP] → Bridge → [USB] → EV3  (Acciones)
  Jetson ← [UDP] ← Bridge ← [USB] ← EV3  (Sensores)

PROTOCOLO:
  1. Jetson envía: "ACTION:N" (donde N = 0-4)
  2. Bridge ejecuta acción en EV3
  3. Bridge lee sensores del EV3
  4. Bridge responde: "SENSORS:gyro_angle,gyro_rate,touch_front,touch_side"

SEGURIDAD:
  - Si no recibe del Jetson por >0.5s → envía STOP al EV3
  - Logging de todas las acciones y sensores
"""
import socket
import time
import threading
from datetime import datetime
import sys
import os
import json

# Añadir parent directory al path para importar config
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from config import UDP_PORT, TIMEOUT_SECONDS, SPEED_FORWARD, SPEED_TURN

from ev3_controller import EV3Controller


class Bridge:
    """Puente Jetson ↔ EV3"""

    def __init__(self, port=UDP_PORT, timeout=TIMEOUT_SECONDS):
        self.port = port
        self.timeout = timeout

        # Estado
        self.last_action = 0
        self.last_received_time = time.time()
        self.running = False

        # Log
        self.log_file = f"bridge_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        self.log(f"=== Bridge iniciado ===")

        # Inicializar EV3
        try:
            self.ev3 = EV3Controller(speed_forward=SPEED_FORWARD, speed_turn=SPEED_TURN)
            self.log("[OK] EV3 conectado")
        except Exception as e:
            self.log(f"ERROR: No se pudo conectar al EV3: {e}")
            raise

        # Socket UDP
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(0.1)  # Timeout corto para recvfrom
        self.sock.bind(('0.0.0.0', self.port))
        self.log(f"[OK] Escuchando en UDP puerto {self.port}")

    def log(self, message):
        """Registra mensaje en consola y archivo"""
        timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        log_msg = f"[{timestamp}] {message}"
        print(log_msg)
        with open(self.log_file, 'a') as f:
            f.write(log_msg + '\n')

    def receive_action(self):
        """Recibe acción del Jetson (bloqueante con timeout)"""
        try:
            data, addr = self.sock.recvfrom(1024)
            action = int(data.decode('utf-8').strip())
            self.last_received_time = time.time()
            return action, addr
        except socket.timeout:
            return None, None
        except ValueError as e:
            self.log(f"ERROR: Dato inválido recibido: {e}")
            return None, None

    def execute_action(self, action, client_addr=None):
        """
        Ejecuta acción en EV3 y opcionalmente envía sensores de vuelta

        Args:
            action: Código de acción (0-4)
            client_addr: (ip, port) del cliente para enviar respuesta. Si None, no envía.

        Returns:
            sensor_data: dict con valores de sensores (o None si no hay client_addr)
        """
        if self.ev3.execute_action(action):
            self.last_action = action
            action_name = ["STOP", "FORWARD", "TURN_LEFT", "TURN_RIGHT", "BACKWARD"][action]
            self.log(f"[OK] Accion {action} ({action_name}) ejecutada")
        else:
            self.log(f"[ERROR] Accion {action} invalida, enviando STOP")
            return None

        # Leer sensores después de ejecutar acción
        sensor_data = self.ev3.read_sensors()

        # Si hay client_addr, enviar sensores de vuelta
        if client_addr is not None:
            self.send_sensors(sensor_data, client_addr)

        return sensor_data

    def send_sensors(self, sensor_data, client_addr):
        """
        Envía datos de sensores al Jetson por UDP

        Formato: "gyro_angle,gyro_rate,touch_front,touch_side"
        Ejemplo: "12.5,-3.2,0,1"
        """
        try:
            # Formato CSV simple
            msg = f"{sensor_data['gyro_angle']:.2f},{sensor_data['gyro_rate']:.2f},{sensor_data['touch_front']},{sensor_data['touch_side']}"
            self.sock.sendto(msg.encode('utf-8'), client_addr)
            self.log(f"-> Sensores enviados a {client_addr[0]}:{client_addr[1]}: {msg}")
        except Exception as e:
            self.log(f"[ERROR] Enviando sensores: {e}")

    def watchdog(self):
        """Thread que monitorea timeout y envía STOP si no recibe"""
        while self.running:
            elapsed = time.time() - self.last_received_time

            if elapsed > self.timeout and self.last_action != 0:
                self.log(f"[WARNING] TIMEOUT ({elapsed:.2f}s sin recibir) -> STOP")
                self.execute_action(0)
                self.last_action = 0

            time.sleep(0.1)

    def run(self):
        """Loop principal del puente"""
        self.running = True

        # Iniciar watchdog en thread separado
        watchdog_thread = threading.Thread(target=self.watchdog, daemon=True)
        watchdog_thread.start()

        self.log("Bridge operativo. Esperando acciones del Jetson...")
        self.log("Presiona Ctrl+C para detener\n")

        try:
            while self.running:
                action, addr = self.receive_action()

                if action is not None:
                    self.log(f"<- Recibido: {action} desde {addr[0]}:{addr[1]}")
                    self.execute_action(action)

        except KeyboardInterrupt:
            self.log("\n\nDeteniendo bridge...")
        finally:
            self.shutdown()

    def shutdown(self):
        """Limpieza al cerrar"""
        self.running = False
        self.log("Enviando STOP final al EV3...")
        self.ev3.execute_action(0)
        time.sleep(0.5)
        self.ev3.cleanup()
        self.sock.close()
        self.log("Bridge cerrado")


def main():
    print("=== Laptop Bridge ===")
    print(f"Puerto UDP: {UDP_PORT}")
    print(f"Timeout: {TIMEOUT_SECONDS}s")
    print(f"Velocidades: Forward={SPEED_FORWARD}%, Turn={SPEED_TURN}%")
    print()

    try:
        bridge = Bridge()
        bridge.run()
    except Exception as e:
        print(f"\nError fatal: {e}")
        import traceback
        traceback.print_exc()


if __name__ == '__main__':
    main()
