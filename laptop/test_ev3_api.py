"""Test para explorar la API de ev3-dc"""
import ev3_dc as ev3

print("=== Explorando API de ev3-dc ===\n")

# Conectar
print("Conectando al EV3...")
my_ev3 = ev3.EV3(protocol=ev3.USB)
print("[OK] Conectado\n")

# Ver atributos y métodos disponibles
print("Métodos disponibles en el objeto EV3:")
methods = [m for m in dir(my_ev3) if not m.startswith('_')]
for method in methods:
    print(f"  - {method}")

print("\n=== Probando control de motores ===")

# Intentar diferentes formas de controlar motores
try:
    print("\nProbando: my_ev3.sound.tone()...")
    my_ev3.sound.tone(1, 440, 200)
    print("[OK] Beep funcionó")
except Exception as e:
    print(f"[ERROR] {e}")

# Probar motor directo
try:
    import time
    print("\nProbando: Motor directo...")
    motor = ev3.Motor(ev3.PORT_A, ev3_obj=my_ev3)
    print("[OK] Motor creado")

    print("Métodos del motor:")
    motor_methods = [m for m in dir(motor) if not m.startswith('_')]
    for method in motor_methods:
        print(f"  - {method}")

except Exception as e:
    print(f"[ERROR] {e}")

print("\n=== Test completado ===")
