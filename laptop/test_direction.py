"""Test para controlar dirección del motor"""
import ev3_dc as ev3
import time

print("Conectando...")
my_ev3 = ev3.EV3(protocol=ev3.USB)
motor_left = ev3.Motor(ev3.PORT_A, ev3_obj=my_ev3)
motor_right = ev3.Motor(ev3.PORT_D, ev3_obj=my_ev3)

# Revisar documentación del método cont
print("\nProbando método cont()...")
print(f"cont.__doc__: {motor_left.cont.__doc__}")

# Intentar diferentes métodos
print("\n=== Probando diferentes formas de movimiento ===")

# Test 1: cont() para movimiento continuo
try:
    print("\n1. Probando cont(30) - adelante")
    ops = motor_left.cont(30)
    my_ev3.send_direct_cmd(ops)
    time.sleep(1)
    motor_left.stop()
    print("[OK]")
except Exception as e:
    print(f"[ERROR] {e}")

# Test 2: cont() con velocidad negativa
try:
    print("\n2. Probando cont(-30) - atrás")
    ops = motor_left.cont(-30)
    my_ev3.send_direct_cmd(ops)
    time.sleep(1)
    motor_left.stop()
    print("[OK]")
except Exception as e:
    print(f"[ERROR] {e}")

print("\n=== Test completado ===")
