"""
EV3 Controller - Controla motores A y D por USB
Funciona con firmware LEGO ORIGINAL (sin microSD/ev3dev)

INSTALACIÓN:
  pip install ev3-dc

CONEXIÓN:
  1. Conecta EV3 a laptop por USB (puerto PC del EV3)
  2. Enciende el EV3 (con firmware LEGO original)
  3. En Windows: Device Manager debe mostrar "LEGO EV3" o similar
"""

try:
    import ev3_dc as ev3
    EV3_AVAILABLE = True
except ImportError:
    print("ADVERTENCIA: ev3-dc no instalado. Ejecuta: pip install ev3-dc")
    EV3_AVAILABLE = False

import time


class EV3Controller:
    """
    Controla motores A (rueda izquierda) y D (rueda derecha)
    Motor C se ignora
    """

    def __init__(self, speed_forward=40, speed_turn=30):
        """
        Args:
            speed_forward: Velocidad para FORWARD/BACKWARD (% de potencia, 0-100)
            speed_turn: Velocidad para giros (% de potencia, 0-100)
        """
        if not EV3_AVAILABLE:
            raise ImportError("ev3-dc no disponible")

        # Conectar al EV3 por USB
        try:
            self.ev3_brick = ev3.EV3(protocol=ev3.USB)
            print("[OK] Conectado al EV3 por USB")
        except Exception as e:
            raise ConnectionError(f"No se pudo conectar al EV3: {e}")

        # Crear objetos Motor
        try:
            self.motor_left = ev3.Motor(ev3.PORT_A, ev3_obj=self.ev3_brick)
            self.motor_right = ev3.Motor(ev3.PORT_D, ev3_obj=self.ev3_brick)
            print("[OK] Motores inicializados")
        except Exception as e:
            raise ConnectionError(f"Error al inicializar motores: {e}")

        # Velocidades (ev3-dc usa -100 a 100)
        self.speed_forward = speed_forward
        self.speed_turn = speed_turn

        # Tabla de acciones
        self.actions = {
            0: self.stop,
            1: self.forward,
            2: self.turn_left,
            3: self.turn_right,
            4: self.backward
        }

    def stop(self):
        """Acción 0: STOP"""
        self.motor_left.stop(brake=True)
        self.motor_right.stop(brake=True)

    def forward(self):
        """Acción 1: FORWARD - ambas ruedas adelante"""
        self.motor_left.start_move(speed=self.speed_forward, direction=1)
        self.motor_right.start_move(speed=self.speed_forward, direction=1)

    def turn_left(self):
        """Acción 2: TURN_LEFT - giro sobre eje"""
        self.motor_left.start_move(speed=self.speed_turn, direction=-1)
        self.motor_right.start_move(speed=self.speed_turn, direction=1)

    def turn_right(self):
        """Acción 3: TURN_RIGHT - giro sobre eje"""
        self.motor_left.start_move(speed=self.speed_turn, direction=1)
        self.motor_right.start_move(speed=self.speed_turn, direction=-1)

    def backward(self):
        """Acción 4: BACKWARD - ambas ruedas atrás"""
        self.motor_left.start_move(speed=self.speed_forward, direction=-1)
        self.motor_right.start_move(speed=self.speed_forward, direction=-1)

    def execute_action(self, action):
        """
        Ejecuta acción (0-4)
        Retorna True si exitoso, False si acción inválida
        """
        if action in self.actions:
            self.actions[action]()
            return True
        else:
            print(f"[ERROR] Acción inválida: {action}")
            self.stop()
            return False

    def cleanup(self):
        """Detiene motores y desconecta"""
        self.stop()
        time.sleep(0.2)
        print("Motores detenidos")


# Test standalone
if __name__ == '__main__':
    import time

    print("=== Test EV3 Controller (Firmware LEGO) ===")
    print("Asegúrate de que el EV3 esté conectado por USB\n")

    try:
        controller = EV3Controller(speed_forward=30, speed_turn=25)

        # Secuencia de prueba
        tests = [
            (1, "FORWARD", 2),
            (0, "STOP", 1),
            (2, "TURN_LEFT", 1.5),
            (0, "STOP", 1),
            (3, "TURN_RIGHT", 1.5),
            (0, "STOP", 1),
            (4, "BACKWARD", 2),
            (0, "STOP", 1)
        ]

        print("Ejecutando secuencia de prueba...")
        for action, name, duration in tests:
            print(f"\n[{name}] durante {duration}s")
            controller.execute_action(action)
            time.sleep(duration)

        print("\n[OK] Prueba completada")

    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()

    finally:
        if 'controller' in locals():
            controller.cleanup()
