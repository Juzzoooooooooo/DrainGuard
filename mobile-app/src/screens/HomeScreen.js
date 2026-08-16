import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  RefreshControl,
  ScrollView,
  Alert,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { getDeviceStatus, controlDrain } from '../services/api';

export default function HomeScreen({ navigation }) {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  useEffect(() => {
    loadStatus();
    const interval = setInterval(loadStatus, 5000); // Auto refresh every 5s
    return () => clearInterval(interval);
  }, []);

  const loadStatus = async () => {
    try {
      const data = await getDeviceStatus();
      setStatus(data);
      setLoading(false);
      setRefreshing(false);
    } catch (error) {
      console.error('Failed to load status:', error);
      setLoading(false);
      setRefreshing(false);
      Alert.alert('Error', 'Failed to connect to device');
    }
  };

  const handleDrainControl = async (action) => {
    try {
      Alert.alert(
        'Confirm Action',
        `Are you sure you want to ${action} the drain?`,
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'Confirm',
            onPress: async () => {
              await controlDrain(action);
              setTimeout(loadStatus, 1000);
              Alert.alert('Success', `Drain ${action} command sent`);
            },
          },
        ]
      );
    } catch (error) {
      Alert.alert('Error', `Failed to ${action} drain`);
    }
  };

  const getWaterLevelColor = (level) => {
    if (level > 150) return '#f44336'; // Red - Critical
    if (level > 100) return '#ff9800'; // Orange - Warning
    return '#4caf50'; // Green - Normal
  };

  const getWaterLevelStatus = (level) => {
    if (level > 150) return 'CRITICAL';
    if (level > 100) return 'WARNING';
    return 'NORMAL';
  };

  if (loading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.loadingText}>Connecting to device...</Text>
      </View>
    );
  }

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={loadStatus} />
      }
    >
      {/* Water Level Card */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="water" size={30} color="#2196F3" />
          <Text style={styles.cardTitle}>Water Level</Text>
        </View>
        <View style={styles.waterLevelContainer}>
          <Text
            style={[
              styles.waterLevelValue,
              { color: getWaterLevelColor(status?.water_level || 0) },
            ]}
          >
            {status?.water_level?.toFixed(1) || '0.0'} cm
          </Text>
          <Text
            style={[
              styles.waterLevelStatus,
              { color: getWaterLevelColor(status?.water_level || 0) },
            ]}
          >
            {getWaterLevelStatus(status?.water_level || 0)}
          </Text>
        </View>
        <View style={styles.progressBar}>
          <View
            style={[
              styles.progressFill,
              {
                width: `${Math.min((status?.water_level / 200) * 100, 100)}%`,
                backgroundColor: getWaterLevelColor(status?.water_level || 0),
              },
            ]}
          />
        </View>
        <Text style={styles.cardSubtext}>
          Distance from sensor: {status?.distance?.toFixed(1) || '0.0'} cm
        </Text>
      </View>

      {/* Drain Control Card */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="settings" size={30} color="#2196F3" />
          <Text style={styles.cardTitle}>Drain Control</Text>
        </View>
        <View style={styles.drainStatus}>
          <Text style={styles.drainStatusText}>
            Status: {status?.drain_open ? 'OPEN' : 'CLOSED'}
          </Text>
          <View
            style={[
              styles.drainIndicator,
              {
                backgroundColor: status?.drain_open ? '#4caf50' : '#f44336',
              },
            ]}
          />
        </View>
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.controlButton, styles.openButton]}
            onPress={() => handleDrainControl('open')}
            disabled={status?.drain_open}
          >
            <Icon name="arrow-upward" size={24} color="#fff" />
            <Text style={styles.buttonText}>Open</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.controlButton, styles.closeButton]}
            onPress={() => handleDrainControl('close')}
            disabled={!status?.drain_open}
          >
            <Icon name="arrow-downward" size={24} color="#fff" />
            <Text style={styles.buttonText}>Close</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* GPS Location Card */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Icon name="location-on" size={30} color="#2196F3" />
          <Text style={styles.cardTitle}>GPS Location</Text>
        </View>
        <Text style={styles.gpsText}>
          Latitude: {status?.latitude?.toFixed(6) || 'N/A'}
        </Text>
        <Text style={styles.gpsText}>
          Longitude: {status?.longitude?.toFixed(6) || 'N/A'}
        </Text>
        <Text style={styles.gpsText}>
          Satellites: {status?.satellites || 0}
        </Text>
        <TouchableOpacity
          style={styles.mapButton}
          onPress={() => navigation.navigate('Map', { location: status })}
        >
          <Icon name="map" size={20} color="#2196F3" />
          <Text style={styles.mapButtonText}>View on Map</Text>
        </TouchableOpacity>
      </View>

      {/* Quick Actions */}
      <View style={styles.actionsContainer}>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('LiveStream')}
        >
          <Icon name="videocam" size={32} color="#2196F3" />
          <Text style={styles.actionButtonText}>Live Stream</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('ServoControl')}
        >
          <Icon name="precision-manufacturing" size={32} color="#2196F3" />
          <Text style={styles.actionButtonText}>Servo Arm</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('History')}
        >
          <Icon name="history" size={32} color="#2196F3" />
          <Text style={styles.actionButtonText}>History</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('Settings')}
        >
          <Icon name="settings" size={32} color="#2196F3" />
          <Text style={styles.actionButtonText}>Settings</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#f5f5f5',
  },
  loadingText: {
    marginTop: 10,
    fontSize: 16,
    color: '#666',
  },
  card: {
    backgroundColor: '#fff',
    margin: 10,
    padding: 15,
    borderRadius: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  cardHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 15,
  },
  cardTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    marginLeft: 10,
    color: '#333',
  },
  waterLevelContainer: {
    alignItems: 'center',
    marginVertical: 10,
  },
  waterLevelValue: {
    fontSize: 48,
    fontWeight: 'bold',
  },
  waterLevelStatus: {
    fontSize: 18,
    fontWeight: 'bold',
    marginTop: 5,
  },
  progressBar: {
    height: 20,
    backgroundColor: '#e0e0e0',
    borderRadius: 10,
    overflow: 'hidden',
    marginVertical: 10,
  },
  progressFill: {
    height: '100%',
    borderRadius: 10,
  },
  cardSubtext: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
  },
  drainStatus: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 15,
  },
  drainStatusText: {
    fontSize: 18,
    fontWeight: 'bold',
    marginRight: 10,
  },
  drainIndicator: {
    width: 20,
    height: 20,
    borderRadius: 10,
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  controlButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 15,
    borderRadius: 8,
    flex: 1,
    margin: 5,
  },
  openButton: {
    backgroundColor: '#4caf50',
  },
  closeButton: {
    backgroundColor: '#f44336',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginLeft: 5,
  },
  gpsText: {
    fontSize: 16,
    marginVertical: 3,
    color: '#333',
  },
  mapButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 10,
    padding: 10,
    backgroundColor: '#e3f2fd',
    borderRadius: 8,
  },
  mapButtonText: {
    color: '#2196F3',
    fontSize: 16,
    marginLeft: 5,
    fontWeight: 'bold',
  },
  actionsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: 10,
  },
  actionButton: {
    alignItems: 'center',
    padding: 15,
    backgroundColor: '#fff',
    borderRadius: 10,
    flex: 1,
    margin: 5,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  actionButtonText: {
    marginTop: 5,
    fontSize: 12,
    color: '#2196F3',
    fontWeight: 'bold',
  },
});
