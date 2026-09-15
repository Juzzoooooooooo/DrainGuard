import {bleProvisioning, DrainGuardProvisioningStatus} from './bleProvisioning';
import type {ServoPositions, SystemStatus} from '../types';

const STATUS_TIMEOUT_MS = 8000;
const COMMAND_TIMEOUT_MS = 5000;
let nextRequestId = 1;

function createRequestId() {
  const requestId = nextRequestId;
  nextRequestId = nextRequestId >= 2_000_000_000 ? 1 : nextRequestId + 1;
  return requestId;
}

function finiteNumber(value: unknown) {
  return typeof value === 'number' && Number.isFinite(value) ? value : null;
}

export function parseControllerStatus(
  status: DrainGuardProvisioningStatus,
): SystemStatus | null {
  const waterLevel = finiteNumber(status.wl);
  const distance = finiteNumber(status.d);
  const latitude = finiteNumber(status.lat);
  const longitude = finiteNumber(status.lon);
  const satellites = finiteNumber(status.sat);

  if (
    status.status !== 'controller_status' ||
    waterLevel === null ||
    distance === null ||
    typeof status.o !== 'boolean' ||
    latitude === null ||
    longitude === null ||
    satellites === null
  ) {
    return null;
  }

  return {
    water_level: waterLevel,
    distance,
    drain_open: status.o,
    latitude,
    longitude,
    satellites,
  };
}

function waitForResponse<T>(
  requestId: number,
  timeoutMs: number,
  readResponse: (status: DrainGuardProvisioningStatus) => T | null,
  send: () => Promise<void>,
) {
  return new Promise<T>((resolve, reject) => {
    let settled = false;
    let timeout: ReturnType<typeof setTimeout>;
    let subscription: {remove: () => void};
    const finish = (callback: () => void) => {
      if (settled) {
        return;
      }
      settled = true;
      clearTimeout(timeout);
      subscription.remove();
      callback();
    };
    subscription = bleProvisioning.onStatus(status => {
      if (status.id !== requestId) {
        return;
      }
      try {
        const result = readResponse(status);
        if (result !== null) {
          finish(() => resolve(result));
        }
      } catch (error) {
        finish(() => reject(error));
      }
    });
    timeout = setTimeout(
      () =>
        finish(() =>
          reject(
            new Error(
              'DrainGuard did not answer the Bluetooth command. Update the ESP32 firmware and reconnect.',
            ),
          ),
        ),
      timeoutMs,
    );

    send().catch(error => finish(() => reject(error)));
  });
}

function waitForCommand(requestId: number, send: () => Promise<void>) {
  return waitForResponse(
    requestId,
    COMMAND_TIMEOUT_MS,
    status => {
      if (
        status.status !== 'command_result' ||
        typeof status.ok !== 'boolean'
      ) {
        return null;
      }
      if (!status.ok) {
        throw new Error(status.message ?? 'DrainGuard rejected the command.');
      }
      return true;
    },
    send,
  ).then(() => undefined);
}

export const bleController = {
  isConnected() {
    return bleProvisioning.isConnected();
  },

  reconnectLast() {
    return bleProvisioning.reconnectLast();
  },

  onConnectionState(listener: (state: string) => void) {
    return bleProvisioning.onState(event => listener(event.state));
  },

  getSystemStatus() {
    const requestId = createRequestId();
    return waitForResponse(
      requestId,
      STATUS_TIMEOUT_MS,
      parseControllerStatus,
      () => bleProvisioning.getControllerStatus(requestId),
    );
  },

  controlArm(action: 'open' | 'close') {
    const requestId = createRequestId();
    return waitForCommand(requestId, () =>
      bleProvisioning.controlArm(action, requestId),
    );
  },

  controlServo(servo: keyof ServoPositions, position: number) {
    const requestId = createRequestId();
    return waitForCommand(requestId, () =>
      bleProvisioning.controlServo(servo, Math.round(position), requestId),
    );
  },
};
