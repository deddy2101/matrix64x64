import 'package:flutter/material.dart';

/// Schermata FAQ con domande frequenti sull'app e sul dispositivo
class FaqScreen extends StatelessWidget {
  const FaqScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0A0A0F),
      appBar: AppBar(
        backgroundColor: const Color(0xFF121218),
        title: const Text('FAQ'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _buildHeader(),
          const SizedBox(height: 24),
          ..._faqItems.map((item) => _FaqTile(
            question: item['question']!,
            answer: item['answer']!,
            icon: item['icon'] as IconData,
          )),
          const SizedBox(height: 32),
          _buildContactSection(context),
        ],
      ),
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [
            const Color(0xFF8B5CF6).withValues(alpha: 0.2),
            const Color(0xFF8B5CF6).withValues(alpha: 0.05),
          ],
        ),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: const Color(0xFF8B5CF6).withValues(alpha: 0.3),
        ),
      ),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: const Color(0xFF8B5CF6).withValues(alpha: 0.2),
              borderRadius: BorderRadius.circular(12),
            ),
            child: const Icon(
              Icons.help_outline,
              color: Color(0xFF8B5CF6),
              size: 28,
            ),
          ),
          const SizedBox(width: 16),
          const Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Domande Frequenti',
                  style: TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 4),
                Text(
                  'Trova risposte alle domande più comuni',
                  style: TextStyle(
                    color: Colors.grey,
                    fontSize: 14,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildContactSection(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF1a1a2e),
        borderRadius: BorderRadius.circular(16),
      ),
      child: Column(
        children: [
          Icon(
            Icons.support_agent,
            size: 48,
            color: Colors.grey[600],
          ),
          const SizedBox(height: 16),
          const Text(
            'Non hai trovato la risposta?',
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Contattaci per ulteriore assistenza',
            style: TextStyle(
              color: Colors.grey[400],
              fontSize: 14,
            ),
          ),
          const SizedBox(height: 16),
          Text(
            'support@ledmatrix.app',
            style: TextStyle(
              color: const Color(0xFF8B5CF6),
              fontSize: 14,
            ),
          ),
        ],
      ),
    );
  }

  static final List<Map<String, dynamic>> _faqItems = [
    {
      'icon': Icons.search_off,
      'question': 'Perché l\'app non trova il mio dispositivo?',
      'answer': '''Assicurati che:

• L'ESP32 sia acceso e connesso alla stessa rete WiFi del tuo telefono/computer
• Il dispositivo e l'app siano sulla stessa sottorete (es. 192.168.1.x)
• Il firewall non stia bloccando le comunicazioni UDP sulla porta 5555
• Su Android: concedi i permessi di posizione (necessari per la scansione di rete)

Prova a riavviare sia l'ESP32 che l'app e ripeti la scansione.''',
    },
    {
      'icon': Icons.wifi_off,
      'question': 'Come configuro il WiFi dell\'ESP32?',
      'answer': '''Al primo avvio, l'ESP32 crea un Access Point chiamato "LEDMatrix-XXXX".

1. Connettiti a questo AP dal tuo telefono
2. Apri l'app e connettiti al dispositivo
3. Vai nelle impostazioni WiFi e inserisci SSID e password della tua rete
4. Salva e riavvia il dispositivo

L'ESP32 si connetterà automaticamente alla tua rete WiFi.''',
    },
    {
      'icon': Icons.play_circle_outline,
      'question': 'Cos\'è la modalità Demo?',
      'answer': '''La modalità Demo ti permette di esplorare tutte le funzionalità dell'app senza avere un dispositivo LED Matrix fisico.

È utile per:
• Familiarizzare con l'interfaccia prima di acquistare l'hardware
• Testare l'app durante lo sviluppo
• Dimostrare le funzionalità a potenziali utenti

In modalità Demo, i dati sono simulati e nessun comando viene inviato a dispositivi reali.''',
    },
    {
      'icon': Icons.auto_awesome,
      'question': 'Come cambio gli effetti visuali?',
      'answer': '''Dalla schermata principale:

1. Trova la sezione "Effetti"
2. Tocca uno degli effetti disponibili per attivarlo immediatamente
3. Usa i controlli "Pausa/Play/Next" per gestire la riproduzione
4. Attiva "Auto-switch" per cambiare automaticamente effetto ogni X secondi

Puoi configurare la durata dell'auto-switch nelle impostazioni.''',
    },
    {
      'icon': Icons.brightness_6,
      'question': 'Come funziona la luminosità giorno/notte?',
      'answer': '''Il dispositivo supporta due livelli di luminosità automatici:

• Luminosità Giorno: utilizzata durante le ore diurne (default: 200)
• Luminosità Notte: utilizzata durante le ore notturne (default: 30)

Puoi configurare:
- I valori di luminosità per entrambe le modalità
- L'orario di inizio e fine della modalità notturna

L'ESP32 cambia automaticamente luminosità in base all'ora.''',
    },
    {
      'icon': Icons.access_time,
      'question': 'Come sincronizzo l\'ora?',
      'answer': '''Hai diverse opzioni:

1. Sincronizza ora: Imposta l'ora del dispositivo all'ora corrente del tuo telefono
2. Imposta manualmente: Scegli un'ora specifica

Se il tuo ESP32 ha un modulo DS3231 (RTC), l'ora viene mantenuta anche dopo lo spegnimento. Altrimenti, dovrai risincronizzare ad ogni riavvio.

La modalità "RTC" usa il modulo hardware, "Fake" simula l'avanzamento del tempo via software.''',
    },
    {
      'icon': Icons.usb,
      'question': 'Posso connettermi via USB?',
      'answer': '''Sì, su desktop (Linux, macOS, Windows) puoi connetterti via porta seriale USB.

1. Collega l'ESP32 al computer via USB
2. L'app mostrerà automaticamente le porte seriali disponibili
3. Seleziona la porta corretta (es. /dev/ttyUSB0 su Linux)

Questa modalità è utile per:
• Debug e sviluppo
• Quando non hai accesso WiFi
• Configurazione iniziale''',
    },
    {
      'icon': Icons.memory,
      'question': 'Quali sono i requisiti hardware?',
      'answer': '''Hardware necessario:

• ESP32 (consigliato: ESP32-WROOM-32)
• Display LED Matrix 64x64 (driver HUB75)
• Alimentatore 5V adeguato (almeno 4A per matrice completa)
• Opzionale: Modulo RTC DS3231 per mantenere l'ora

Il firmware è open source e disponibile su GitHub.''',
    },
    {
      'icon': Icons.smartphone,
      'question': 'Su quali piattaforme funziona l\'app?',
      'answer': '''L'app è disponibile per:

• Android 6.0+ (API 23+)
• iOS 12.0+
• Linux (x64, ARM64)
• macOS 10.14+
• Windows 10+

Su mobile (Android/iOS) la connessione avviene solo via WiFi.
Su desktop puoi usare anche la connessione seriale USB.''',
    },
    {
      'icon': Icons.security,
      'question': 'Perché l\'app richiede permessi di posizione?',
      'answer': '''Su Android, i permessi di posizione sono richiesti da Google per effettuare scansioni di rete WiFi.

L'app NON:
• Traccia la tua posizione
• Invia dati a server esterni
• Raccoglie informazioni personali

I permessi sono usati esclusivamente per trovare dispositivi LED Matrix sulla tua rete locale.''',
    },
  ];
}

class _FaqTile extends StatefulWidget {
  final String question;
  final String answer;
  final IconData icon;

  const _FaqTile({
    required this.question,
    required this.answer,
    required this.icon,
  });

  @override
  State<_FaqTile> createState() => _FaqTileState();
}

class _FaqTileState extends State<_FaqTile> {
  bool _isExpanded = false;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      decoration: BoxDecoration(
        color: const Color(0xFF1a1a2e),
        borderRadius: BorderRadius.circular(12),
        border: _isExpanded
            ? Border.all(color: const Color(0xFF8B5CF6).withValues(alpha: 0.5))
            : null,
      ),
      child: Column(
        children: [
          InkWell(
            onTap: () => setState(() => _isExpanded = !_isExpanded),
            borderRadius: BorderRadius.circular(12),
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Row(
                children: [
                  Container(
                    padding: const EdgeInsets.all(8),
                    decoration: BoxDecoration(
                      color: const Color(0xFF8B5CF6).withValues(alpha: 0.1),
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Icon(
                      widget.icon,
                      color: const Color(0xFF8B5CF6),
                      size: 20,
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      widget.question,
                      style: const TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                  ),
                  AnimatedRotation(
                    turns: _isExpanded ? 0.5 : 0,
                    duration: const Duration(milliseconds: 200),
                    child: Icon(
                      Icons.keyboard_arrow_down,
                      color: Colors.grey[400],
                    ),
                  ),
                ],
              ),
            ),
          ),
          AnimatedCrossFade(
            firstChild: const SizedBox.shrink(),
            secondChild: Container(
              width: double.infinity,
              padding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
              child: Container(
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: Colors.black.withValues(alpha: 0.2),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text(
                  widget.answer,
                  style: TextStyle(
                    color: Colors.grey[300],
                    fontSize: 14,
                    height: 1.5,
                  ),
                ),
              ),
            ),
            crossFadeState: _isExpanded
                ? CrossFadeState.showSecond
                : CrossFadeState.showFirst,
            duration: const Duration(milliseconds: 200),
          ),
        ],
      ),
    );
  }
}
