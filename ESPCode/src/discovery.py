#!/usr/bin/env python3
"""
Test Discovery Service Multi-Interface
Testa automaticamente tutte le interfacce di rete disponibili
"""

import socket
import time
import sys
from datetime import datetime
import netifaces
import ipaddress

# Configurazione
DISCOVERY_PORT = 5555
MAGIC_STRING = b"LEDMATRIX_DISCOVER"
TIMEOUT_PER_INTERFACE = 2  # secondi per interfaccia
BROADCASTS_PER_INTERFACE = 3


class NetworkInterface:
    """Rappresenta un'interfaccia di rete"""
    def __init__(self, name, ip, netmask, broadcast=None):
        self.name = name
        self.ip = ip
        self.netmask = netmask
        self.broadcast = broadcast or self._calculate_broadcast()
        
    def _calculate_broadcast(self):
        """Calcola indirizzo broadcast dalla subnet"""
        try:
            network = ipaddress.IPv4Network(f"{self.ip}/{self.netmask}", strict=False)
            return str(network.broadcast_address)
        except:
            # Fallback a .255
            parts = self.ip.split('.')
            return f"{parts[0]}.{parts[1]}.{parts[2]}.255"
    
    def __str__(self):
        return f"{self.name}: {self.ip}/{self.netmask} (broadcast: {self.broadcast})"


def get_network_interfaces():
    """Ottiene tutte le interfacce di rete attive con IPv4"""
    interfaces = []
    
    try:
        for iface_name in netifaces.interfaces():
            # Salta loopback
            if iface_name.startswith('lo'):
                continue
                
            addrs = netifaces.ifaddresses(iface_name)
            
            # Cerca indirizzi IPv4
            if netifaces.AF_INET in addrs:
                for addr_info in addrs[netifaces.AF_INET]:
                    ip = addr_info.get('addr')
                    netmask = addr_info.get('netmask')
                    broadcast = addr_info.get('broadcast')
                    
                    if ip and ip != '127.0.0.1' and netmask:
                        interfaces.append(NetworkInterface(
                            iface_name, ip, netmask, broadcast
                        ))
    except Exception as e:
        print(f"Errore lettura interfacce: {e}")
        # Fallback a metodo semplice
        interfaces = get_network_interfaces_fallback()
    
    return interfaces


def get_network_interfaces_fallback():
    """Metodo fallback senza netifaces"""
    interfaces = []
    
    try:
        # Ottieni IP locale principale
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        
        # Assumi /24
        interfaces.append(NetworkInterface(
            "default", local_ip, "255.255.255.0"
        ))
    except:
        pass
    
    return interfaces


def test_interface(interface, devices_found):
    """Testa una singola interfaccia di rete"""
    
    print(f"\n{'='*70}")
    print(f"  Test interfaccia: {interface.name}")
    print(f"{'='*70}")
    print(f"  IP locale: {interface.ip}")
    print(f"  Netmask: {interface.netmask}")
    print(f"  Broadcast: {interface.broadcast}")
    print()
    
    try:
        # Crea socket e bind sull'IP specifico dell'interfaccia
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # IMPORTANTE: Bind sull'IP specifico dell'interfaccia
        sock.bind((interface.ip, 0))
        local_port = sock.getsockname()[1]
        
        print(f"  ✓ Socket bound su {interface.ip}:{local_port}")
        
        sock.settimeout(0.3)
        
        print(f"  Invio {BROADCASTS_PER_INTERFACE} broadcast...")
        
        interface_devices = {}
        start_time = time.time()
        broadcast_count = 0
        
        while time.time() - start_time < TIMEOUT_PER_INTERFACE:
            # Invia broadcast
            try:
                sock.sendto(MAGIC_STRING, (interface.broadcast, DISCOVERY_PORT))
                broadcast_count += 1
                print(f"    [{datetime.now().strftime('%H:%M:%S.%f')[:-3]}] "
                      f"Broadcast #{broadcast_count} → {interface.broadcast}")
            except Exception as e:
                print(f"    ✗ Errore invio: {e}")
            
            # Ascolta risposte
            listen_start = time.time()
            while time.time() - listen_start < 0.5:
                try:
                    data, addr = sock.recvfrom(1024)
                    response = data.decode('utf-8').strip()
                    
                    print(f"\n    >>> RISPOSTA da {addr[0]}:{addr[1]}")
                    print(f"        {response}")
                    
                    # Analizza risposta
                    if response.startswith("LEDMATRIX_HERE"):
                        parts = response.split(',')
                        if len(parts) >= 4:
                            device_name = parts[1]
                            device_ip = parts[2]
                            device_port = parts[3]
                            
                            # Se IP è 0.0.0.0, usa l'IP del mittente
                            if device_ip == "0.0.0.0":
                                device_ip = addr[0]
                                print(f"        ℹ IP era 0.0.0.0, uso IP mittente: {device_ip}")
                            
                            key = f"{device_ip}:{device_port}"
                            
                            if key not in interface_devices:
                                device_info = {
                                    'name': device_name,
                                    'ip': device_ip,
                                    'port': device_port,
                                    'interface': interface.name,
                                    'first_seen': datetime.now(),
                                    'sender_ip': addr[0]
                                }
                                interface_devices[key] = device_info
                                devices_found[key] = device_info
                                
                                print(f"        ✓ DISPOSITIVO TROVATO!")
                                print(f"          Nome: {device_name}")
                                print(f"          IP: {device_ip}")
                                print(f"          Porta: {device_port}")
                                print(f"          Via: {interface.name}")
                    print()
                    
                except socket.timeout:
                    pass
                except Exception as e:
                    pass
            
            time.sleep(0.6)
        
        sock.close()
        
        print(f"  Completato: {len(interface_devices)} dispositivi trovati su {interface.name}")
        return len(interface_devices)
        
    except Exception as e:
        print(f"  ✗ Errore su {interface.name}: {e}")
        return 0


def test_all_interfaces():
    """Testa tutte le interfacce di rete disponibili"""
    
    print("="*70)
    print("  Discovery Service Multi-Interface - LED Matrix ESP32")
    print("="*70)
    print()
    
    # Ottieni tutte le interfacce
    print("[1] Rilevamento interfacce di rete...")
    interfaces = get_network_interfaces()
    
    if not interfaces:
        print("  ✗ Nessuna interfaccia di rete trovata!")
        return []
    
    print(f"  ✓ Trovate {len(interfaces)} interfacce:\n")
    for i, iface in enumerate(interfaces, 1):
        print(f"    {i}. {iface}")
    
    # Testa ogni interfaccia
    print(f"\n[2] Test di ogni interfaccia...")
    all_devices = {}
    
    for iface in interfaces:
        found = test_interface(iface, all_devices)
        time.sleep(0.5)  # Pausa tra interfacce
    
    # Riepilogo
    print(f"\n{'='*70}")
    print("  RIEPILOGO FINALE")
    print(f"{'='*70}\n")
    
    if all_devices:
        print(f"✓ Trovati {len(all_devices)} dispositivi in totale:\n")
        
        for key, device in all_devices.items():
            print(f"  • {device['name']}")
            print(f"    IP reale: {device['ip']}")
            print(f"    IP mittente: {device['sender_ip']}")
            print(f"    Porta: {device['port']}")
            print(f"    Via interfaccia: {device['interface']}")
            print(f"    Trovato: {device['first_seen'].strftime('%H:%M:%S')}")
            print()
    else:
        print("✗ Nessun dispositivo trovato su nessuna interfaccia\n")
        print("Suggerimenti:")
        print("  1. Verifica che ESP32 sia acceso e connesso")
        print("  2. Controlla il monitor seriale ESP32")
        print("  3. Prova a disabilitare temporaneamente una delle interfacce")
        print("  4. Verifica firewall/antivirus")
        print()
    
    return list(all_devices.values())


def interactive_test():
    """Test interattivo con selezione interfaccia"""
    
    print("="*70)
    print("  Test Discovery - Selezione Interfaccia")
    print("="*70)
    print()
    
    # Ottieni interfacce
    interfaces = get_network_interfaces()
    
    if not interfaces:
        print("✗ Nessuna interfaccia trovata!")
        return
    
    print("Interfacce disponibili:\n")
    for i, iface in enumerate(interfaces, 1):
        print(f"  {i}. {iface}")
    
    print(f"  {len(interfaces)+1}. Testa tutte le interfacce")
    print("  0. Annulla")
    print()
    
    try:
        choice = int(input("Scegli interfaccia: "))
        
        if choice == 0:
            return
        elif choice == len(interfaces) + 1:
            test_all_interfaces()
        elif 1 <= choice <= len(interfaces):
            devices = {}
            test_interface(interfaces[choice-1], devices)
        else:
            print("✗ Scelta non valida")
            
    except ValueError:
        print("✗ Input non valido")
    except KeyboardInterrupt:
        print("\n\nInterrotto")


def main():
    """Main function"""
    
    print("""
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║       LED Matrix ESP32 - Multi-Interface Discovery Test         ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
    """)
    
    # Controlla se netifaces è disponibile
    try:
        import netifaces
        print("✓ netifaces disponibile - modalità avanzata attiva\n")
    except ImportError:
        print("ℹ netifaces non disponibile - uso modalità base")
        print("  Per funzionalità complete: pip install netifaces\n")
    
    if len(sys.argv) > 1:
        if sys.argv[1] == "auto":
            test_all_interfaces()
        elif sys.argv[1] == "help":
            print("Utilizzo:")
            print("  python3 test_discovery_multi.py           - Test automatico")
            print("  python3 test_discovery_multi.py auto      - Test tutte interfacce")
            print("  python3 test_discovery_multi.py select    - Selezione interfaccia")
            print()
        elif sys.argv[1] == "select":
            interactive_test()
        else:
            print(f"Comando sconosciuto: {sys.argv[1]}")
    else:
        # Default: test automatico di tutte le interfacce
        test_all_interfaces()


if __name__ == "__main__":
    main()