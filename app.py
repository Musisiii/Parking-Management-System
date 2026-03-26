from flask import Flask, jsonify, render_template
from flask_cors import CORS
from db_config import get_db_connection
from datetime import datetime

app = Flask(__name__, template_folder='templates')
CORS(app)  # Allow frontend to access API

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/vehicle-logs', methods=['GET'])
def get_vehicle_logs():
    try:
        conn = get_db_connection()
        if not conn:
            return jsonify({'error': 'Database connection failed'}), 500
        cursor = conn.cursor(dictionary=True)
        cursor.execute("""
            SELECT id, plate_number, balance, entry_timestamp, exit_timestamp, 
                   duration, fee, payment_status
            FROM parking_logs
            ORDER BY entry_timestamp DESC
        """)
        logs = cursor.fetchall()
        # Format timestamps
        for log in logs:
            log['entry_timestamp'] = log['entry_timestamp'].isoformat() if log['entry_timestamp'] else None
            log['exit_timestamp'] = log['exit_timestamp'].isoformat() if log['exit_timestamp'] else None
        cursor.close()
        conn.close()
        return jsonify(logs)
    except Exception as e:
        print(f"[ERROR] Fetching vehicle logs: {e}")
        return jsonify({'error': 'Failed to fetch vehicle logs'}), 500

@app.route('/api/incident-logs', methods=['GET'])
def get_incident_logs():
    try:
        conn = get_db_connection()
        if not conn:
            return jsonify({'error': 'Database connection failed'}), 500
        cursor = conn.cursor(dictionary=True)
        cursor.execute("""
            SELECT id, plate_number, timestamp, gate_location, incident_type
            FROM incident_logs
            ORDER BY timestamp DESC
        """)
        logs = cursor.fetchall()
        # Format timestamps
        for log in logs:
            log['timestamp'] = log['timestamp'].isoformat()
        cursor.close()
        conn.close()
        return jsonify(logs)
    except Exception as e:
        print(f"[ERROR] Fetching incident logs: {e}")
        return jsonify({'error': 'Failed to fetch incident logs'}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)