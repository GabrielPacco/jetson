"""
Test simple para simular el Jetson enviando acciones UDP al bridge
Ejecuta esto después de iniciar el bridge
"""
import socket
import time
import sys
import os

# Añadir parent directory al path para importar config
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from config import UDP_PORT, LOCALHOST

def send_action(sock, action, host="127.0.0.1", port=UDP_PORT):
    """Envía una acción al bridge"""
    message = str(action).encode('utf-8')
    sock.sendto(message, (host, port))
    action_name = ["STOP", "FORWARD", "TURN_LEFT", "TURN_RIGHT", "BACKWARD"][action]
    print(f"Enviado: {action} ({action_name})")

def main():
    print("=== Test Bridge Sender ===")
    print(f"Enviando a localhost:{UDP_PORT}\n")

    # Crear socket UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        # Secuencia de prueba simple
        tests = [
            (1, "FORWARD", 2),
            (0, "STOP", 1),
            (2, "TURN_LEFT", 1.5),
            (0, "STOP", 1),
            (3, "TURN_RIGHT", 1.5),
            (0, "STOP", 1),
        ]

        print("Ejecutando secuencia de prueba...")
        print("(Asegurate de que el bridge este corriendo!)\n")

        for action, name, duration in tests:
            send_action(sock, action)
            time.sleep(duration)

        print("\n[OK] Prueba completada")

    except KeyboardInterrupt:
        print("\n\nInterrumpido por usuario")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
    finally:
        sock.close()

if __name__ == '__main__':
    main()
