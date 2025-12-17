"""
Script para verificar conexión del EV3
"""
import sys

print("=== Verificación de EV3 ===\n")

# 1. Verificar ev3-dc
try:
    import ev3_dc as ev3
    print("[OK] ev3-dc instalado")
except ImportError:
    print("[X] ev3-dc NO instalado")
    print("  Solucion: pip install ev3-dc")
    sys.exit(1)

# 2. Verificar pyusb
try:
    import usb.core
    print("[OK] pyusb instalado")
except ImportError:
    print("[X] pyusb NO instalado")
    print("  Solucion: pip install pyusb")
    sys.exit(1)

# 3. Buscar dispositivos USB LEGO
print("\nBuscando dispositivos USB...")
try:
    devices = list(usb.core.find(find_all=True))

    if not devices:
        print("[X] NO se encontraron dispositivos USB")
        print("\nPOSIBLES CAUSAS:")
        print("1. EV3 no esta conectado por USB")
        print("2. EV3 no esta encendido")
        print("3. Falta driver libusb en Windows")
        print("\nSOLUCION WINDOWS:")
        print("Descarga libusb-win32 desde: https://sourceforge.net/projects/libusb-win32/")
        print("O usa Zadig: https://zadig.akeo.ie/")
    else:
        print(f"[OK] Se encontraron {len(devices)} dispositivos USB\n")

        # Buscar LEGO EV3 especificamente
        lego_found = False
        for dev in devices:
            try:
                vendor_id = dev.idVendor
                product_id = dev.idProduct

                # LEGO vendor ID: 0x0694, EV3 product ID: 0x0005
                if vendor_id == 0x0694 and product_id == 0x0005:
                    print(f"[OK] EV3 ENCONTRADO!")
                    print(f"   Vendor ID: 0x{vendor_id:04x}")
                    print(f"   Product ID: 0x{product_id:04x}")
                    lego_found = True
                    break
            except:
                pass

        if not lego_found:
            print("[X] EV3 NO encontrado entre los dispositivos USB")
            print("\nDispositivos detectados:")
            for dev in devices:
                try:
                    print(f"  - Vendor:Product = {dev.idVendor:04x}:{dev.idProduct:04x}")
                except:
                    pass
            print("\nEL EV3 deberia aparecer como: 0694:0005")

except usb.core.NoBackendError:
    print("[X] ERROR: No hay backend USB disponible")
    print("\nSOLUCION WINDOWS:")
    print("1. Descarga libusb-win32: https://sourceforge.net/projects/libusb-win32/")
    print("2. Extrae el archivo")
    print("3. Copia libusb0.dll a C:\\Windows\\System32\\")
    print("\nO usa Zadig para instalar driver:")
    print("1. Descarga: https://zadig.akeo.ie/")
    print("2. Conecta EV3")
    print("3. En Zadig: Options -> List All Devices")
    print("4. Selecciona 'LEGO EV3'")
    print("5. Instala driver 'libusb-win32'")

print("\n" + "="*50)
