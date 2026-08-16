import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Alert,
  ActivityIndicator,
} from 'react-native';
import Slider from '@react-native-community/slider';
import Icon from 'react-native-vector-icons/MaterialIcons';
import axios from 'axios';
import AsyncStorage from '@react-native-async-storage/async-storage';

export default function ServoControlScreen() {
  const [deviceIP, setDeviceIP] = useState('192.168.1.100');
  const [loading, setLoading] = useState(true);
  
  // Servo positions
  const [basePos, setBasePos] = useState(330);
  const [shoulderPos, setShoulderPos] = useState(150);
  const [elbowPos, setElbowPos] = useState(300);
  const [gripperPos, setGripperPos] = useState(410);

  useEffect(() => {
    loadDeviceIP();
    loadServoStatus();
  }, []);

  const loadDeviceIP = async () => {
    const ip = await AsyncStorage.getItem('deviceIP');
    if (ip) setDeviceIP(ip);
  };

  const loadServoStatus = async () => {
    try {
      const response = await axios.get(`http://${deviceIP}/api/servo/status`);
      const { base, shoulder, elbow, gripper } = response.data;
      setBasePos(base);
      setShoulderPos(shoulder);
      setElbowPos(elbow);
      setGripperPos(gripper);
      setLoading(false);
    } catch (error) {
      console.error('Failed to load servo status:', error);
      setLoading(false);
    }
  };

  const setServo = async (servo, position) => {
    try {
      await axios.post(`http://${deviceIP}/api/servo/${servo}?position=${position}`);
    } catch (error) {
      Alert.alert('Error', `Failed to control ${servo} servo`);
    }
  };

  const handleArmOpen = async () => {
    try {
      await axios.post(`http://${deviceIP}/api/arm/open`);
      Alert.alert('Success', 'Opening drain with robotic arm');
      setTimeout(loadServoStatus, 3000);
    } catch (error) {
      Alert.alert('Error', 'Failed to open drain with arm');
    }
  };

  const handleArmClose = async () => {
    try {
      await axios.post(`http://${deviceIP}/api/arm/close`);
      Alert.alert('Success', 'Closing drain with robotic arm');
      setTimeout(loadServoStatus, 3000);
    } catch (error) {
      Alert.alert('Error', 'Failed to close drain with arm');
    }
  };

  const handleHomePosition = async () => {
    try {
      await axios.post(`http://${deviceIP}/api/arm/home`);
      Alert.alert('Success', 'Moving to home position');
      setTimeout(loadServoStatus, 2000);
    } catch (error) {
      Alert.alert('Error', 'Failed to move to home position');
    }
  };

  const handleDemo = async () => {
    try {
      await axios.post(`http://${deviceIP}/api/arm/demo`);
      Alert.alert('Demo Started', 'Running demonstration sequence');
    } catch (error) {
      Alert.alert('Error', 'Failed to run demo');
    }
  };

  if (loading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.loadingText}>Loading servo controller...</Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.quickActions}>
        <Text style={styles.sectionTitle}>Quick Actions</Text>
        <View style={styles.buttonRow}>
          <TouchableOpacity style={[styles.actionBtn, styles.openBtn]} onPress={handleArmOpen}>
            <Icon name="arrow-upward" size={24} color="#fff" />
            <Text style={styles.btnText}>Open Drain</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.actionBtn, styles.closeBtn]} onPress={handleArmClose}>
            <Icon name="arrow-downward" size={24} color="#fff" />
            <Text style={styles.btnText}>Close Drain</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.buttonRow}>
          <TouchableOpacity style={[styles.actionBtn, styles.homeBtn]} onPress={handleHomePosition}>
            <Icon name="home" size={24} color="#fff" />
            <Text style={styles.btnText}>Home</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.actionBtn, styles.demoBtn]} onPress={handleDemo}>
            <Icon name="play-circle-filled" size={24} color="#fff" />
            <Text style={styles.btnText}>Demo</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.servoControls}>
        <Text style={styles.sectionTitle}>Manual Servo Control</Text>

        {/* Base Servo */}
        <View style={styles.servoControl}>
          <View style={styles.servoHeader}>
            <Icon name="360" size={24} color="#2196F3" />
            <Text style={styles.servoLabel}>Base</Text>
            <Text style={styles.servoValue}>{Math.round(basePos)}</Text>
          </View>
          <Slider
            style={styles.slider}
            minimumValue={150}
            maximumValue={450}
            value={basePos}
            onValueChange={setBasePos}
            onSlidingComplete={(value) => setServo('base', Math.round(value))}
            minimumTrackTintColor="#2196F3"
            maximumTrackTintColor="#ddd"
            thumbTintColor="#2196F3"
          />
        </View>

        {/* Shoulder Servo */}
        <View style={styles.servoControl}>
          <View style={styles.servoHeader}>
            <Icon name="unfold-more" size={24} color="#4caf50" />
            <Text style={styles.servoLabel}>Shoulder</Text>
            <Text style={styles.servoValue}>{Math.round(shoulderPos)}</Text>
          </View>
          <Slider
            style={styles.slider}
            minimumValue={150}
            maximumValue={380}
            value={shoulderPos}
            onValueChange={setShoulderPos}
            onSlidingComplete={(value) => setServo('shoulder', Math.round(value))}
            minimumTrackTintColor="#4caf50"
            maximumTrackTintColor="#ddd"
            thumbTintColor="#4caf50"
          />
        </View>

        {/* Elbow Servo */}
        <View style={styles.servoControl}>
          <View style={styles.servoHeader}>
            <Icon name="dehaze" size={24} color="#ff9800" />
            <Text style={styles.servoLabel}>Elbow</Text>
            <Text style={styles.servoValue}>{Math.round(elbowPos)}</Text>
          </View>
          <Slider
            style={styles.slider}
            minimumValue={300}
            maximumValue={380}
            value={elbowPos}
            onValueChange={setElbowPos}
            onSlidingComplete={(value) => setServo('elbow', Math.round(value))}
            minimumTrackTintColor="#ff9800"
            maximumTrackTintColor="#ddd"
            thumbTintColor="#ff9800"
          />
        </View>

        {/* Gripper Servo */}
        <View style={styles.servoControl}>
          <View style={styles.servoHeader}>
            <Icon name="pan-tool" size={24} color="#f44336" />
            <Text style={styles.servoLabel}>Gripper</Text>
            <Text style={styles.servoValue}>{Math.round(gripperPos)}</Text>
          </View>
          <Slider
            style={styles.slider}
            minimumValue={410}
            maximumValue={510}
            value={gripperPos}
            onValueChange={setGripperPos}
            onSlidingComplete={(value) => setServo('gripper', Math.round(value))}
            minimumTrackTintColor="#f44336"
            maximumTrackTintColor="#ddd"
            thumbTintColor="#f44336"
          />
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 10,
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    marginTop: 10,
    fontSize: 16,
    color: '#666',
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  quickActions: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 10,
    marginBottom: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 10,
  },
  actionBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 15,
    borderRadius: 8,
    margin: 5,
  },
  openBtn: {
    backgroundColor: '#4caf50',
  },
  closeBtn: {
    backgroundColor: '#f44336',
  },
  homeBtn: {
    backgroundColor: '#2196F3',
  },
  demoBtn: {
    backgroundColor: '#9c27b0',
  },
  btnText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginLeft: 5,
  },
  servoControls: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  servoControl: {
    marginBottom: 20,
  },
  servoHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  servoLabel: {
    fontSize: 16,
    fontWeight: 'bold',
    marginLeft: 10,
    flex: 1,
    color: '#333',
  },
  servoValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#666',
  },
  slider: {
    width: '100%',
    height: 40,
  },
});
