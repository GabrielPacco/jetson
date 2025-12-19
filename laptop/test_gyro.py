#!/usr/bin/env python3
"""
Test de lectura del Gyro Sensor
Ejecutar ANTES de usar bridge.py para verificar que el sensor funciona
"""

try:
    import ev3_dc as ev3
    print("[OK] ev3-dc importado correctamente")
except ImportError:
    print("[ERROR] ev3-dc no instalado")
    print("Ejecuta: pip install ev3-dc")
    exit(1)

import time

print("\n" + "="*60)
print("Test de Gyro Sensor - EV3")
print("="*60)
print("\nConectando al EV3 por USB...")

try:
    # Conectar al EV3
    ev3_brick = ev3.EV3(protocol=ev3.USB)
    print("[OK] Conectado al EV3 por USB")
except Exception as e:
    print(f"[ERROR] No se pudo conectar al EV3: {e}")
    print("\nVerifica:")
    print("  1. EV3 está encendido")
    print("  2. Cable USB conectado")
    print("  3. Device Manager muestra 'LEGO EV3'")
    exit(1)

print("\nIntentando inicializar Gyro Sensor en Puerto 2...")

try:
    # Intentar crear sensor
    gyro_sensor = ev3.Sensor(ev3.PORT_2, ev3_obj=ev3_brick)
    print("[OK] Gyro Sensor creado")
except Exception as e:
    print(f"[ERROR] No se pudo crear sensor: {e}")
    exit(1)

print("\n" + "="*60)
print("Leyendo valores del sensor (10 lecturas)...")
print("Mueve el robot para ver cambios en los valores")
print("="*60 + "\n")

# Descubrir API del sensor
print("Métodos disponibles en el objeto sensor:")
methods = [m for m in dir(gyro_sensor) if not m.startswith('_')]
print(methods)
print()

for i in range(10):
    try:
        # Intentar diferentes métodos de lectura
        print(f"\nLectura {i+1}/10:")

        # Método 1: read()
        try:
            value = gyro_sensor.read()
            print(f"  gyro_sensor.read() = {value} (tipo: {type(value).__name__})")
        except Exception as e:
            print(f"  gyro_sensor.read() - ERROR: {e}")

        # Método 2: value (propiedad)
        try:
            value = gyro_sensor.value
            print(f"  gyro_sensor.value = {value} (tipo: {type(value).__name__})")
        except Exception as e:
            print(f"  gyro_sensor.value - ERROR: {e}")

        # Método 3: Atributos específicos (si existen)
        try:
            if hasattr(gyro_sensor, 'angle'):
                print(f"  gyro_sensor.angle = {gyro_sensor.angle}")
            if hasattr(gyro_sensor, 'rate'):
                print(f"  gyro_sensor.rate = {gyro_sensor.rate}")
        except Exception as e:
            print(f"  Atributos específicos - ERROR: {e}")

        time.sleep(1)

    except KeyboardInterrupt:
        print("\n\nTest interrumpido por usuario")
        break
    except Exception as e:
        print(f"  ERROR GENERAL: {e}")
        break

print("\n" + "="*60)
print("Test completado")
print("="*60)
print("\nINSTRUCCIONES:")
print("1. Identifica cuál método funcionó (read(), value, angle, etc.)")
print("2. Anota el tipo de dato devuelto (int, float, tuple, list)")
print("3. Compárteme la salida completa para ajustar ev3_controller.py")
print("="*60)
