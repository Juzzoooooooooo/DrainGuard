import {
  getBaseUrl,
  isValidDeviceAddress,
  normalizeDeviceAddress,
  parseSystemStatus,
} from '../src/services/api';

describe('device address helpers', () => {
  it('normalizes addresses entered as either an IP or URL', () => {
    expect(normalizeDeviceAddress(' 192.168.4.1/ ')).toBe('192.168.4.1');
    expect(normalizeDeviceAddress('http://drain.local/')).toBe('drain.local');
  });

  it('builds the local HTTP base URL used by the firmware', () => {
    expect(getBaseUrl('192.168.4.1')).toBe('http://192.168.4.1');
  });

  it('rejects addresses containing paths or invalid ports', () => {
    expect(isValidDeviceAddress('192.168.4.1')).toBe(true);
    expect(isValidDeviceAddress('drain.local:8080')).toBe(true);
    expect(isValidDeviceAddress('drain.local/api')).toBe(false);
    expect(isValidDeviceAddress('drain.local:70000')).toBe(false);
  });

  it('validates status payloads before the UI uses them', () => {
    const status = {
      water_level: 80,
      distance: 120,
      drain_open: false,
      latitude: 0,
      longitude: 0,
      satellites: 0,
    };

    expect(parseSystemStatus(status)).toEqual(status);
    expect(() => parseSystemStatus({...status, distance: '120'})).toThrow(
      'invalid status response',
    );
  });
});
