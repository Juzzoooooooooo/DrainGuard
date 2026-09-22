export interface HotspotStatus {
  enabled: boolean;
  ssid: string;
  ip: string;
  clients: number;
}

export interface HotspotConfig {
  ssid: string;
  password: string;
}
