import threading
from flask import Flask, request, jsonify, render_template_string, Response, make_response
import base64
import os
import time

app = Flask(__name__)
store = {}

should_send_update = False
FIRMWARE_PATH = "print_a.ino.bin" 

def u16be(b0, b1):
    return ((b0 & 0xFF) << 8) | (b1 & 0xFF)

def s16be(b0, b1):
    v = ((b0 & 0xFF) << 8) | (b1 & 0xFF)
    return v - 0x10000 if v & 0x8000 else v

def decompress_delta16(blob: bytes):
    if len(blob) < 2:
        return []
    out = []
    i = 0
    prev = u16be(blob[i], blob[i + 1])
    i += 2
    out.append(prev)
    while i + 1 < len(blob):
        d = s16be(blob[i], blob[i + 1])
        i += 2
        prev = (prev + d) & 0xFFFF
        out.append(prev)
    return out

def decompress_ts16(blob: bytes, regs: int = 10):
    """Decompress TimeSeries16 format to match ESP TimeSeriesCompressor."""
    if len(blob) < regs * 2:
        return []

    out = []
    i = 0

    # First frame = absolute values
    prev = []
    for r in range(regs):
        val = u16be(blob[i], blob[i + 1])
        i += 2
        prev.append(val)
        out.append(val)

    # Decode subsequent frames until blob exhausted
    while i < len(blob):
        mask_bytes = (regs + 7) // 8
        if i + mask_bytes > len(blob):
            break

        # Read mask
        mask = []
        for mb in range(mask_bytes):
            mask.append(blob[i])
            i += 1

        curr = []
        reg_index = 0

        while reg_index < regs and i < len(blob):
            mb = reg_index // 8
            bit = 1 << (reg_index % 8)

            if mask[mb] & bit:
                # Deltas packed in 4-bit nibbles
                if i >= len(blob):
                    break
                byte = blob[i]
                i += 1
                for delta4 in [(byte >> 4) & 0x0F, byte & 0x0F]:
                    if reg_index >= regs:
                        break
                    d = (delta4 ^ 0x08) - 0x08  # sign-extend 4-bit
                    val = (prev[reg_index] + d) & 0xFFFF
                    curr.append(val)
                    out.append(val)
                    reg_index += 1
            else:
                # Full absolute value
                if i + 1 >= len(blob):
                    break
                val = u16be(blob[i], blob[i + 1])
                i += 2
                curr.append(val)
                out.append(val)
                reg_index += 1

        if len(curr) == regs:
            prev = curr

    return out

@app.post("/api/uplink")
def uplink():
    try:
        j = request.get_json(force=True)
    except Exception as e:
        return jsonify({"status": "error", "ack": False, "error": str(e)}), 400

    dev = j.get("deviceId", "unknown")
    seq = int(j.get("seqNo", 0))
    idx = int(j.get("chunkIndex", 0))
    cnt = int(j.get("chunkCount", 1))
    algo = j.get("algo", "raw")

    if (dev, seq) not in store:
        store[(dev, seq)] = {
            "meta": {
                "algo": algo,
                "tStart": j.get("tStart", 0),
                "tEnd": j.get("tEnd", 0),
                "chunks": cnt,
            },
            "chunks": [None] * cnt,
        }

    try:
        store[(dev, seq)]["chunks"][idx] = base64.b64decode(j["data"])
    except Exception as e:
        return jsonify({"status": "error", "ack": False, "error": str(e)}), 400

    if all(c is not None for c in store[(dev, seq)]["chunks"]):
        blob = b"".join(store[(dev, seq)]["chunks"])
        samples = []
        if algo == "delta16":
            try:
                samples = decompress_delta16(blob)
            except:
                samples = []
        elif algo == "ts16":
            try:
                samples = decompress_ts16(blob, regs=10)
            except:
                samples = []

        store[(dev, seq)]["meta"]["compressed_len"] = len(blob)
        store[(dev, seq)]["samples"] = samples
        return jsonify({"status": "ok", "ack": True, "complete": True})

    return jsonify({"status": "ok", "ack": True, "complete": False})

@app.route("/api/firmware")
def serve_firmware():
    global should_send_update

    if not should_send_update:
        print("[SERVER] ESP checked — no update available.")
        return make_response("No update", 204)

    should_send_update = False  # reset flag for one-time send

    if not os.path.exists(FIRMWARE_PATH):
        return make_response("Firmware file not found", 404)

    file_size = os.path.getsize(FIRMWARE_PATH)
    print(f"[SERVER] Sending firmware ({file_size} bytes)...")

    def generate():
        chunk_size = 1024
        sent = 0
        with open(FIRMWARE_PATH, "rb") as f:
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                sent += len(chunk)
                yield chunk
                percent = (sent / file_size) * 100
                print(f"\r[SERVER] Progress: {percent:.1f}%", end="", flush=True)
                # optional delay to make printing visible
                time.sleep(0.005)
        print("\n[SERVER] ✅ Firmware fully sent.")

    headers = {
        "Content-Type": "application/octet-stream",
        "Content-Length": str(file_size),
    }

    return Response(generate(), headers=headers)

@app.get("/")
def index():
    html = """<h1>EcoWatt Cloud</h1><ul>
    {% for (dev,seq), rec in store.items() %}
      <li><a href="/view/{{dev}}/{{seq}}">{{dev}} #{{seq}}</a></li>
    {% endfor %}</ul>"""
    return render_template_string(html, store=store)

@app.get("/view/<dev>/<int:seq>")
def view(dev, seq):
    rec = store.get((dev, seq))
    if not rec:
        return "Not found", 404
    meta = rec["meta"]
    samples = rec.get("samples", [])
    html = """
    <h2>{{dev}} #{{seq}}</h2>
    <p>Algo: {{meta.get('algo')}}<br>
    Compressed: {{meta.get('compressed_len',0)}} bytes<br>
    Decompressed: {{samples|length}} samples<br>
    Compression Ratio (approx): 
        {% if samples %}
        {{ "%.2f"|format((samples|length*2) / (meta.get('compressed_len',1))) }}:1
        {% else %}
        N/A
        {% endif %}
    </p>

    <h3>Decompressed Samples by Register</h3>
    <table border="1" cellpadding="4" cellspacing="0">
    <tr>
        <th>Index</th>
        <th>Register</th>
        <th>Value</th>
    </tr>
    {% for v in samples[:60] %}
    <tr>
        <td>{{ loop.index0 }}</td>
        <td>Register {{ loop.index0 % 10 }}</td>
        <td>{{ v }}</td>
    </tr>
    {% endfor %}
    </table>

    <h3>Original Array (Preview up to 60)</h3>
    <pre>{{ samples[:60] }}</pre>

    <p><a href="/">Back</a></p>
    """
    return render_template_string(html, dev=dev, seq=seq, meta=meta, samples=samples)

def console_listener():
    """Listens for 'yes' to trigger the next firmware update."""
    global should_send_update
    print("Type 'yes' to send firmware on next ESP request.")
    while True:
        cmd = input().strip().lower()
        if cmd == "update":
            should_send_update = True
            print("✅ Next ESP request will receive firmware.")
        else:
            print("Ignored command. Type 'yes' to trigger update.")

if __name__ == "__main__":
    threading.Thread(target=console_listener, daemon=True).start()
    app.run(host="0.0.0.0", port=5000, debug=True)
