import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import MapView, { Marker, PROVIDER_GOOGLE } from 'react-native-maps';

export default function MapScreen({ route }) {
  const { location } = route.params;

  const region = {
    latitude: location?.latitude || 0,
    longitude: location?.longitude || 0,
    latitudeDelta: 0.01,
    longitudeDelta: 0.01,
  };

  return (
    <View style={styles.container}>
      <MapView
        provider={PROVIDER_GOOGLE}
        style={styles.map}
        initialRegion={region}
        showsUserLocation={true}
        showsMyLocationButton={true}
      >
        {location && location.latitude && location.longitude && (
          <Marker
            coordinate={{
              latitude: location.latitude,
              longitude: location.longitude,
            }}
            title="Drain Guard Location"
            description={`Sats: ${location.satellites || 0}`}
            pinColor="#2196F3"
          />
        )}
      </MapView>
      <View style={styles.infoCard}>
        <Text style={styles.infoTitle}>Device Location</Text>
        <Text style={styles.infoText}>
          Latitude: {location?.latitude?.toFixed(6) || 'N/A'}
        </Text>
        <Text style={styles.infoText}>
          Longitude: {location?.longitude?.toFixed(6) || 'N/A'}
        </Text>
        <Text style={styles.infoText}>
          Satellites: {location?.satellites || 0}
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  map: {
    width: Dimensions.get('window').width,
    height: Dimensions.get('window').height,
  },
  infoCard: {
    position: 'absolute',
    bottom: 20,
    left: 20,
    right: 20,
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.3,
    shadowRadius: 4,
    elevation: 5,
  },
  infoTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 10,
    color: '#333',
  },
  infoText: {
    fontSize: 14,
    marginVertical: 2,
    color: '#666',
  },
});
