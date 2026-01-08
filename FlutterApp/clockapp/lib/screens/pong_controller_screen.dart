import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import '../services/device_service.dart';
import '../services/pong_device_interface.dart';
import '../widgets/common/common_widgets.dart';

class PongControllerScreen extends StatefulWidget {
  final IPongDevice deviceService;

  const PongControllerScreen({
    super.key,
    required this.deviceService,
  });

  @override
  State<PongControllerScreen> createState() => _PongControllerScreenState();
}

class _PongControllerScreenState extends State<PongControllerScreen> {
  PongState? _pongState;
  int? _myPlayer; // 1 o 2, null se non unito
  late List<StreamSubscription> _subscriptions;
  double _sliderValue = 50.0; // 0-100, inizia al centro

  @override
  void initState() {
    super.initState();
    _subscriptions = [
      widget.deviceService.pongStream.listen((state) {
        setState(() => _pongState = state);
      }),
    ];

    // Richiedi stato iniziale
    widget.deviceService.pongGetState();
  }

  @override
  void dispose() {
    // Lascia il gioco se siamo uniti
    if (_myPlayer != null) {
      widget.deviceService.pongLeave(_myPlayer!);
    }
    for (final sub in _subscriptions) {
      sub.cancel();
    }
    super.dispose();
  }

  void _joinPlayer(int player) {
    widget.deviceService.pongJoin(player);
    setState(() {
      _myPlayer = player;
      _sliderValue = 50.0; // Reset al centro
    });
    HapticFeedback.mediumImpact();
  }

  void _leavePlayer() {
    if (_myPlayer != null) {
      widget.deviceService.pongLeave(_myPlayer!);
      setState(() {
        _myPlayer = null;
        _sliderValue = 50.0;
      });
      HapticFeedback.lightImpact();
    }
  }

  void _onSliderChanged(double value) {
    if (_myPlayer != null) {
      setState(() => _sliderValue = value);
      // Invia posizione all'ESP (0=fondo, 100=cima)
      widget.deviceService.pongSetPosition(_myPlayer!, value.round());
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Pong Controller'),
        backgroundColor: Colors.transparent,
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () => widget.deviceService.pongGetState(),
            tooltip: 'Aggiorna stato',
          ),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            children: [
              _buildStatusCard(),
              const SizedBox(height: 16),
              if (_myPlayer == null) ...[
                _buildJoinCard(),
              ] else ...[
                _buildControllerCard(),
              ],
              const SizedBox(height: 16),
              _buildGameControlsCard(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildStatusCard() {
    final state = _pongState;
    final gameStateText = state != null
        ? _getGameStateText(state.gameState)
        : 'Caricamento...';
    final gameStateColor = state != null
        ? _getGameStateColor(state.gameState)
        : Colors.grey;

    return GlassCard(
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.sports_esports, color: gameStateColor),
              const SizedBox(width: 8),
              Text(
                gameStateText,
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: gameStateColor,
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          // Punteggio
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildPlayerScore(
                'P1',
                state?.score1 ?? 0,
                state?.player1IsHuman ?? false,
                _myPlayer == 1,
                Colors.green,
              ),
              Text(
                '-',
                style: TextStyle(
                  fontSize: 32,
                  fontWeight: FontWeight.bold,
                  color: Colors.grey[400],
                ),
              ),
              _buildPlayerScore(
                'P2',
                state?.score2 ?? 0,
                state?.player2IsHuman ?? false,
                _myPlayer == 2,
                Colors.red,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildPlayerScore(
    String label,
    int score,
    bool isHuman,
    bool isMe,
    Color color,
  ) {
    return Column(
      children: [
        Container(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          decoration: BoxDecoration(
            color: isMe
                ? color.withOpacity(0.3)
                : isHuman
                    ? color.withOpacity(0.15)
                    : Colors.grey.withOpacity(0.15),
            borderRadius: BorderRadius.circular(8),
            border: isMe
                ? Border.all(color: color, width: 2)
                : null,
          ),
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                label,
                style: TextStyle(
                  color: isHuman ? color : Colors.grey,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(width: 4),
              Icon(
                isHuman ? Icons.person : Icons.smart_toy,
                size: 16,
                color: isHuman ? color : Colors.grey,
              ),
            ],
          ),
        ),
        const SizedBox(height: 8),
        Text(
          '$score',
          style: TextStyle(
            fontSize: 48,
            fontWeight: FontWeight.bold,
            color: isHuman ? color : Colors.grey[400],
          ),
        ),
        if (isMe)
          Text(
            '(Tu)',
            style: TextStyle(
              fontSize: 12,
              color: color,
            ),
          ),
      ],
    );
  }

  Widget _buildJoinCard() {
    final p1Available = !(_pongState?.player1IsHuman ?? false);
    final p2Available = !(_pongState?.player2IsHuman ?? false);

    return GlassCard(
      child: Column(
        children: [
          Text(
            'Unisciti alla partita',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.grey[300],
            ),
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: _buildJoinButton(
                  'Player 1',
                  Colors.green,
                  p1Available,
                  () => _joinPlayer(1),
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: _buildJoinButton(
                  'Player 2',
                  Colors.red,
                  p2Available,
                  () => _joinPlayer(2),
                ),
              ),
            ],
          ),
          if (!p1Available && !p2Available)
            Padding(
              padding: const EdgeInsets.only(top: 12),
              child: Text(
                'Tutti i posti sono occupati',
                style: TextStyle(
                  color: Colors.orange[400],
                  fontSize: 12,
                ),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildJoinButton(
    String label,
    Color color,
    bool available,
    VoidCallback onTap,
  ) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: available ? onTap : null,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            color: color.withOpacity(available ? 0.2 : 0.05),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: color.withOpacity(available ? 0.5 : 0.1),
            ),
          ),
          child: Column(
            children: [
              Icon(
                available ? Icons.person_add : Icons.person,
                size: 32,
                color: available ? color : Colors.grey,
              ),
              const SizedBox(height: 8),
              Text(
                label,
                style: TextStyle(
                  color: available ? color : Colors.grey,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 4),
              Text(
                available ? 'Disponibile' : 'Occupato',
                style: TextStyle(
                  color: available ? color.withOpacity(0.7) : Colors.grey[600],
                  fontSize: 12,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildControllerCard() {
    final color = _myPlayer == 1 ? Colors.green : Colors.red;
    final isPlaying = _pongState?.isPlaying ?? false;

    return GlassCard(
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Player $_myPlayer',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: color,
                ),
              ),
              TextButton.icon(
                onPressed: _leavePlayer,
                icon: const Icon(Icons.exit_to_app, size: 18),
                label: const Text('Esci'),
                style: TextButton.styleFrom(
                  foregroundColor: Colors.orange[400],
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
          // Slider verticale per controllo paddle
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              // Etichetta "TOP"
              Text(
                'TOP',
                style: TextStyle(
                  color: Colors.grey[600],
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(width: 16),
              // Slider verticale
              Container(
                height: 300,
                padding: const EdgeInsets.symmetric(horizontal: 8),
                decoration: BoxDecoration(
                  color: color.withOpacity(isPlaying ? 0.1 : 0.03),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(
                    color: color.withOpacity(isPlaying ? 0.4 : 0.1),
                    width: 2,
                  ),
                ),
                child: RotatedBox(
                  quarterTurns: 3,
                  child: SliderTheme(
                    data: SliderThemeData(
                      activeTrackColor: color,
                      inactiveTrackColor: color.withOpacity(0.2),
                      thumbColor: color,
                      overlayColor: color.withOpacity(0.3),
                      thumbShape: const RoundSliderThumbShape(
                        enabledThumbRadius: 12,
                      ),
                      trackHeight: 8,
                    ),
                    child: Slider(
                      value: _sliderValue,
                      min: 0,
                      max: 100,
                      onChanged: isPlaying ? _onSliderChanged : null,
                      onChangeStart: (_) {
                        if (isPlaying) HapticFeedback.lightImpact();
                      },
                      onChangeEnd: (_) {
                        if (isPlaying) HapticFeedback.mediumImpact();
                      },
                    ),
                  ),
                ),
              ),
              const SizedBox(width: 16),
              // Etichetta "BOTTOM"
              Text(
                'BOTTOM',
                style: TextStyle(
                  color: Colors.grey[600],
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          // Indicatore posizione
          Text(
            '${_sliderValue.round()}%',
            style: TextStyle(
              color: color,
              fontSize: 24,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            isPlaying
                ? 'Muovi lo slider per controllare'
                : 'Attendi inizio partita',
            style: TextStyle(
              color: Colors.grey[500],
              fontSize: 12,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildGameControlsCard() {
    final state = _pongState;
    final isWaiting = state?.isWaiting ?? false;
    final isPlaying = state?.isPlaying ?? false;
    final isPaused = state?.isPaused ?? false;
    final isGameOver = state?.isGameOver ?? false;
    final hasPlayers = (state?.player1IsHuman ?? false) ||
        (state?.player2IsHuman ?? false);

    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.gamepad,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(width: 12),
              const Text(
                'Controlli Partita',
                style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
              ),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              // Start - visibile solo in waiting o gameover con almeno un player
              if (isWaiting || isGameOver)
                Expanded(
                  child: ActionButton(
                    icon: Icons.play_arrow,
                    label: 'Start',
                    color: Colors.green[400],
                    enabled: hasPlayers,
                    onTap: () {
                      widget.deviceService.pongStart();
                      HapticFeedback.mediumImpact();
                    },
                  ),
                ),
              // Pause - visibile solo in playing
              if (isPlaying)
                Expanded(
                  child: ActionButton(
                    icon: Icons.pause,
                    label: 'Pausa',
                    color: Colors.orange[400],
                    onTap: () {
                      widget.deviceService.pongPause();
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              // Resume - visibile solo in paused
              if (isPaused)
                Expanded(
                  child: ActionButton(
                    icon: Icons.play_arrow,
                    label: 'Riprendi',
                    color: Colors.green[400],
                    onTap: () {
                      widget.deviceService.pongResume();
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              const SizedBox(width: 12),
              // Reset - sempre visibile
              Expanded(
                child: ActionButton(
                  icon: Icons.restart_alt,
                  label: 'Reset',
                  color: Colors.red[400],
                  onTap: () {
                    widget.deviceService.pongReset();
                    HapticFeedback.mediumImpact();
                  },
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  String _getGameStateText(String state) {
    switch (state) {
      case 'waiting':
        return 'In attesa';
      case 'playing':
        return 'In gioco';
      case 'paused':
        return 'Pausa';
      case 'gameover':
        return 'Game Over';
      default:
        return state;
    }
  }

  Color _getGameStateColor(String state) {
    switch (state) {
      case 'waiting':
        return Colors.orange;
      case 'playing':
        return Colors.green;
      case 'paused':
        return Colors.yellow;
      case 'gameover':
        return Colors.red;
      default:
        return Colors.grey;
    }
  }
}
