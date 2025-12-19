"""
EV3 Controller - Controla motores A y D por USB y lee sensores
Funciona con firmware LEGO ORIGINAL (sin microSD/ev3dev)

INSTALACIÓN:
  pip install ev3-dc

CONEXIÓN:
  1. Conecta EV3 a laptop por USB (puerto PC del EV3)
  2. Enciende el EV3 (con firmware LEGO original)
  3. En Windows: Device Manager debe mostrar "LEGO EV3" o similar

SENSORES GYRO BOY:
  - Puerto 2: Gyro Sensor (ángulo y velocidad angular)
  - Puerto 1: Touch Sensor (frontal) - OPCIONAL
  - Puerto 4: Touch Sensor (lateral) - OPCIONAL
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
    Lee sensores: Gyro + Touch (si están conectados)
    """

    def __init__(self, speed_forward=40, speed_turn=30,
                 use_gyro=True, use_touch=False):
        """
        Args:
            speed_forward: Velocidad para FORWARD/BACKWARD (% de potencia, 0-100)
            speed_turn: Velocidad para giros (% de potencia, 0-100)
            use_gyro: Intentar inicializar sensor de giroscopio (Puerto 2)
            use_touch: Intentar inicializar sensores táctiles (Puertos 1, 4)
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
            print("[OK] Motores inicializados (Puerto A, D)")
        except Exception as e:
            raise ConnectionError(f"Error al inicializar motores: {e}")

        # Inicializar sensores
        self.gyro_sensor = None
        self.touch_front = None
        self.touch_side = None

        # Gyro Sensor (típicamente Puerto 2 en Gyro Boy)
        # NOTA: ev3-dc usa clases Sensor genéricas, no específicas por tipo
        if use_gyro:
            try:
                # ev3-dc API: Sensor(port, ev3_obj=...)
                # Gyro sensor devuelve valores con .read()
                self.gyro_sensor = ev3.Sensor(ev3.PORT_2, ev3_obj=self.ev3_brick)
                print("[OK] Gyro Sensor inicializado (Puerto 2)")
                print("     IMPORTANTE: Verifica que el sensor esté conectado y sea tipo GYRO")
            except Exception as e:
                print(f"[WARNING] No se pudo inicializar Gyro Sensor: {e}")
                print("          Continuando sin giroscopio...")

        # Touch Sensors (opcional - solo si tienes sensores táctiles)
        if use_touch:
            try:
                self.touch_front = ev3.Sensor(ev3.PORT_1, ev3_obj=self.ev3_brick)
                print("[OK] Touch Sensor inicializado (Puerto 1)")
            except Exception as e:
                print(f"[INFO] Touch Sensor frontal no detectado: {e}")

            try:
                self.touch_side = ev3.Sensor(ev3.PORT_4, ev3_obj=self.ev3_brick)
                print("[OK] Touch Sensor lateral inicializado (Puerto 4)")
            except Exception as e:
                print(f"[INFO] Touch Sensor lateral no detectado: {e}")

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

    def read_sensors(self):
        """
        Lee valores de los sensores del EV3

        Returns:
            dict con:
                - gyro_angle: Ángulo del giroscopio en grados (-inf a +inf)
                - gyro_rate: Velocidad angular en grados/segundo
                - touch_front: 1 si presionado, 0 si no (o -1 si no disponible)
                - touch_side: 1 si presionado, 0 si no (o -1 si no disponible)
        """
        sensor_data = {
            'gyro_angle': 0.0,
            'gyro_rate': 0.0,
            'touch_front': -1,
            'touch_side': -1
        }

        # Leer giroscopio
        if self.gyro_sensor is not None:
            try:
                # ev3-dc API: Sensor.read() devuelve valor(es) del sensor
                # Para Gyro Sensor, típicamente devuelve ángulo
                # NOTA: Puede necesitar ajustes según el modo del sensor
                value = self.gyro_sensor.read()

                # Si read() devuelve tupla (angle, rate), desempaquetar
                # Si devuelve solo ángulo, usar directamente
                if isinstance(value, (list, tuple)) and len(value) >= 2:
                    sensor_data['gyro_angle'] = float(value[0])
                    sensor_data['gyro_rate'] = float(value[1])
                elif isinstance(value, (int, float)):
                    sensor_data['gyro_angle'] = float(value)
                    sensor_data['gyro_rate'] = 0.0  # No disponible
                else:
                    sensor_data['gyro_angle'] = 0.0
                    sensor_data['gyro_rate'] = 0.0

            except Exception as e:
                print(f"[ERROR] Leyendo giroscopio: {e}")
                print(f"        Tipo de valor recibido: {type(value) if 'value' in locals() else 'N/A'}")
                sensor_data['gyro_angle'] = 0.0
                sensor_data['gyro_rate'] = 0.0

        # Leer sensor táctil frontal
        if self.touch_front is not None:
            try:
                value = self.touch_front.read()
                # Touch sensor típicamente devuelve 0 o 1
                sensor_data['touch_front'] = 1 if value else 0
            except Exception as e:
                print(f"[ERROR] Leyendo touch frontal: {e}")
                sensor_data['touch_front'] = -1

        # Leer sensor táctil lateral
        if self.touch_side is not None:
            try:
                value = self.touch_side.read()
                sensor_data['touch_side'] = 1 if value else 0
            except Exception as e:
                print(f"[ERROR] Leyendo touch lateral: {e}")
                sensor_data['touch_side'] = -1

        return sensor_data

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
