import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  RefreshControl,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';

export default function HistoryScreen() {
  const [history, setHistory] = useState([]);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  useEffect(() => {
    loadHistory();
  }, []);

  const loadHistory = async () => {
    // Mock data - replace with actual API call
    const mockHistory = [
      {
        id: '1',
        timestamp: new Date(Date.now() - 3600000).toISOString(),
        type: 'alert',
        message: 'Critical water level detected',
        level: 185,
      },
      {
        id: '2',
        timestamp: new Date(Date.now() - 7200000).toISOString(),
        type: 'action',
        message: 'Drain opened automatically',
        level: 190,
      },
      {
        id: '3',
        timestamp: new Date(Date.now() - 10800000).toISOString(),
        type: 'info',
        message: 'System started',
        level: 50,
      },
      {
        id: '4',
        timestamp: new Date(Date.now() - 86400000).toISOString(),
        type: 'action',
        message: 'Drain closed by user',
        level: 30,
      },
      {
        id: '5',
        timestamp: new Date(Date.now() - 172800000).toISOString(),
        type: 'warning',
        message: 'Warning water level reached',
        level: 120,
      },
    ];

    setTimeout(() => {
      setHistory(mockHistory);
      setLoading(false);
      setRefreshing(false);
    }, 500);
  };

  const getIconName = (type) => {
    switch (type) {
      case 'alert':
        return 'warning';
      case 'action':
        return 'settings';
      case 'warning':
        return 'info';
      default:
        return 'circle';
    }
  };

  const getIconColor = (type) => {
    switch (type) {
      case 'alert':
        return '#f44336';
      case 'action':
        return '#2196F3';
      case 'warning':
        return '#ff9800';
      default:
        return '#666';
    }
  };

  const formatTimestamp = (timestamp) => {
    const date = new Date(timestamp);
    const now = new Date();
    const diff = now - date;

    if (diff < 3600000) {
      const mins = Math.floor(diff / 60000);
      return `${mins} min${mins !== 1 ? 's' : ''} ago`;
    } else if (diff < 86400000) {
      const hours = Math.floor(diff / 3600000);
      return `${hours} hour${hours !== 1 ? 's' : ''} ago`;
    } else if (diff < 604800000) {
      const days = Math.floor(diff / 86400000);
      return `${days} day${days !== 1 ? 's' : ''} ago`;
    } else {
      return date.toLocaleDateString();
    }
  };

  const renderItem = ({ item }) => (
    <View style={styles.historyItem}>
      <View style={[styles.iconContainer, { backgroundColor: getIconColor(item.type) + '20' }]}>
        <Icon name={getIconName(item.type)} size={24} color={getIconColor(item.type)} />
      </View>
      <View style={styles.historyContent}>
        <Text style={styles.historyMessage}>{item.message}</Text>
        <Text style={styles.historyTimestamp}>{formatTimestamp(item.timestamp)}</Text>
        <Text style={styles.historyLevel}>Water level: {item.level} cm</Text>
      </View>
    </View>
  );

  if (loading) {
    return (
      <View style={styles.centerContainer}>
        <ActivityIndicator size="large" color="#2196F3" />
        <Text style={styles.loadingText}>Loading history...</Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <FlatList
        data={history}
        renderItem={renderItem}
        keyExtractor={(item) => item.id}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={loadHistory} />
        }
        contentContainerStyle={styles.listContainer}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Icon name="inbox" size={64} color="#ccc" />
            <Text style={styles.emptyText}>No history available</Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  centerContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    marginTop: 10,
    fontSize: 16,
    color: '#666',
  },
  listContainer: {
    padding: 10,
  },
  historyItem: {
    flexDirection: 'row',
    backgroundColor: '#fff',
    padding: 15,
    marginBottom: 10,
    borderRadius: 10,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  iconContainer: {
    width: 50,
    height: 50,
    borderRadius: 25,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 15,
  },
  historyContent: {
    flex: 1,
  },
  historyMessage: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 5,
  },
  historyTimestamp: {
    fontSize: 12,
    color: '#999',
    marginBottom: 3,
  },
  historyLevel: {
    fontSize: 14,
    color: '#666',
  },
  emptyContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 50,
  },
  emptyText: {
    marginTop: 10,
    fontSize: 16,
    color: '#999',
  },
});
