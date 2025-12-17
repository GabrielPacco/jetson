"""Test para encontrar cómo establecer velocidad"""
import ev3_dc as ev3
import time

print("Conectando...")
my_ev3 = ev3.EV3(protocol=ev3.USB)
motor = ev3.Motor(ev3.PORT_A, ev3_obj=my_ev3)

print("Métodos relacionados con velocidad:")
print(f"  - speed: {type(motor.speed)}")

# Probar establecer velocidad
print("\nProbando establecer velocidad...")
try:
    motor.speed = 30
    print(f"[OK] motor.speed = 30 -> Actual: {motor.speed}")
except Exception as e:
    print(f"[ERROR] {e}")

# Probar start_move
print("\nProbando start_move()...")
try:
    motor.start_move()
    print("[OK] Motor en movimiento")
    time.sleep(1)
    motor.stop()
    print("[OK] Motor detenido")
except Exception as e:
    print(f"[ERROR] {e}")

print("\n=== Test completado ===")
