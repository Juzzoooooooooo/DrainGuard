/**
 * Push Notification Service for DrainGuard
 * Handles local push notifications when alerts are received via WebSocket
 */

import {Platform} from 'react-native';
import PushNotification, {Importance} from 'react-native-push-notification';

export type NotificationLevel = 'info' | 'warning' | 'critical';

class NotificationService {
  private initialized = false;

  /**
   * Initialize notification service — must be called once at app startup
   */
  initialize() {
    if (this.initialized) return;

    PushNotification.configure({
      onNotification: (notification) => {
        console.log('[Notification] Received:', notification);
      },
      permissions: {
        alert: true,
        badge: true,
        sound: true,
      },
      popInitialNotification: true,
      requestPermissions: Platform.OS === 'ios',
    });

    // Create notification channel for Android
    if (Platform.OS === 'android') {
      PushNotification.createChannel(
        {
          channelId: 'drainguard-alerts',
          channelName: 'DrainGuard Alerts',
          channelDescription: 'Critical water level alerts',
          importance: Importance.HIGH,
          vibrate: true,
          playSound: true,
          soundName: 'default',
        },
        (created) => console.log(`[Notification] Channel created: ${created}`),
      );
    }

    this.initialized = true;
    console.log('[Notification] Service initialized');
  }

  /**
   * Show local push notification
   */
  showAlert(
    title: string,
    message: string,
    level: NotificationLevel = 'info',
  ) {
    if (!this.initialized) {
      console.warn('[Notification] Service not initialized');
      return;
    }

    const priority = level === 'critical' ? 'high' : level === 'warning' ? 'default' : 'low';

    PushNotification.localNotification({
      channelId: 'drainguard-alerts',
      title,
      message,
      playSound: level === 'critical',
      soundName: 'default',
      priority,
      importance: level === 'critical' ? 'high' : 'default',
      vibrate: level === 'critical',
      vibration: level === 'critical' ? 300 : 0,
    });

    console.log(`[Notification] Sent: ${title} - ${message}`);
  }

  /**
   * Request notification permissions (iOS)
   */
  async requestPermissions(): Promise<boolean> {
    if (Platform.OS !== 'ios') return true;

    return new Promise((resolve) => {
      PushNotification.requestPermissions(['alert', 'badge', 'sound']).then(
        (permissions) => {
          console.log('[Notification] iOS permissions:', permissions);
          resolve(permissions.alert === true);
        },
      );
    });
  }

  /**
   * Cancel all notifications
   */
  cancelAll() {
    PushNotification.cancelAllLocalNotifications();
  }
}

export const notificationService = new NotificationService();
export default notificationService;
