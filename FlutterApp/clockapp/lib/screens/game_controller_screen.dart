import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:async';
import '../services/device_service.dart';
import '../widgets/common/common_widgets.dart';

/// Tipo di gioco supportato
enum GameType { pong, snake }

class GameControllerScreen extends StatefulWidget {
  final DeviceService deviceService;
  final GameType initialGame;

  const GameControllerScreen({
    super.key,
    required this.deviceService,
    this.initialGame = GameType.pong,
  });

  @override
  State<GameControllerScreen> createState() => _GameControllerScreenState();
}

class _GameControllerScreenState extends State<GameControllerScreen>
    with SingleTickerProviderStateMixin {
  late GameType _currentGame;
  late TabController _tabController;
  late List<StreamSubscription> _subscriptions;

  // Pong state
  PongState? _pongState;
  int? _myPongPlayer;
  double _pongSliderValue = 50.0;

  // Snake state
  SnakeState? _snakeState;
  bool _snakeJoined = false;

  @override
  void initState() {
    super.initState();
    _currentGame = widget.initialGame;
    _tabController = TabController(
      length: 2,
      vsync: this,
      initialIndex: _currentGame == GameType.pong ? 0 : 1,
    );

    _tabController.addListener(() {
      if (!_tabController.indexIsChanging) {
        setState(() {
          _currentGame = _tabController.index == 0 ? GameType.pong : GameType.snake;
        });
        _requestGameState();
      }
    });

    _subscriptions = [
      widget.deviceService.pongStream.listen((state) {
        setState(() => _pongState = state);
      }),
      widget.deviceService.snakeStream.listen((state) {
        setState(() {
          _snakeState = state;
          _snakeJoined = state.playerJoined;
        });
      }),
    ];

    // Richiedi stato iniziale
    _requestGameState();
  }

  void _requestGameState() {
    if (_currentGame == GameType.pong) {
      widget.deviceService.pongGetState();
    } else {
      widget.deviceService.snakeGetState();
    }
  }

  @override
  void dispose() {
    // Lascia i giochi se siamo uniti
    if (_myPongPlayer != null) {
      widget.deviceService.pongLeave(_myPongPlayer!);
    }
    if (_snakeJoined) {
      widget.deviceService.snakeLeave();
    }
    for (final sub in _subscriptions) {
      sub.cancel();
    }
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Games'),
        backgroundColor: Colors.transparent,
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _requestGameState,
            tooltip: 'Aggiorna stato',
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: _currentGame == GameType.pong
              ? Colors.green
              : Colors.lime,
          tabs: [
            Tab(
              icon: Icon(
                Icons.sports_tennis,
                color: _tabController.index == 0 ? Colors.green : Colors.grey,
              ),
              text: 'Pong',
            ),
            Tab(
              icon: Icon(
                Icons.pest_control,
                color: _tabController.index == 1 ? Colors.lime : Colors.grey,
              ),
              text: 'Snake',
            ),
          ],
        ),
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          _buildPongTab(),
          _buildSnakeTab(),
        ],
      ),
    );
  }

  // ═══════════════════════════════════════════
  // PONG TAB
  // ═══════════════════════════════════════════

  Widget _buildPongTab() {
    return SafeArea(
      child: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            _buildPongStatusCard(),
            const SizedBox(height: 16),
            if (_myPongPlayer == null) ...[
              _buildPongJoinCard(),
            ] else ...[
              _buildPongControllerCard(),
            ],
            const SizedBox(height: 16),
            _buildPongControlsCard(),
          ],
        ),
      ),
    );
  }

  Widget _buildPongStatusCard() {
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
              Icon(Icons.sports_tennis, color: gameStateColor),
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
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildPongPlayerScore(
                'P1',
                state?.score1 ?? 0,
                state?.player1IsHuman ?? false,
                _myPongPlayer == 1,
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
              _buildPongPlayerScore(
                'P2',
                state?.score2 ?? 0,
                state?.player2IsHuman ?? false,
                _myPongPlayer == 2,
                Colors.red,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildPongPlayerScore(
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
                ? color.withValues(alpha: 0.3)
                : isHuman
                    ? color.withValues(alpha: 0.15)
                    : Colors.grey.withValues(alpha: 0.15),
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

  Widget _buildPongJoinCard() {
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
                  () => _pongJoinPlayer(1),
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: _buildJoinButton(
                  'Player 2',
                  Colors.red,
                  p2Available,
                  () => _pongJoinPlayer(2),
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
            color: color.withValues(alpha: available ? 0.2 : 0.05),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: color.withValues(alpha: available ? 0.5 : 0.1),
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
                  color: available ? color.withValues(alpha: 0.7) : Colors.grey[600],
                  fontSize: 12,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildPongControllerCard() {
    final color = _myPongPlayer == 1 ? Colors.green : Colors.red;
    final isPlaying = _pongState?.isPlaying ?? false;

    return GlassCard(
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Player $_myPongPlayer',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: color,
                ),
              ),
              TextButton.icon(
                onPressed: _pongLeavePlayer,
                icon: const Icon(Icons.exit_to_app, size: 18),
                label: const Text('Esci'),
                style: TextButton.styleFrom(
                  foregroundColor: Colors.orange[400],
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                'TOP',
                style: TextStyle(
                  color: Colors.grey[600],
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(width: 16),
              Container(
                height: 250,
                padding: const EdgeInsets.symmetric(horizontal: 8),
                decoration: BoxDecoration(
                  color: color.withValues(alpha: isPlaying ? 0.1 : 0.03),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(
                    color: color.withValues(alpha: isPlaying ? 0.4 : 0.1),
                    width: 2,
                  ),
                ),
                child: RotatedBox(
                  quarterTurns: 3,
                  child: SliderTheme(
                    data: SliderThemeData(
                      activeTrackColor: color,
                      inactiveTrackColor: color.withValues(alpha: 0.2),
                      thumbColor: color,
                      overlayColor: color.withValues(alpha: 0.3),
                      thumbShape: const RoundSliderThumbShape(
                        enabledThumbRadius: 12,
                      ),
                      trackHeight: 8,
                    ),
                    child: Slider(
                      value: _pongSliderValue,
                      min: 0,
                      max: 100,
                      onChanged: isPlaying ? _onPongSliderChanged : null,
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
          Text(
            '${_pongSliderValue.round()}%',
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

  Widget _buildPongControlsCard() {
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

  void _pongJoinPlayer(int player) {
    widget.deviceService.pongJoin(player);
    setState(() {
      _myPongPlayer = player;
      _pongSliderValue = 50.0;
    });
    HapticFeedback.mediumImpact();
  }

  void _pongLeavePlayer() {
    if (_myPongPlayer != null) {
      widget.deviceService.pongLeave(_myPongPlayer!);
      setState(() {
        _myPongPlayer = null;
        _pongSliderValue = 50.0;
      });
      HapticFeedback.lightImpact();
    }
  }

  void _onPongSliderChanged(double value) {
    if (_myPongPlayer != null) {
      setState(() => _pongSliderValue = value);
      widget.deviceService.pongSetPosition(_myPongPlayer!, value.round());
    }
  }

  // ═══════════════════════════════════════════
  // SNAKE TAB
  // ═══════════════════════════════════════════

  Widget _buildSnakeTab() {
    return SafeArea(
      child: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            _buildSnakeStatusCard(),
            const SizedBox(height: 16),
            if (!_snakeJoined) ...[
              _buildSnakeJoinCard(),
            ] else ...[
              _buildSnakeControllerCard(),
            ],
            const SizedBox(height: 16),
            _buildSnakeControlsCard(),
          ],
        ),
      ),
    );
  }

  Widget _buildSnakeStatusCard() {
    final state = _snakeState;
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
              Icon(Icons.pest_control, color: gameStateColor),
              const SizedBox(width: 8),
              Text(
                'Snake - $gameStateText',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: gameStateColor,
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          // Score e Stats
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildSnakeStat(
                'Score',
                '${state?.score ?? 0}',
                Colors.lime,
                Icons.star,
              ),
              _buildSnakeStat(
                'High',
                '${state?.highScore ?? 0}',
                Colors.amber,
                Icons.emoji_events,
              ),
              _buildSnakeStat(
                'Level',
                '${state?.level ?? 1}',
                Colors.cyan,
                Icons.speed,
              ),
              _buildSnakeStat(
                'Length',
                '${state?.length ?? 3}',
                Colors.green,
                Icons.straighten,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSnakeStat(String label, String value, Color color, IconData icon) {
    return Column(
      children: [
        Container(
          padding: const EdgeInsets.all(8),
          decoration: BoxDecoration(
            color: color.withValues(alpha: 0.15),
            borderRadius: BorderRadius.circular(12),
          ),
          child: Icon(icon, color: color, size: 24),
        ),
        const SizedBox(height: 8),
        Text(
          value,
          style: TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
        Text(
          label,
          style: TextStyle(
            fontSize: 10,
            color: Colors.grey[500],
          ),
        ),
      ],
    );
  }

  Widget _buildSnakeJoinCard() {
    return GlassCard(
      child: Column(
        children: [
          // Serpente decorativo animato
          Container(
            padding: const EdgeInsets.all(20),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: List.generate(6, (i) {
                final colors = [
                  Colors.lime[700],
                  Colors.lime[600],
                  Colors.lime[500],
                  Colors.lime[400],
                  Colors.lime[300],
                  Colors.lime[200],
                ];
                return Container(
                  width: 20,
                  height: 20,
                  margin: const EdgeInsets.symmetric(horizontal: 2),
                  decoration: BoxDecoration(
                    color: colors[i],
                    borderRadius: BorderRadius.circular(i == 0 ? 8 : 4),
                    border: i == 0
                        ? Border.all(color: Colors.white, width: 2)
                        : null,
                  ),
                  child: i == 0
                      ? const Center(
                          child: Text(':', style: TextStyle(fontSize: 10)),
                        )
                      : null,
                );
              }),
            ),
          ),
          Text(
            'Pronto a giocare?',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.grey[300],
            ),
          ),
          const SizedBox(height: 16),
          SizedBox(
            width: double.infinity,
            child: ElevatedButton.icon(
              onPressed: _snakeJoin,
              icon: const Icon(Icons.play_circle_outline),
              label: const Text('Unisciti al Gioco'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.lime[700],
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(vertical: 16),
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSnakeControllerCard() {
    final isPlaying = _snakeState?.isPlaying ?? false;
    final direction = _snakeState?.direction ?? 'right';

    return GlassCard(
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Row(
                children: [
                  Icon(Icons.pest_control, color: Colors.lime[400]),
                  const SizedBox(width: 8),
                  Text(
                    'Controllo Serpente',
                    style: TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                      color: Colors.lime[400],
                    ),
                  ),
                ],
              ),
              TextButton.icon(
                onPressed: _snakeLeave,
                icon: const Icon(Icons.exit_to_app, size: 18),
                label: const Text('Esci'),
                style: TextButton.styleFrom(
                  foregroundColor: Colors.orange[400],
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
          // D-Pad per controllo direzione
          SizedBox(
            height: 200,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // Pulsante UP
                _buildDirectionButton(
                  Icons.keyboard_arrow_up,
                  'up',
                  direction == 'up',
                  isPlaying,
                ),
                const SizedBox(height: 8),
                // Riga centrale: LEFT - indicatore - RIGHT
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _buildDirectionButton(
                      Icons.keyboard_arrow_left,
                      'left',
                      direction == 'left',
                      isPlaying,
                    ),
                    const SizedBox(width: 8),
                    // Indicatore centrale direzione
                    Container(
                      width: 60,
                      height: 60,
                      decoration: BoxDecoration(
                        color: Colors.lime.withValues(alpha: 0.2),
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(
                          color: Colors.lime.withValues(alpha: 0.4),
                          width: 2,
                        ),
                      ),
                      child: Icon(
                        _getDirectionIcon(direction),
                        color: Colors.lime,
                        size: 32,
                      ),
                    ),
                    const SizedBox(width: 8),
                    _buildDirectionButton(
                      Icons.keyboard_arrow_right,
                      'right',
                      direction == 'right',
                      isPlaying,
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                // Pulsante DOWN
                _buildDirectionButton(
                  Icons.keyboard_arrow_down,
                  'down',
                  direction == 'down',
                  isPlaying,
                ),
              ],
            ),
          ),
          const SizedBox(height: 8),
          Text(
            isPlaying
                ? 'Usa le frecce per cambiare direzione'
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

  Widget _buildDirectionButton(
    IconData icon,
    String direction,
    bool isActive,
    bool enabled,
  ) {
    final color = Colors.lime;
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: enabled ? () => _onSnakeDirectionTap(direction) : null,
        borderRadius: BorderRadius.circular(16),
        child: Container(
          width: 60,
          height: 60,
          decoration: BoxDecoration(
            color: isActive
                ? color.withValues(alpha: 0.4)
                : color.withValues(alpha: enabled ? 0.15 : 0.05),
            borderRadius: BorderRadius.circular(16),
            border: Border.all(
              color: isActive
                  ? color
                  : color.withValues(alpha: enabled ? 0.3 : 0.1),
              width: isActive ? 3 : 1,
            ),
            boxShadow: isActive
                ? [
                    BoxShadow(
                      color: color.withValues(alpha: 0.4),
                      blurRadius: 8,
                      spreadRadius: 1,
                    ),
                  ]
                : null,
          ),
          child: Icon(
            icon,
            color: enabled ? color : Colors.grey[600],
            size: 36,
          ),
        ),
      ),
    );
  }

  IconData _getDirectionIcon(String direction) {
    switch (direction) {
      case 'up':
        return Icons.north;
      case 'down':
        return Icons.south;
      case 'left':
        return Icons.west;
      case 'right':
        return Icons.east;
      default:
        return Icons.east;
    }
  }

  Widget _buildSnakeControlsCard() {
    final state = _snakeState;
    final isWaiting = state?.isWaiting ?? true;
    final isPlaying = state?.isPlaying ?? false;
    final isPaused = state?.isPaused ?? false;
    final isGameOver = state?.isGameOver ?? false;

    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(
                Icons.gamepad,
                color: Colors.lime[400],
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
              if (isWaiting || isGameOver)
                Expanded(
                  child: ActionButton(
                    icon: Icons.play_arrow,
                    label: 'Start',
                    color: Colors.lime[600],
                    enabled: _snakeJoined,
                    onTap: () {
                      widget.deviceService.snakeStart();
                      HapticFeedback.mediumImpact();
                    },
                  ),
                ),
              if (isPlaying)
                Expanded(
                  child: ActionButton(
                    icon: Icons.pause,
                    label: 'Pausa',
                    color: Colors.orange[400],
                    onTap: () {
                      widget.deviceService.snakePause();
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              if (isPaused)
                Expanded(
                  child: ActionButton(
                    icon: Icons.play_arrow,
                    label: 'Riprendi',
                    color: Colors.lime[600],
                    onTap: () {
                      widget.deviceService.snakeResume();
                      HapticFeedback.lightImpact();
                    },
                  ),
                ),
              const SizedBox(width: 12),
              Expanded(
                child: ActionButton(
                  icon: Icons.restart_alt,
                  label: 'Reset',
                  color: Colors.red[400],
                  onTap: () {
                    widget.deviceService.snakeReset();
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

  void _snakeJoin() {
    widget.deviceService.snakeJoin();
    setState(() => _snakeJoined = true);
    HapticFeedback.mediumImpact();
  }

  void _snakeLeave() {
    widget.deviceService.snakeLeave();
    setState(() => _snakeJoined = false);
    HapticFeedback.lightImpact();
  }

  void _onSnakeDirectionTap(String direction) {
    widget.deviceService.snakeSetDirection(direction);
    HapticFeedback.lightImpact();
  }

  // ═══════════════════════════════════════════
  // UTILS
  // ═══════════════════════════════════════════

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
