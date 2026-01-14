import 'package:flutter/material.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';
import '../models/scheduled_text.dart';

class AddScheduledTextDialog extends StatefulWidget {
  final ScheduledText? scheduledText;

  const AddScheduledTextDialog({super.key, this.scheduledText});

  @override
  State<AddScheduledTextDialog> createState() => _AddScheduledTextDialogState();
}

class _AddScheduledTextDialogState extends State<AddScheduledTextDialog> {
  late TextEditingController _textController;
  late Color _selectedColor;
  late TimeOfDay _selectedTime;
  int _repeatMode = 0; // 0=every day, 1=week days, 2=specific date
  final List<bool> _selectedWeekDays = List.filled(7, false);
  DateTime? _selectedDate;
  int _loopCount = 1; // 0=infinite, 1+=number of loops

  @override
  void initState() {
    super.initState();

    if (widget.scheduledText != null) {
      // Edit mode
      final st = widget.scheduledText!;
      _textController = TextEditingController(text: st.text);
      _selectedColor = st.color;
      _selectedTime = TimeOfDay(hour: st.hour, minute: st.minute);
      _loopCount = st.loopCount;

      // Determine repeat mode
      if (st.year != 0 || st.month != 0 || st.day != 0) {
        _repeatMode = 2; // Specific date
        if (st.year != 0) {
          _selectedDate = DateTime(st.year, st.month, st.day);
        }
      } else if (st.repeatDays != 0xFF) {
        _repeatMode = 1; // Week days
        final days = ScheduledText.bitmaskToWeekDays(st.repeatDays);
        for (var day in days) {
          _selectedWeekDays[day] = true;
        }
      } else {
        _repeatMode = 0; // Every day
      }
    } else {
      // Add mode
      _textController = TextEditingController();
      _selectedColor = Colors.yellow;
      _selectedTime = TimeOfDay.now();
      _loopCount = 1;
    }
  }

  @override
  void dispose() {
    _textController.dispose();
    super.dispose();
  }

  Future<void> _pickColor() async {
    Color pickerColor = _selectedColor;

    await showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Scegli Colore'),
        content: SingleChildScrollView(
          child: ColorPicker(
            pickerColor: pickerColor,
            onColorChanged: (color) => pickerColor = color,
            pickerAreaHeightPercent: 0.8,
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Annulla'),
          ),
          TextButton(
            onPressed: () {
              setState(() => _selectedColor = pickerColor);
              Navigator.pop(context);
            },
            child: const Text('OK'),
          ),
        ],
      ),
    );
  }

  Future<void> _pickTime() async {
    final time = await showTimePicker(
      context: context,
      initialTime: _selectedTime,
    );

    if (time != null) {
      setState(() => _selectedTime = time);
    }
  }

  Future<void> _pickDate() async {
    final date = await showDatePicker(
      context: context,
      initialDate: _selectedDate ?? DateTime.now(),
      firstDate: DateTime.now(),
      lastDate: DateTime.now().add(const Duration(days: 365 * 2)),
    );

    if (date != null) {
      setState(() => _selectedDate = date);
    }
  }

  void _save() {
    if (_textController.text.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Inserisci un testo')),
      );
      return;
    }

    if (_repeatMode == 1 && !_selectedWeekDays.contains(true)) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Seleziona almeno un giorno')),
      );
      return;
    }

    if (_repeatMode == 2 && _selectedDate == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Seleziona una data')),
      );
      return;
    }

    int repeatDays = 0xFF;
    int year = 0;
    int month = 0;
    int day = 0;

    if (_repeatMode == 1) {
      // Week days
      repeatDays = ScheduledText.weekDaysToBitmask(
        _selectedWeekDays.asMap().entries.where((e) => e.value).map((e) => e.key).toList(),
      );
    } else if (_repeatMode == 2) {
      // Specific date
      if (_selectedDate != null) {
        year = _selectedDate!.year;
        month = _selectedDate!.month;
        day = _selectedDate!.day;
        repeatDays = 0xFF; // Ignored when date is specified
      }
    }

    Navigator.pop(context, {
      'text': _textController.text,
      'color': _selectedColor,
      'hour': _selectedTime.hour,
      'minute': _selectedTime.minute,
      'repeatDays': repeatDays,
      'year': year,
      'month': month,
      'day': day,
      'loopCount': _loopCount,
    });
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(widget.scheduledText == null ? 'Nuova Scritta Programmata' : 'Modifica Scritta'),
      content: SingleChildScrollView(
        child: SizedBox(
          width: MediaQuery.of(context).size.width * 0.9,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Text input
              TextField(
                controller: _textController,
                maxLength: 127,
                decoration: const InputDecoration(
                  labelText: 'Testo',
                  hintText: 'Inserisci il testo da visualizzare',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 16),

              // Color picker
              const Text('Colore:', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
              const SizedBox(height: 8),
              InkWell(
                onTap: _pickColor,
                child: Container(
                  height: 50,
                  decoration: BoxDecoration(
                    color: _selectedColor,
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(color: Colors.grey),
                  ),
                  child: const Center(
                    child: Text(
                      'Tocca per cambiare',
                      style: TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        shadows: [Shadow(color: Colors.black, blurRadius: 2)],
                      ),
                    ),
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // Time picker
              const Text('Ora:', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
              const SizedBox(height: 8),
              InkWell(
                onTap: _pickTime,
                child: Container(
                  padding: const EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    border: Border.all(color: Colors.grey),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Row(
                    children: [
                      const Icon(Icons.access_time),
                      const SizedBox(width: 12),
                      Text(
                        _selectedTime.format(context),
                        style: const TextStyle(fontSize: 18),
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // Repeat mode
              const Text('Ripetizione:', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
              const SizedBox(height: 8),
              SegmentedButton<int>(
                segments: const [
                  ButtonSegment(value: 0, label: Text('Ogni giorno'), icon: Icon(Icons.all_inclusive)),
                  ButtonSegment(value: 1, label: Text('Giorni'), icon: Icon(Icons.calendar_today)),
                  ButtonSegment(value: 2, label: Text('Data'), icon: Icon(Icons.event)),
                ],
                selected: {_repeatMode},
                onSelectionChanged: (Set<int> newSelection) {
                  setState(() => _repeatMode = newSelection.first);
                },
              ),
              const SizedBox(height: 12),

              // Week days selector
              if (_repeatMode == 1) ...[
                const Text('Seleziona giorni:', style: TextStyle(fontSize: 14)),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  children: List.generate(7, (index) {
                    final days = ['Lun', 'Mar', 'Mer', 'Gio', 'Ven', 'Sab', 'Dom'];
                    return FilterChip(
                      label: Text(days[index]),
                      selected: _selectedWeekDays[index],
                      onSelected: (selected) {
                        setState(() => _selectedWeekDays[index] = selected);
                      },
                    );
                  }),
                ),
              ],

              // Date selector
              if (_repeatMode == 2) ...[
                InkWell(
                  onTap: _pickDate,
                  child: Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      border: Border.all(color: Colors.grey),
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Row(
                      children: [
                        const Icon(Icons.calendar_today),
                        const SizedBox(width: 12),
                        Text(
                          _selectedDate == null
                              ? 'Seleziona data'
                              : '${_selectedDate!.day}/${_selectedDate!.month}/${_selectedDate!.year}',
                          style: const TextStyle(fontSize: 16),
                        ),
                      ],
                    ),
                  ),
                ),
              ],

              const SizedBox(height: 16),

              // Loop count selector
              const Text('Numero di ripetizioni:', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
              const SizedBox(height: 8),
              Row(
                children: [
                  Expanded(
                    child: SegmentedButton<int>(
                      segments: const [
                        ButtonSegment(value: 1, label: Text('1x')),
                        ButtonSegment(value: 2, label: Text('2x')),
                        ButtonSegment(value: 3, label: Text('3x')),
                        ButtonSegment(value: 0, label: Text('∞'), icon: Icon(Icons.all_inclusive, size: 16)),
                      ],
                      selected: {_loopCount},
                      onSelectionChanged: (Set<int> newSelection) {
                        setState(() => _loopCount = newSelection.first);
                      },
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 4),
              Text(
                _loopCount == 0
                    ? 'Il testo scorrerà all\'infinito (per uso manuale)'
                    : 'Il testo scorrerà $_loopCount volt${_loopCount == 1 ? 'a' : 'e'} poi tornerà all\'effetto precedente',
                style: TextStyle(fontSize: 12, color: Colors.grey[600]),
              ),
            ],
          ),
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Annulla'),
        ),
        ElevatedButton(
          onPressed: _save,
          child: const Text('Salva'),
        ),
      ],
    );
  }
}
