#!/usr/bin/env python3
"""
Test Sender - Simula el Jetson para testing local
Permite enviar acciones manualmente al bridge
"""
import socket
import time
import sys
import os

# Añadir parent directory al path para importar config
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from config import LOCALHOST, UDP_PORT, ACTIONS

LAPTOP_IP = LOCALHOST  # Localhost para testing local


def send_action(sock, action):
    """Envía acción al bridge"""
    message = str(action).encode('utf-8')
    sock.sendto(message, (LAPTOP_IP, UDP_PORT))
    print(f"→ Enviado: {action} ({ACTIONS[action]})")


def interactive_mode():
    """Modo interactivo: elige acciones con teclado"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print("\n=== Test Sender Interactivo ===")
    print("Comandos:")
    print("  0 = STOP")
    print("  1 = FORWARD")
    print("  2 = TURN_LEFT")
    print("  3 = TURN_RIGHT")
    print("  4 = BACKWARD")
    print("  q = Salir")
    print()

    try:
        while True:
            cmd = input("Acción [0-4 o q]: ").strip()

            if cmd == 'q':
                break

            try:
                action = int(cmd)
                if action in ACTIONS:
                    send_action(sock, action)
                else:
                    print("Acción inválida. Usa 0-4")
            except ValueError:
                print("Entrada inválida. Usa 0-4 o q")

    except KeyboardInterrupt:
        print("\n")
    finally:
        # Enviar STOP al salir
        print("Enviando STOP final...")
        send_action(sock, 0)
        sock.close()


def sequence_mode():
    """Modo secuencia: ejecuta prueba predefinida"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print("\n=== Test Sender Secuencia ===")

    sequence = [
        (1, "FORWARD", 2.0),
        (0, "STOP", 1.0),
        (2, "TURN_LEFT", 1.5),
        (0, "STOP", 1.0),
        (3, "TURN_RIGHT", 1.5),
        (0, "STOP", 1.0),
        (4, "BACKWARD", 2.0),
        (0, "STOP", 1.0)
    ]

    try:
        for action, name, duration in sequence:
            print(f"\n[{name}] durante {duration}s")
            send_action(sock, action)
            time.sleep(duration)

        print("\n✓ Secuencia completada")

    except KeyboardInterrupt:
        print("\n\nInterrumpido")
    finally:
        send_action(sock, 0)
        sock.close()


def main():
    print("\n=== Test Sender ===")
    print("Asegúrate de que bridge.py esté corriendo en otra terminal")
    print()
    print("Modo:")
    print("  1 = Interactivo (control manual)")
    print("  2 = Secuencia (prueba automática)")
    print()

    choice = input("Selecciona modo [1 o 2]: ").strip()

    if choice == '1':
        interactive_mode()
    elif choice == '2':
        sequence_mode()
    else:
        print("Opción inválida")


if __name__ == '__main__':
    main()
