import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ActivityIndicator,
  Alert,
  TouchableOpacity,
} from 'react-native';
import { WebView } from 'react-native-webview';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { getCameraStreamURL } from '../services/api';

export default function LiveStreamScreen() {
  const [streamUrl, setStreamUrl] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(false);

  useEffect(() => {
    loadStreamURL();
  }, []);

  const loadStreamURL = async () => {
    try {
      const url = await getCameraStreamURL();
      setStreamUrl(url);
      setLoading(false);
      setError(false);
    } catch (err) {
      console.error('Failed to load stream URL:', err);
      setError(true);
      setLoading(false);
      Alert.alert('Error', 'Failed to connect to camera');
    }
  };

  const handleRefresh = () => {
    setLoading(true);
    setError(false);
    loadStreamURL();
  };

  if (loading) {
    return (
      <View style={styles.centerContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.statusText}>Connecting to camera...</Text>
      </View>
    );
  }

  if (error) {
    return (
      <View style={styles.centerContainer}>
        <Icon name="videocam-off" size={64} color="#999" />
        <Text style={styles.errorText}>Camera not available</Text>
        <TouchableOpacity style={styles.retryButton} onPress={handleRefresh}>
          <Icon name="refresh" size={24} color="#fff" />
          <Text style={styles.retryButtonText}>Retry</Text>
        </TouchableOpacity>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.streamContainer}>
        <WebView
          source={{ uri: streamUrl }}
          style={styles.webview}
          onError={() => setError(true)}
          startInLoadingState={true}
          renderLoading={() => (
            <ActivityIndicator
              size="large"
              color="#2196F3"
              style={styles.loader}
            />
          )}
        />
      </View>
      <View style={styles.controlsContainer}>
        <TouchableOpacity style={styles.controlButton} onPress={handleRefresh}>
          <Icon name="refresh" size={24} color="#2196F3" />
          <Text style={styles.controlButtonText}>Refresh</Text>
        </TouchableOpacity>
        <View style={styles.statusIndicator}>
          <View style={styles.liveIndicator} />
          <Text style={styles.liveText}>LIVE</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000',
  },
  centerContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#f5f5f5',
  },
  statusText: {
    marginTop: 10,
    fontSize: 16,
    color: '#666',
  },
  errorText: {
    marginTop: 10,
    fontSize: 18,
    color: '#999',
  },
  streamContainer: {
    flex: 1,
  },
  webview: {
    flex: 1,
  },
  loader: {
    position: 'absolute',
    top: '50%',
    left: '50%',
    marginLeft: -20,
    marginTop: -20,
  },
  controlsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#fff',
    padding: 10,
  },
  controlButton: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 10,
    backgroundColor: '#e3f2fd',
    borderRadius: 8,
  },
  controlButtonText: {
    marginLeft: 5,
    fontSize: 16,
    color: '#2196F3',
    fontWeight: 'bold',
  },
  statusIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  liveIndicator: {
    width: 12,
    height: 12,
    borderRadius: 6,
    backgroundColor: '#f44336',
    marginRight: 5,
  },
  liveText: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#f44336',
  },
  retryButton: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 20,
    padding: 15,
    backgroundColor: '#2196F3',
    borderRadius: 8,
  },
  retryButtonText: {
    marginLeft: 5,
    fontSize: 16,
    color: '#fff',
    fontWeight: 'bold',
  },
});
