import 'package:flutter/material.dart';

class ScheduledText {
  final int id;
  final String text;
  final Color color;
  final int hour;
  final int minute;
  final int repeatDays; // Bitmask: bit0=Mon, bit1=Tue, ..., bit6=Sun, 0xFF=every day
  final int year; // 0 = every year
  final int month; // 0 = every month
  final int day; // 0 = every day
  final int loopCount; // 0 = infinite, 1+ = number of repetitions
  final bool enabled;

  ScheduledText({
    required this.id,
    required this.text,
    required this.color,
    required this.hour,
    required this.minute,
    this.repeatDays = 0xFF,
    this.year = 0,
    this.month = 0,
    this.day = 0,
    this.loopCount = 1,
    this.enabled = true,
  });

  /// Creates ScheduledText from CSV fields
  /// Expected format from ESP32: id,text,color,hour,min,repeat,year,month,day,loopCount,enabled
  factory ScheduledText.fromCsvFields(List<String> fields) {
    if (fields.length < 11) {
      throw ArgumentError('Invalid scheduled text CSV format: need at least 11 fields');
    }

    return ScheduledText(
      id: int.parse(fields[0]),
      text: fields[1],
      color: _rgb565ToColor(int.parse(fields[2])),
      hour: int.parse(fields[3]),
      minute: int.parse(fields[4]),
      repeatDays: int.parse(fields[5]),
      year: int.parse(fields[6]),
      month: int.parse(fields[7]),
      day: int.parse(fields[8]),
      loopCount: int.parse(fields[9]),
      enabled: fields[10] == '1',
    );
  }

  /// Converts RGB565 color (uint16) to Flutter Color
  static Color _rgb565ToColor(int rgb565) {
    final r = ((rgb565 >> 11) & 0x1F) << 3; // 5 bits rosso -> 8 bits
    final g = ((rgb565 >> 5) & 0x3F) << 2;  // 6 bits verde -> 8 bits
    final b = (rgb565 & 0x1F) << 3;         // 5 bits blu -> 8 bits
    return Color.fromARGB(255, r, g, b);
  }

  /// Converts Flutter Color to RGB565 (uint16)
  static int colorToRgb565(Color color) {
    final r = ((color.r * 255.0).round().clamp(0, 255) >> 3) & 0x1F;      // 8 bits -> 5 bits
    final g = ((color.g * 255.0).round().clamp(0, 255) >> 2) & 0x3F;    // 8 bits -> 6 bits
    final b = ((color.b * 255.0).round().clamp(0, 255) >> 3) & 0x1F;     // 8 bits -> 5 bits
    return (r << 11) | (g << 5) | b;
  }

  /// Returns a formatted time string (HH:MM)
  String get timeString {
    return '${hour.toString().padLeft(2, '0')}:${minute.toString().padLeft(2, '0')}';
  }

  /// Returns a human-readable repeat pattern
  String get repeatPattern {
    if (year != 0 || month != 0 || day != 0) {
      // Specific date
      if (year != 0) {
        return '$year-${month.toString().padLeft(2, '0')}-${day.toString().padLeft(2, '0')}';
      } else if (month != 0) {
        return 'Every ${month.toString().padLeft(2, '0')}-${day.toString().padLeft(2, '0')}';
      } else {
        return 'Every ${day.toString().padLeft(2, '0')} of month';
      }
    } else if (repeatDays == 0xFF) {
      return 'Every day';
    } else {
      // Week days
      final days = <String>[];
      if (repeatDays & 0x01 != 0) days.add('Mon');
      if (repeatDays & 0x02 != 0) days.add('Tue');
      if (repeatDays & 0x04 != 0) days.add('Wed');
      if (repeatDays & 0x08 != 0) days.add('Thu');
      if (repeatDays & 0x10 != 0) days.add('Fri');
      if (repeatDays & 0x20 != 0) days.add('Sat');
      if (repeatDays & 0x40 != 0) days.add('Sun');
      return days.join(', ');
    }
  }

  /// Returns week days as bitmask from list of selected days
  /// days: [0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun]
  static int weekDaysToBitmask(List<int> selectedDays) {
    int bitmask = 0;
    for (var day in selectedDays) {
      bitmask |= (1 << day);
    }
    return bitmask;
  }

  /// Returns list of selected days from bitmask
  /// Returns [0-6] where 0=Mon, 1=Tue, ..., 6=Sun
  static List<int> bitmaskToWeekDays(int bitmask) {
    final days = <int>[];
    for (int i = 0; i < 7; i++) {
      if (bitmask & (1 << i) != 0) {
        days.add(i);
      }
    }
    return days;
  }

  @override
  String toString() {
    return 'ScheduledText{id: $id, text: "$text", time: $timeString, repeat: $repeatPattern, enabled: $enabled}';
  }
}
