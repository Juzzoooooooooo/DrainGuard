/**
 * WiFi Connection Screen (Hotspot-Only Version)
 * Shows instructions to connect to ESP32 hotspot
 */

import React, {useState, useEffect} from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ActivityIndicator,
  Alert,
} from 'react-native';
import httpAPI from '../services/httpAPI';

interface WiFiConnectionProps {
  onConnected: () => void;
}

export const WiFiConnection: React.FC<WiFiConnectionProps> = ({onConnected}) => {
  const [testing, setTesting] = useState(false);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    // Auto-test connection every 3 seconds
    const interval = setInterval(() => {
      if (!testing && !connected) {
        testConnection();
      }
    }, 3000);

    return () => clearInterval(interval);
  }, [testing, connected]);

  const testConnection = async () => {
    setTesting(true);
    try {
      const isConnected = await httpAPI.testConnection();
      if (isConnected) {
        setConnected(true);
        onConnected();
      }
    } catch (error) {
      // Connection failed - stay on this screen
    } finally {
      setTesting(false);
    }
  };

  const handleManualTest = async () => {
    setTesting(true);
    try {
      const isConnected = await httpAPI.testConnection();
      if (isConnected) {
        setConnected(true);
        Alert.alert('Connected!', 'Successfully connected to DrainGuard');
        onConnected();
      } else {
        Alert.alert(
          'Not Connected',
          'Cannot reach DrainGuard. Make sure you are connected to the DrainGuard WiFi hotspot.'
        );
      }
    } catch (error) {
      Alert.alert(
        'Connection Error',
        'Cannot reach DrainGuard. Check your WiFi connection.'
      );
    } finally {
      setTesting(false);
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>📡 Connect to DrainGuard</Text>
        
        <View style={styles.instructions}>
          <Text style={styles.step}>Step 1: Open WiFi Settings</Text>
          <Text style={styles.detail}>
            Go to your phone's WiFi settings
          </Text>

          <Text style={styles.step}>Step 2: Connect to Hotspot</Text>
          <Text style={styles.detail}>
            WiFi Network: <Text style={styles.bold}>DrainGuard-Robot</Text>
          </Text>
          <Text style={styles.detail}>
            Password: <Text style={styles.bold}>DrainGuard123</Text>
          </Text>

          <Text style={styles.step}>Step 3: Return to App</Text>
          <Text style={styles.detail}>
            The app will automatically detect the connection
          </Text>
        </View>

        {testing && (
          <View style={styles.testingContainer}>
            <ActivityIndicator size="large" color="#4CAF50" />
            <Text style={styles.testingText}>Testing connection...</Text>
          </View>
        )}

        {!connected && !testing && (
          <TouchableOpacity 
            style={styles.testButton} 
            onPress={handleManualTest}
          >
            <Text style={styles.testButtonText}>Test Connection</Text>
          </TouchableOpacity>
        )}

        {connected && (
          <View style={styles.connectedContainer}>
            <Text style={styles.connectedText}>✅ Connected!</Text>
          </View>
        )}
      </View>

      <View style={styles.infoCard}>
        <Text style={styles.infoTitle}>ℹ️ About This App</Text>
        <Text style={styles.infoText}>
          This app controls DrainGuard over WiFi. No Bluetooth required!
        </Text>
        <Text style={styles.infoText}>
          Make sure your phone stays connected to the DrainGuard WiFi network
          while using the app.
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 20,
    justifyContent: 'center',
  },
  card: {
    backgroundColor: 'white',
    borderRadius: 12,
    padding: 24,
    marginBottom: 16,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 20,
    textAlign: 'center',
  },
  instructions: {
    marginBottom: 24,
  },
  step: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#4CAF50',
    marginTop: 16,
    marginBottom: 8,
  },
  detail: {
    fontSize: 14,
    color: '#666',
    marginBottom: 4,
    paddingLeft: 8,
  },
  bold: {
    fontWeight: 'bold',
    color: '#333',
  },
  testButton: {
    backgroundColor: '#4CAF50',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
  },
  testButtonText: {
    color: 'white',
    fontSize: 16,
    fontWeight: 'bold',
  },
  testingContainer: {
    alignItems: 'center',
    padding: 16,
  },
  testingText: {
    marginTop: 12,
    fontSize: 14,
    color: '#666',
  },
  connectedContainer: {
    padding: 16,
    backgroundColor: '#E8F5E9',
    borderRadius: 8,
    alignItems: 'center',
  },
  connectedText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  infoCard: {
    backgroundColor: '#E3F2FD',
    borderRadius: 12,
    padding: 20,
  },
  infoTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#1976D2',
    marginBottom: 12,
  },
  infoText: {
    fontSize: 14,
    color: '#555',
    marginBottom: 8,
    lineHeight: 20,
  },
});

export default WiFiConnection;
