
import socket
import struct
import threading
import time
from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

# Shared state
state = {
    "ecu_online": False,
    "dtcs": [],
    "log": [],
    "uptime": 0,
    "last_seen": 0,
}

DTC_MAP = {
    0x010101: {"name": "Brake Sensor Loss",    "sev": "HIGH",   "event": 1},
    0x010201: {"name": "Door Lock Failure",     "sev": "HIGH",   "event": 2},
    0x010301: {"name": "Over Temperature",      "sev": "MED",    "event": 3},
    0x010401: {"name": "Motor Overcurrent",     "sev": "HIGH",   "event": 4},
    0x010501: {"name": "CAN Timeout",           "sev": "MED",    "event": 5},
    0x010601: {"name": "Signal Loss",           "sev": "MED",    "event": 6},
    0x010701: {"name": "HVAC Failure",          "sev": "LOW",    "event": 7},
    0x010801: {"name": "Power Undervoltage",    "sev": "HIGH",   "event": 8},
    0x010901: {"name": "Brake Pressure Low",    "sev": "HIGH",   "event": 9},
    0x010A01: {"name": "Door Open At Speed",    "sev": "HIGH",   "event": 10},
    0x010B01: {"name": "Motor Stall",           "sev": "MED",    "event": 11},
    0x010C01: {"name": "CAN Bus-Off",           "sev": "HIGH",   "event": 12},
    0x010D01: {"name": "Ethernet Link Down",    "sev": "MED",    "event": 13},
    0x010E01: {"name": "Ethernet Timeout",      "sev": "MED",    "event": 14},
    0x010F01: {"name": "Speed Sensor Fail",     "sev": "HIGH",   "event": 15},
}

def add_log(msg):
    ts = time.strftime("%H:%M:%S")
    state["log"].insert(0, {"ts": ts, "msg": msg})
    if len(state["log"]) > 30:
        state["log"].pop()

def send_uds(sock, mcast_addr, data):
    # Build ISO-TP Single Frame CAN-UDP packet
    # Format: CAN_ID(4) + DLC(1) + DATA(8) = 13 bytes
    dlc = len(data) + 1
    payload = bytes([dlc & 0xFF, 0, 0, 0])  # CAN ID 0x7DF little-endian
    payload = struct.pack("!I", 0x7DF)       # CAN ID big-endian
    payload += bytes([dlc])                  # DLC
    frame_data = bytes([len(data)]) + bytes(data) + bytes(8 - len(data))
    payload += frame_data[:8]
    sock.sendto(payload, mcast_addr)

def parse_uds_response(data):
    # data is raw bytes from UDP CAN frame
    # Format: CAN_ID(4) + DLC(1) + DATA(8)
    if len(data) < 13:
        return None
    can_id = struct.unpack("!I", data[0:4])[0]
    if can_id != 0x7E8 and can_id != 0x000007E8:
        return None
    dlc  = data[4]
    payload = data[5:5+min(dlc,8)]
    return payload

def udp_listener():
    MCAST = "239.0.0.1"
    PORT  = 5555
    sock  = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.bind(("", PORT))
    mreq = struct.pack("4sL", socket.inet_aton(MCAST), socket.INADDR_ANY)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    sock.settimeout(2.0)

    mcast_addr = (MCAST, PORT)
    add_log("[Bridge] Listening on " + MCAST + ":" + str(PORT))

    while True:
        try:
            # Send UDS 0x19 0x02 ReadDTC request every 2 seconds
            req = struct.pack("<IB8s", 0x7DF, 4,
                              bytes([0x03, 0x19, 0x02, 0xFF, 0, 0, 0, 0]))
            sock.sendto(req, mcast_addr)

            # Collect responses for 1 second
            found_dtcs = []
            deadline = time.time() + 1.0
            full_response = bytearray()
            total_len = 0
            got_ff = False

            while time.time() < deadline:
                try:
                    raw, addr = sock.recvfrom(1024)
                    if len(raw) < 13:
                        continue
                    can_id = struct.unpack("<I", raw[0:4])[0]
                    if can_id != 0x7E8 and can_id != 0x000007E8:
                        continue

                    dlc     = raw[4]
                    payload = raw[5:5+min(dlc,8)]
                    pci     = payload[0] & 0xF0

                    if pci == 0x00:  # Single Frame
                        sf_len = payload[0] & 0x0F
                        full_response = bytearray(payload[1:1+sf_len])
                        total_len = sf_len
                        got_ff = False
                        break

                    elif pci == 0x10:  # First Frame
                        total_len = ((payload[0] & 0x0F) << 8) | payload[1]
                        full_response = bytearray(payload[2:])
                        got_ff = True
                        # Send Flow Control
                        fc = struct.pack("<IB8s", 0x7DF, 3,
                                         bytes([0x30, 0x00, 0x00, 0,0,0,0,0]))
                        sock.sendto(fc, mcast_addr)

                    elif pci == 0x20 and got_ff:  # Consecutive Frame
                        full_response += bytearray(payload[1:])
                        if len(full_response) >= total_len:
                            break

                except socket.timeout:
                    break

            # Parse response: 59 02 mask [DTC3 STATUS] ...
            if len(full_response) >= 3 and full_response[0] == 0x59:
                state["ecu_online"] = True
                state["last_seen"]  = time.time()
                idx = 3
                new_dtcs = []
                while idx + 3 < len(full_response):
                    dtc = (full_response[idx] << 16) |                           (full_response[idx+1] << 8) |                            full_response[idx+2]
                    uds_status = full_response[idx+3]
                    idx += 4
                    info = DTC_MAP.get(dtc, {"name": "Unknown", "sev": "UNK"})
                    new_dtcs.append({
                        "dtc":    hex(dtc),
                        "name":   info["name"],
                        "sev":    info["sev"],
                        "status": uds_status,
                        "status_hex": hex(uds_status),
                        "failed": bool(uds_status & 0x01),
                        "confirmed": bool(uds_status & 0x08),
                    })
                if new_dtcs != state["dtcs"]:
                    state["dtcs"] = new_dtcs
                    add_log(f"[UDS] 0x19 response: {len(new_dtcs)} DTCs")

        except socket.timeout:
            if time.time() - state["last_seen"] > 5:
                if state["ecu_online"]:
                    add_log("[Bridge] ECU offline - no response")
                state["ecu_online"] = False
        except Exception as e:
            add_log(f"[Bridge] Error: {str(e)}")
            time.sleep(1)

        time.sleep(1)

TEMPLATE = """<!DOCTYPE html>
<html>
<head>
<title>Railway DEM Dashboard</title>
<meta http-equiv="refresh" content="2">
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:monospace;background:#0d0d0d;color:#e0e0e0;padding:20px}
h1{font-size:18px;color:#00ccff;margin-bottom:16px;letter-spacing:2px}
.topbar{display:flex;align-items:center;gap:16px;margin-bottom:20px}
.badge{font-size:11px;padding:3px 10px;border-radius:3px;font-weight:bold}
.online{background:#0a3a0a;color:#00ff88;border:1px solid #00ff88}
.offline{background:#3a0a0a;color:#ff4444;border:1px solid #ff4444}
.metrics{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:20px}
.metric{background:#1a1a1a;border:1px solid #333;padding:14px;border-radius:4px}
.metric .label{font-size:11px;color:#888;margin-bottom:6px}
.metric .value{font-size:28px;font-weight:bold}
.red{color:#ff4444}.green{color:#00ff88}.amber{color:#ffaa00}.cyan{color:#00ccff}
table{width:100%;border-collapse:collapse;margin-bottom:20px;font-size:13px}
th{background:#1a1a1a;padding:8px 12px;text-align:left;color:#888;
   border-bottom:1px solid #333;font-size:11px;text-transform:uppercase}
td{padding:8px 12px;border-bottom:1px solid #1e1e1e}
tr:hover td{background:#1a1a1a}
.sev-HIGH{color:#ff4444;font-weight:bold}
.sev-MED{color:#ffaa00}
.sev-LOW{color:#00ff88}
.failed{color:#ff4444;font-weight:bold}
.passed{color:#00ff88}
.log{background:#0a0a0a;border:1px solid #222;padding:12px;
     font-size:12px;height:150px;overflow-y:auto;border-radius:4px}
.log-line{padding:2px 0;border-bottom:1px solid #111;color:#888}
.section-title{font-size:12px;color:#888;text-transform:uppercase;
               letter-spacing:1px;margin-bottom:8px}
</style>
</head>
<body>
<div class="topbar">
  <h1>Railway DEM/DCM Dashboard</h1>
  <span class="badge {{ 'online' if ecu_online else 'offline' }}">
    ECU {{ 'ONLINE' if ecu_online else 'OFFLINE' }}
  </span>
</div>
<div class="metrics">
  <div class="metric"><div class="label">Active DTCs</div>
    <div class="value {{ 'red' if active > 0 else 'green' }}">{{ active }}</div></div>
  <div class="metric"><div class="label">Confirmed DTCs</div>
    <div class="value {{ 'amber' if confirmed > 0 else 'green' }}">{{ confirmed }}</div></div>
  <div class="metric"><div class="label">Events Monitored</div>
    <div class="value cyan">15</div></div>
  <div class="metric"><div class="label">Transport</div>
    <div class="value cyan" style="font-size:14px;margin-top:6px">CAN-UDP<br>239.0.0.1:5555</div></div>
</div>
<div class="section-title">DTC Event Memory</div>
<table>
  <tr><th>DTC Code</th><th>Event Name</th><th>Severity</th>
      <th>Status</th><th>UDS Byte</th><th>Confirmed</th></tr>
  {% for d in dtcs %}
  <tr>
    <td style="font-family:monospace;color:#00ccff">{{ d.dtc }}</td>
    <td>{{ d.name }}</td>
    <td class="sev-{{ d.sev }}">{{ d.sev }}</td>
    <td class="{{ 'failed' if d.failed else 'passed' }}">
      {{ 'FAILED' if d.failed else 'PASSED' }}</td>
    <td style="color:#888;font-family:monospace">{{ d.status_hex }}</td>
    <td class="{{ 'amber' if d.confirmed else 'passed' }}">
      {{ 'YES' if d.confirmed else 'NO' }}</td>
  </tr>
  {% endfor %}
  {% if not dtcs %}
  <tr><td colspan="6" style="text-align:center;color:#444;padding:20px">
    No active DTCs</td></tr>
  {% endif %}
</table>
<div class="section-title">System Log</div>
<div class="log">
  {% for l in log %}
  <div class="log-line">
    <span style="color:#444">{{ l.ts }}</span>
    <span style="margin-left:8px">{{ l.msg }}</span>
  </div>
  {% endfor %}
</div>
</body>
</html>"""

@app.route("/")
def index():
    active    = sum(1 for d in state["dtcs"] if d["failed"])
    confirmed = sum(1 for d in state["dtcs"] if d["confirmed"])
    return render_template_string(
        TEMPLATE,
        ecu_online = state["ecu_online"],
        dtcs       = state["dtcs"],
        log        = state["log"],
        active     = active,
        confirmed  = confirmed,
    )

@app.route("/api/dtcs")
def api_dtcs():
    return jsonify(state)

if __name__ == "__main__":
    t = threading.Thread(target=udp_listener, daemon=True)
    t.start()
    print("Dashboard running at http://localhost:5000")
    print("Start your ECU: cd ~/railway-dem/build && ./rail_ecu")
    app.run(host="0.0.0.0", port=5000, debug=False)
