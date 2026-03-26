import serial
import serial.tools.list_ports
import platform
from db_config import get_db_connection

def detect_arduino_port():
    ports = list(serial.tools.list_ports.comports())
    system = platform.system()
    for port in ports:
        if system == "Windows" and "COM6" in port.device:
            return port.device
    return None

def update_database(plate, balance, uid):
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        # Create card_balances table if not exists
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS card_balances (
                uid VARCHAR(8) PRIMARY KEY,
                plate_number VARCHAR(20),
                balance INT NOT NULL
            )
        """)
        # Update or insert balance
        cursor.execute("""
            INSERT INTO card_balances (uid, plate_number, balance)
            VALUES (%s, %s, %s)
            ON DUPLICATE KEY UPDATE balance = %s, plate_number = %s
        """, (uid, plate, balance, balance, plate))
        conn.commit()
        print(f"[DATABASE] Updated balance for UID {uid}: {balance}")
        cursor.close()
        conn.close()
    except Exception as e:
        print(f"[ERROR] Database update failed: {e}")

def main():
    port = detect_arduino_port()
    if not port:
        print("[ERROR] Arduino not found")
        return
    try:
        ser = serial.Serial(port, 9600, timeout=1)
        print(f"[CONNECTED] Listening on {port}")
        ser.reset_input_buffer()
        while True:
            if ser.in_waiting:
                line = ser.readline().decode().strip()
                print(f"[SERIAL] Received: {line}")
                if line.startswith("DATA:"):
                    try:
                        _, data = line.split("DATA:", 1)
                        plate, balance, uid = data.strip().split(',')
                        balance = int(balance)
                        print(f"[INFO] Plate: {plate}, Balance: {balance}, UID: {uid}")
                    except ValueError as e:
                        print(f"[ERROR] Invalid data format: {e}")
                elif line == "DONE":
                    if 'plate' in locals() and 'balance' in locals() and 'uid' in locals():
                        update_database(plate, balance, uid)
                    else:
                        print("[ERROR] No valid data to update")
    except KeyboardInterrupt:
        print("[EXIT] Program terminated")
    except Exception as e:
        print(f"[ERROR] {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()