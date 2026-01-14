import 'dart:async';
import 'package:flutter/material.dart';
import '../models/scheduled_text.dart';
import 'device_service.dart';

class ScheduledTextService {
  final DeviceService _device;
  final StreamController<String> _statusController = StreamController.broadcast();

  ScheduledTextService(this._device);

  Stream<String> get statusStream => _statusController.stream;

  void dispose() {
    _statusController.close();
  }

  /// Lists all scheduled texts from the device
  Future<List<ScheduledText>> listScheduledTexts() async {
    try {
      _updateStatus('Loading scheduled texts...');

      // Send command
      _device.send('schedtext,list');

      // Wait for response
      final completer = Completer<List<ScheduledText>>();
      StreamSubscription? subscription;

      subscription = _device.rawDataStream.listen((data) {
        if (data.startsWith('SCHEDULED_TEXTS,')) {
          subscription?.cancel();

          try {
            print('[ScheduledTextService] Received CSV: $data');
            final texts = _parseScheduledTexts(data);
            _updateStatus('Loaded ${texts.length} scheduled text(s)');
            completer.complete(texts);
          } catch (e) {
            _updateStatus('Error parsing scheduled texts: $e');
            completer.completeError('Failed to parse scheduled texts: $e');
          }
        } else if (data.startsWith('ERR,')) {
          subscription?.cancel();
          final error = data.substring(4);
          _updateStatus('Error: $error');
          completer.completeError(error);
        }
      });

      // Timeout after 5 seconds
      return completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          subscription?.cancel();
          _updateStatus('Timeout: no response from device');
          throw TimeoutException('No response from device');
        },
      );
    } catch (e) {
      _updateStatus('Error: $e');
      rethrow;
    }
  }

  /// Adds a new scheduled text
  Future<bool> addScheduledText({
    required String text,
    required Color color,
    required int hour,
    required int minute,
    int repeatDays = 0xFF,
    int year = 0,
    int month = 0,
    int day = 0,
    int loopCount = 1,
  }) async {
    try {
      _updateStatus('Adding scheduled text...');

      final colorValue = ScheduledText.colorToRgb565(color);

      // Build command
      String command = 'schedtext,add,$text,$colorValue,$hour,$minute,$repeatDays,$year,$month,$day,$loopCount';

      // Send command
      _device.send(command);

      // Wait for response
      final completer = Completer<bool>();
      StreamSubscription? subscription;

      subscription = _device.rawDataStream.listen((data) {
        if (data.startsWith('OK,')) {
          subscription?.cancel();
          _updateStatus('Scheduled text added successfully');
          completer.complete(true);
        } else if (data.startsWith('ERR,')) {
          subscription?.cancel();
          final error = data.substring(4);
          _updateStatus('Error: $error');
          completer.complete(false);
        }
      });

      // Timeout after 5 seconds
      return completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          subscription?.cancel();
          _updateStatus('Timeout: no response from device');
          return false;
        },
      );
    } catch (e) {
      _updateStatus('Error: $e');
      return false;
    }
  }

  /// Updates an existing scheduled text
  Future<bool> updateScheduledText({
    required int id,
    required String text,
    required Color color,
    required int hour,
    required int minute,
    int repeatDays = 0xFF,
    int year = 0,
    int month = 0,
    int day = 0,
    int loopCount = 1,
  }) async {
    try {
      _updateStatus('Updating scheduled text...');

      final colorValue = ScheduledText.colorToRgb565(color);

      // Build command
      String command = 'schedtext,update,$id,$text,$colorValue,$hour,$minute,$repeatDays,$year,$month,$day,$loopCount';

      print('[ScheduledTextService] Sending update command: $command');

      // Send command
      _device.send(command);

      // Wait for response
      final completer = Completer<bool>();
      StreamSubscription? subscription;

      subscription = _device.rawDataStream.listen((data) {
        if (data.startsWith('OK,')) {
          subscription?.cancel();
          _updateStatus('Scheduled text updated successfully');
          completer.complete(true);
        } else if (data.startsWith('ERR,')) {
          subscription?.cancel();
          final error = data.substring(4);
          _updateStatus('Error: $error');
          completer.complete(false);
        }
      });

      // Timeout after 5 seconds
      return completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          subscription?.cancel();
          _updateStatus('Timeout: no response from device');
          return false;
        },
      );
    } catch (e) {
      _updateStatus('Error: $e');
      return false;
    }
  }

  /// Deletes a scheduled text
  Future<bool> deleteScheduledText(int id) async {
    try {
      _updateStatus('Deleting scheduled text...');

      // Send command
      _device.send('schedtext,delete,$id');

      // Wait for response
      final completer = Completer<bool>();
      StreamSubscription? subscription;

      subscription = _device.rawDataStream.listen((data) {
        if (data.startsWith('OK,')) {
          subscription?.cancel();
          _updateStatus('Scheduled text deleted successfully');
          completer.complete(true);
        } else if (data.startsWith('ERR,')) {
          subscription?.cancel();
          final error = data.substring(4);
          _updateStatus('Error: $error');
          completer.complete(false);
        }
      });

      // Timeout after 5 seconds
      return completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          subscription?.cancel();
          _updateStatus('Timeout: no response from device');
          return false;
        },
      );
    } catch (e) {
      _updateStatus('Error: $e');
      return false;
    }
  }

  /// Enables a scheduled text
  Future<bool> enableScheduledText(int id) async {
    return _setEnabled(id, true);
  }

  /// Disables a scheduled text
  Future<bool> disableScheduledText(int id) async {
    return _setEnabled(id, false);
  }

  Future<bool> _setEnabled(int id, bool enabled) async {
    try {
      _updateStatus(enabled ? 'Enabling...' : 'Disabling...');

      // Send command
      _device.send('schedtext,${enabled ? 'enable' : 'disable'},$id');

      // Wait for response
      final completer = Completer<bool>();
      StreamSubscription? subscription;

      subscription = _device.rawDataStream.listen((data) {
        if (data.startsWith('OK,')) {
          subscription?.cancel();
          _updateStatus(enabled ? 'Enabled' : 'Disabled');
          completer.complete(true);
        } else if (data.startsWith('ERR,')) {
          subscription?.cancel();
          final error = data.substring(4);
          _updateStatus('Error: $error');
          completer.complete(false);
        }
      });

      // Timeout after 5 seconds
      return completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          subscription?.cancel();
          _updateStatus('Timeout: no response from device');
          return false;
        },
      );
    } catch (e) {
      _updateStatus('Error: $e');
      return false;
    }
  }

  /// Parses SCHEDULED_TEXTS response from ESP32
  /// Format: SCHEDULED_TEXTS,count,id1,text1,color1,hour1,min1,repeat1,year1,month1,day1,enabled1,...
  List<ScheduledText> _parseScheduledTexts(String data) {
    final parts = data.split(',');

    if (parts[0] != 'SCHEDULED_TEXTS') {
      throw FormatException('Invalid format: expected SCHEDULED_TEXTS');
    }

    if (parts.length < 2) {
      throw FormatException('Invalid format: missing count');
    }

    final count = int.parse(parts[1]);
    final texts = <ScheduledText>[];

    // Each scheduled text has 11 fields
    const fieldsPerText = 11;
    int index = 2;

    for (int i = 0; i < count; i++) {
      if (index + fieldsPerText > parts.length) {
        throw FormatException('Invalid format: incomplete scheduled text data');
      }

      try {
        final textFields = parts.sublist(index, index + fieldsPerText);
        print('[ScheduledTextService] Parsing text $i - fields: $textFields');
        final scheduledText = ScheduledText.fromCsvFields(textFields);
        print('[ScheduledTextService] Parsed: id=${scheduledText.id}, loopCount=${scheduledText.loopCount}');
        texts.add(scheduledText);
        index += fieldsPerText;
      } catch (e) {
        print('Error parsing scheduled text $i: $e');
        // Skip this one and continue
        index += fieldsPerText;
      }
    }

    return texts;
  }

  void _updateStatus(String status) {
    if (!_statusController.isClosed) {
      _statusController.add(status);
    }
  }
}
