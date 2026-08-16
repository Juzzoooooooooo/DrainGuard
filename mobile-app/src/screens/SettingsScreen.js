import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  Switch,
  ScrollView,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import AsyncStorage from '@react-native-async-storage/async-storage';

export default function SettingsScreen() {
  const [deviceIP, setDeviceIP] = useState('192.168.1.100');
  const [alertPhone, setAlertPhone] = useState('');
  const [autoOpen, setAutoOpen] = useState(true);
  const [criticalLevel, setCriticalLevel] = useState('20');
  const [warningLevel, setWarningLevel] = useState('50');
  const [notifications, setNotifications] = useState(true);

  useEffect(() => {
    loadSettings();
  }, []);

  const loadSettings = async () => {
    try {
      const settings = await AsyncStorage.multiGet([
        'deviceIP',
        'alertPhone',
        'autoOpen',
        'criticalLevel',
        'warningLevel',
        'notifications',
      ]);

      settings.forEach(([key, value]) => {
        if (value !== null) {
          switch (key) {
            case 'deviceIP':
              setDeviceIP(value);
              break;
            case 'alertPhone':
              setAlertPhone(value);
              break;
            case 'autoOpen':
              setAutoOpen(value === 'true');
              break;
            case 'criticalLevel':
              setCriticalLevel(value);
              break;
            case 'warningLevel':
              setWarningLevel(value);
              break;
            case 'notifications':
              setNotifications(value === 'true');
              break;
          }
        }
      });
    } catch (error) {
      console.error('Failed to load settings:', error);
    }
  };

  const saveSettings = async () => {
    try {
      await AsyncStorage.multiSet([
        ['deviceIP', deviceIP],
        ['alertPhone', alertPhone],
        ['autoOpen', String(autoOpen)],
        ['criticalLevel', criticalLevel],
        ['warningLevel', warningLevel],
        ['notifications', String(notifications)],
      ]);

      Alert.alert('Success', 'Settings saved successfully');
    } catch (error) {
      Alert.alert('Error', 'Failed to save settings');
    }
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Configuration</Text>
        
        <Text style={styles.label}>Device IP Address</Text>
        <TextInput
          style={styles.input}
          value={deviceIP}
          onChangeText={setDeviceIP}
          placeholder="192.168.1.100"
          keyboardType="numeric"
        />

        <Text style={styles.label}>Alert Phone Number</Text>
        <TextInput
          style={styles.input}
          value={alertPhone}
          onChangeText={setAlertPhone}
          placeholder="+1234567890"
          keyboardType="phone-pad"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Alert Thresholds</Text>
        
        <Text style={styles.label}>Critical Level (cm)</Text>
        <TextInput
          style={styles.input}
          value={criticalLevel}
          onChangeText={setCriticalLevel}
          placeholder="20"
          keyboardType="numeric"
        />

        <Text style={styles.label}>Warning Level (cm)</Text>
        <TextInput
          style={styles.input}
          value={warningLevel}
          onChangeText={setWarningLevel}
          placeholder="50"
          keyboardType="numeric"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Automation</Text>
        
        <View style={styles.switchRow}>
          <View style={styles.switchLabel}>
            <Icon name="autorenew" size={24} color="#2196F3" />
            <Text style={styles.switchText}>Auto-open drain on critical level</Text>
          </View>
          <Switch
            value={autoOpen}
            onValueChange={setAutoOpen}
            trackColor={{ false: '#ddd', true: '#90caf9' }}
            thumbColor={autoOpen ? '#2196F3' : '#f4f3f4'}
          />
        </View>

        <View style={styles.switchRow}>
          <View style={styles.switchLabel}>
            <Icon name="notifications" size={24} color="#2196F3" />
            <Text style={styles.switchText}>Push notifications</Text>
          </View>
          <Switch
            value={notifications}
            onValueChange={setNotifications}
            trackColor={{ false: '#ddd', true: '#90caf9' }}
            thumbColor={notifications ? '#2196F3' : '#f4f3f4'}
          />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>App Version</Text>
          <Text style={styles.infoValue}>1.0.0</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Device Type</Text>
          <Text style={styles.infoValue}>ESP32 DevKit V1</Text>
        </View>
      </View>

      <TouchableOpacity style={styles.saveButton} onPress={saveSettings}>
        <Icon name="save" size={24} color="#fff" />
        <Text style={styles.saveButtonText}>Save Settings</Text>
      </TouchableOpacity>

      <View style={styles.spacer} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  section: {
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
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  label: {
    fontSize: 14,
    color: '#666',
    marginBottom: 5,
    marginTop: 10,
  },
  input: {
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    backgroundColor: '#fafafa',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  switchLabel: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
  },
  switchText: {
    fontSize: 16,
    marginLeft: 10,
    color: '#333',
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  infoLabel: {
    fontSize: 16,
    color: '#666',
  },
  infoValue: {
    fontSize: 16,
    color: '#333',
    fontWeight: '500',
  },
  saveButton: {
    flexDirection: 'row',
    backgroundColor: '#2196F3',
    margin: 10,
    padding: 15,
    borderRadius: 10,
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.2,
    shadowRadius: 4,
    elevation: 3,
  },
  saveButtonText: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
    marginLeft: 10,
  },
  spacer: {
    height: 20,
  },
});
