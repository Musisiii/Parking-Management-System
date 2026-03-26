import csv
import serial
import time
import serial.tools.list_ports
import platform
from datetime import datetime
import math
from db_config import get_db_connection

CSV_FILE = 'plates_log.csv'
RATE_PER_HOUR = 500

def detect_arduino_port():
  ports = list(serial.tools.list_ports.comports())
  system = platform.system()
  for port in ports:
    if system == "Linux":
      if "ttyUSB" in port.device or "ttyACM" in port.device:
        return port.device
    elif system == "Darwin":
      if "cu." in port.device or "usbserial" in port.device:
        return port.device
    elif system == "Windows":
      if "COM6" in port.device:
        return port.device
  return None

def parse_arduino_data(line):
  try:
    if line.startswith("DATA:"):
      _, data = line.split("DATA:", 1)
      parts = data.strip().split(',')
      if len(parts) != 2:
        print(f"[ERROR] Invalid data format: {data}")
        return None, None
      plate = parts[0].strip()
      balance_str = parts[1].strip()
      if not plate or not balance_str.isdigit():
        print(f"[ERROR] Invalid plate or balance: {plate}, {balance_str}")
        return None, None
      balance = int(balance_str)
      return plate, balance
    return None, None
  except Exception as e:
    print(f"[ERROR] Parsing failed: {e}")
    return None, None

def process_payment(plate, balance, ser):
  try:
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
      SELECT id, entry_timestamp
      FROM parking_logs
      WHERE plate_number = %s AND payment_status = 'pending'
      ORDER BY entry_timestamp DESC LIMIT 1
    """, (plate,))
    result = cursor.fetchone()

    if not result:
      print(f"[PAYMENT] No active entry found for plate {plate}")
      ser.write(b'I\n')
      cursor.close()
      conn.close()
      return

    log_id, entry_time = result
    minutes_spent = (datetime.now() - entry_time).total_seconds() / 60
    amount_due = math.ceil(minutes_spent / 60) * RATE_PER_HOUR

    if balance < amount_due:
      print(f"[PAYMENT] Insufficient balance: {balance} < {amount_due}")
      ser.write(b'I\n')
      cursor.close()
      conn.close()
      return
    else:
      new_balance = balance - amount_due
      print(f"[WAIT] Waiting for Arduino READY...")
      start_time = time.time()
      while True:
        if ser.in_waiting:
          arduino_response = ser.readline().decode().strip()
          print(f"[ARDUINO] {arduino_response}")
          if arduino_response == "READY":
            break
        if time.time() - start_time > 10:
          print("[ERROR] Timeout waiting for READY")
          cursor.close()
          conn.close()
          return
        time.sleep(0.1)

      ser.write(f"{new_balance}\n".encode())
      print(f"[PAYMENT] Sent new balance: {new_balance}")
      start_time = time.time()
      print(f"[WAIT] Waiting for Arduino DONE...")
      while True:
        if ser.in_waiting:
          confirm = ser.readline().decode().strip()
          print(f"[ARDUINO] {confirm}")
          if "DONE" in confirm:
            print(f"[PAYMENT] Write confirmed")
            cursor.execute("""
              UPDATE parking_logs
              SET exit_timestamp = NOW(),
                  balance = %s,
                  duration = %s,
                  fee = %s,
                  payment_status = 'cleared'
              WHERE id = %s
            """, (new_balance, minutes_spent, amount_due, log_id))
            conn.commit()
            with open(CSV_FILE, 'r') as f:
              rows = list(csv.reader(f))
            header = rows[0]
            for i, row in enumerate(rows[1:]):
              if row[0] == plate and row[1] == '0':
                rows[i + 1][1] = '1'
                rows[i + 1].append(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
                break
            with open(CSV_FILE, 'w', newline='') as f:
              writer = csv.writer(f)
              writer.writerow(header)
              writer.writerows(rows)
            print(f"[PAYMENT] Payment of {amount_due} RWF successful for {plate}")
            break
          elif "[ERROR]" in confirm:
            print(f"[ERROR] Arduino failed to write balance")
            break
        if time.time() - start_time > 15:
          print("[ERROR] Timeout waiting for DONE")
          break
        time.sleep(0.1)
    cursor.close()
    conn.close()
  except Exception as e:
    print(f"[ERROR] Payment processing failed: {e}")

def main():
  port = detect_arduino_port()
  if not port:
    print("[ERROR] Arduino not found")
    return
  try:
    ser = serial.Serial(port, 9600, timeout=1)
    print(f"[CONNECTED] Listening on {port}")
    time.sleep(2)
    ser.reset_input_buffer()
    while True:
      if ser.in_waiting:
        line = ser.readline().decode().strip()
        print(f"[SERIAL] Received: {line}")
        plate, balance = parse_arduino_data(line)
        if plate and balance is not None:
          process_payment(plate, balance, ser)
  except KeyboardInterrupt:
    print("[EXIT] Program terminated")
  except Exception as e:
    print(f"[ERROR] {e}")
  finally:
    if 'ser' in locals():
      ser.close()

if __name__ == "__main__":
  main()