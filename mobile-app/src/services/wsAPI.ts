/**
 * WebSocket API for DrainGuard — real-time motor and servo control
 * Port 81 — persistent connection, no reconnect overhead per command
 */

const WS_URL = 'ws://192.168.4.1:81';
const RECONNECT_DELAY = 2000;

type MessageHandler = (data: Record<string, unknown>) => void;

class WSAPIService {
  private ws: WebSocket | null = null;
  private connected = false;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private messageHandlers: MessageHandler[] = [];
  private connecting = false;

  connect() {
    if (this.connected || this.connecting) return;
    this.connecting = true;
    try {
      this.ws = new WebSocket(WS_URL);

      this.ws.onopen = () => {
        this.connected = true;
        this.connecting = false;
        if (this.reconnectTimer) {
          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = null;
        }
        console.log('[WS] Connected');
      };

      this.ws.onclose = () => {
        this.connected = false;
        this.connecting = false;
        this.ws = null;
        console.log('[WS] Disconnected — reconnecting...');
        this.reconnectTimer = setTimeout(() => this.connect(), RECONNECT_DELAY);
      };

      this.ws.onerror = () => {
        this.connected = false;
        this.connecting = false;
        this.ws?.close();
      };

      this.ws.onmessage = (e) => {
        try {
          const data = JSON.parse(e.data) as Record<string, unknown>;
          this.messageHandlers.forEach(h => h(data));
        } catch {}
      };
    } catch {
      this.connecting = false;
    }
  }

  disconnect() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.ws?.close();
    this.ws = null;
    this.connected = false;
  }

  isConnected() {
    return this.connected;
  }

  onMessage(handler: MessageHandler) {
    this.messageHandlers.push(handler);
    return () => {
      this.messageHandlers = this.messageHandlers.filter(h => h !== handler);
    };
  }

  private send(data: object) {
    if (!this.connected || !this.ws) {
      // Try to connect and retry once
      this.connect();
      return;
    }
    this.ws.send(JSON.stringify(data));
  }

  // ── Motor commands ───────────────────────────────────────────────────────

  motorForward()  { this.send({cmd: 'motor', dir: 'forward'}); }
  motorBackward() { this.send({cmd: 'motor', dir: 'backward'}); }
  motorLeft()     { this.send({cmd: 'motor', dir: 'left'}); }
  motorRight()    { this.send({cmd: 'motor', dir: 'right'}); }
  motorStop()     { this.send({cmd: 'motor', dir: 'stop'}); }

  // ── Servo commands — dir: 1=fwd, -1=rev ────────────────────────────────

  moveBase(dir: number)     { this.send({cmd: 'servo', joint: 'base',     dir: dir > 0 ? 'fwd' : 'rev'}); }
  moveShoulder(dir: number) { this.send({cmd: 'servo', joint: 'shoulder', dir: dir > 0 ? 'fwd' : 'rev'}); }
  moveElbow(dir: number)    { this.send({cmd: 'servo', joint: 'elbow',    dir: dir > 0 ? 'fwd' : 'rev'}); }
  moveGripper(dir: number)  { this.send({cmd: 'servo', joint: 'gripper',  dir: dir > 0 ? 'fwd' : 'rev'}); }

  // ── Arm sequence ─────────────────────────────────────────────────────────

  armOpen()  { this.send({cmd: 'arm', action: 'open'}); }
  armClose() { this.send({cmd: 'arm', action: 'close'}); }
  armHome()  { this.send({cmd: 'arm', action: 'home'}); }

  // ── Status ───────────────────────────────────────────────────────────────

  getStatus() { this.send({cmd: 'get_status'}); }
}

export const wsAPI = new WSAPIService();
export default wsAPI;
