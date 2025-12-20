from flask import Flask, request, jsonify, render_template_string
import base64

app = Flask(__name__)
store = {}

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

@app.get("/")
def index():
    html = """<h1>EcoWatt Cloud</h1><ul>
    {% for k, rec in store.items() %}
      {% if k is sequence and k|length == 2 %}
        <li><a href="/view/{{k[0]}}/{{k[1]}}">{{k[0]}} #{{k[1]}}</a></li>
      {% else %}
        <li>{{k}} (non-uplink entry)</li>
      {% endif %}
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

# --------------------------------------------------------------------
# Remote Configuration (Part 1)
# --------------------------------------------------------------------

@app.post("/api/config-update")
def config_update():
    """Cloud → Device: queue a configuration update"""
    try:
        j = request.get_json(force=True)
    except Exception as e:
        return jsonify({"status": "error", "error": str(e)}), 400

    dev = j.get("deviceId", "unknown")
    cfg = j.get("config_update", {})
    if not cfg:
        return jsonify({"status": "error", "error": "Missing config_update"}), 400

    store[f"cfg_{dev}"] = cfg
    print(f"[CONFIG-UPDATE] queued for {dev}: {cfg}")
    return jsonify({"status": "queued", "deviceId": dev, "config": cfg})


@app.get("/api/downlink")
def downlink():
    """Device → Cloud: ESP polls for configuration updates"""
    dev = request.args.get("deviceId", "unknown")
    cfg = store.pop(f"cfg_{dev}", None)
    return jsonify({"pending": bool(cfg), "config_update": cfg or {}})


@app.post("/api/config-ack")
def config_ack():
    """Device → Cloud: ESP acknowledges configuration applied"""
    try:
        j = request.get_json(force=True)
    except Exception as e:
        return jsonify({"status": "error", "error": str(e)}), 400

    dev = j.get("deviceId", "unknown")
    ack = j.get("config_ack", {})
    store[f"ack_{dev}"] = ack
    print(f"[CONFIG-ACK] from {dev}: {ack}")
    return jsonify({"status": "ok", "ack_received": True})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
