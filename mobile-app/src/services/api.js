import axios from 'axios';
import AsyncStorage from '@react-native-async-storage/async-storage';

// Default device IP - can be changed in settings
let DEVICE_IP = '192.168.1.100';

// Load device IP from storage
AsyncStorage.getItem('deviceIP').then((ip) => {
  if (ip) {
    DEVICE_IP = ip;
  }
});

const API_BASE_URL = `http://${DEVICE_IP}`;
const API_TIMEOUT = 5000;

const apiClient = axios.create({
  baseURL: API_BASE_URL,
  timeout: API_TIMEOUT,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Update API base URL when device IP changes
export const updateDeviceIP = async (ip) => {
  DEVICE_IP = ip;
  apiClient.defaults.baseURL = `http://${ip}`;
  await AsyncStorage.setItem('deviceIP', ip);
};

// Get device status
export const getDeviceStatus = async () => {
  try {
    const response = await apiClient.get('/api/status');
    return response.data;
  } catch (error) {
    console.error('Failed to get device status:', error);
    throw error;
  }
};

// Control drain
export const controlDrain = async (action) => {
  try {
    const endpoint = action === 'open' ? '/api/drain/open' : '/api/drain/close';
    const response = await apiClient.post(endpoint);
    return response.data;
  } catch (error) {
    console.error(`Failed to ${action} drain:`, error);
    throw error;
  }
};

// Get GPS location
export const getGPSLocation = async () => {
  try {
    const response = await apiClient.get('/api/gps');
    return response.data;
  } catch (error) {
    console.error('Failed to get GPS location:', error);
    throw error;
  }
};

// Get camera stream URL
export const getCameraStreamURL = async () => {
  try {
    const response = await apiClient.get('/api/camera/stream');
    return response.data.stream_url;
  } catch (error) {
    console.error('Failed to get camera stream URL:', error);
    // Return fallback URL
    return `http://${DEVICE_IP}:81/stream`;
  }
};

// Get historical data
export const getHistory = async (limit = 50) => {
  try {
    const response = await apiClient.get('/api/history', {
      params: { limit },
    });
    return response.data;
  } catch (error) {
    console.error('Failed to get history:', error);
    throw error;
  }
};

// Send command via SMS (backend API)
export const sendSMSCommand = async (command) => {
  try {
    const response = await apiClient.post('/api/sms/command', {
      command,
    });
    return response.data;
  } catch (error) {
    console.error('Failed to send SMS command:', error);
    throw error;
  }
};

export default {
  getDeviceStatus,
  controlDrain,
  getGPSLocation,
  getCameraStreamURL,
  getHistory,
  sendSMSCommand,
  updateDeviceIP,
};
