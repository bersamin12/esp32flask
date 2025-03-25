from flask import Flask, request, jsonify
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import json
import time
from datetime import datetime

# Initialize Flask app
app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})
app.config['SECRET_KEY'] = 'your-secret-key'

# Initialize SocketIO for frontend connections only
socketio = SocketIO(app, cors_allowed_origins="*")

# Store recent sensor data
sensor_data_history = []
MAX_HISTORY_SIZE = 100

@app.route('/')
def index():
    return "ESP32 Sensor Backend Running!"

# Simple HTTP endpoint for ESP32 to post data
@app.route('/api/sensor', methods=['POST'])
def receive_sensor_data():
    try:
        data = request.json
        print(f"Received HTTP data: {data}")
        
        # Add server-side timestamp
        data['server_timestamp'] = datetime.now().isoformat()
        
        # Add to history
        sensor_data_history.append(data)
        
        # Keep history size limited
        while len(sensor_data_history) > MAX_HISTORY_SIZE:
            sensor_data_history.pop(0)
        
        # Broadcast to all connected Socket.IO clients
        socketio.emit('sensor_data', data)
        
        return jsonify({"status": "success"}), 200
    except Exception as e:
        print(f"Error in /api/sensor: {e}")
        return jsonify({"status": "error", "message": str(e)}), 400

# Socket.IO for frontend connections
@socketio.on('connect')
def handle_connect():
    print('Web client connected')
    # Send all historical data on connection
    emit('sensor_history', sensor_data_history)

@socketio.on('disconnect')
def handle_disconnect():
    print('Web client disconnected')

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5005, debug=True)