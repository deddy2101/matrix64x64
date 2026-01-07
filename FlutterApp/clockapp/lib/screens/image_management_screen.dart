import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import '../services/device_service.dart';
import '../services/image_service.dart';

class ImageManagementScreen extends StatefulWidget {
  final DeviceService deviceService;

  const ImageManagementScreen({
    super.key,
    required this.deviceService,
  });

  @override
  State<ImageManagementScreen> createState() => _ImageManagementScreenState();
}

class _ImageManagementScreenState extends State<ImageManagementScreen> {
  late ImageService _imageService;
  List<StoredImage> _images = [];
  ImageStorageInfo? _storageInfo;
  bool _loading = false;
  String _status = '';

  @override
  void initState() {
    super.initState();
    _imageService = ImageService(widget.deviceService);
    _imageService.statusStream.listen((status) {
      if (mounted) {
        setState(() => _status = status);
      }
    });
    _refreshImages();
  }

  @override
  void dispose() {
    _imageService.dispose();
    super.dispose();
  }

  Future<void> _refreshImages() async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    setState(() => _loading = true);

    try {
      final images = await _imageService.listImages();
      final storageInfo = await _imageService.getStorageInfo();

      if (mounted) {
        setState(() {
          _images = images;
          _storageInfo = storageInfo;
          _loading = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() => _loading = false);
        _showSnackBar('Errore caricamento: $e');
      }
    }
  }

  Future<void> _uploadImage() async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    try {
      // Pick image file
      final result = await FilePicker.platform.pickFiles(
        type: FileType.image,
        allowMultiple: false,
      );

      if (result == null || result.files.isEmpty) {
        return;
      }

      final file = result.files.first;
      final bytes = file.bytes;

      if (bytes == null) {
        _showSnackBar('Impossibile leggere il file');
        return;
      }

      // Get image name
      String? imageName = await _showNameDialog(file.name);
      if (imageName == null || imageName.isEmpty) {
        return;
      }

      // Remove extension and sanitize
      imageName = imageName.split('.').first.replaceAll(RegExp(r'[^a-zA-Z0-9_-]'), '_');

      setState(() => _loading = true);

      // Upload
      final success = await _imageService.uploadImage(imageName, bytes);

      if (success) {
        _showSnackBar('Immagine caricata!');
        await _refreshImages();
      } else {
        _showSnackBar('Errore upload: ${_imageService.status}');
      }

    } catch (e) {
      _showSnackBar('Errore: $e');
    } finally {
      if (mounted) {
        setState(() => _loading = false);
      }
    }
  }

  Future<void> _deleteImage(StoredImage image) async {
    final confirm = await _showConfirmDialog(
      'Elimina immagine',
      'Vuoi eliminare "${image.name}"?',
    );

    if (confirm != true) return;

    setState(() => _loading = true);

    final success = await _imageService.deleteImage(image.name);

    if (success) {
      _showSnackBar('Immagine eliminata');
      await _refreshImages();
    } else {
      _showSnackBar('Errore eliminazione');
      setState(() => _loading = false);
    }
  }

  Future<void> _showImageOnMatrix(StoredImage image) async {
    if (!widget.deviceService.isConnected) {
      _showSnackBar('Dispositivo non connesso');
      return;
    }

    // Prima, switcha all'effetto "Images" (DynamicImageEffect)
    // Cerca l'effetto nella lista
    final effects = widget.deviceService.lastEffects;
    if (effects == null) {
      _showSnackBar('Lista effetti non disponibile');
      return;
    }

    // Trova l'indice dell'effetto che contiene "Images"
    int imageEffectIndex = -1;
    for (int i = 0; i < effects.length; i++) {
      if (effects[i].name.toLowerCase().contains('images:')) {
        imageEffectIndex = i;
        break;
      }
    }

    if (imageEffectIndex == -1) {
      _showSnackBar('Effetto "Images" non trovato. Ricarica gli effetti.');
      return;
    }

    // Switcha all'effetto Images
    widget.deviceService.send('effect,select,$imageEffectIndex');

    // Mostra feedback
    _showSnackBar('Mostrando ${image.name} sulla matrice...');

    // Chiudi la schermata e torna alla home
    Navigator.pop(context);
  }

  Future<String?> _showNameDialog(String defaultName) async {
    final controller = TextEditingController(text: defaultName.split('.').first);

    return showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Nome immagine'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(
            labelText: 'Nome',
            hintText: 'es: mario',
          ),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Annulla'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, controller.text),
            child: const Text('OK'),
          ),
        ],
      ),
    );
  }

  Future<bool?> _showConfirmDialog(String title, String message) {
    return showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: Text(message),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Annulla'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('Elimina'),
          ),
        ],
      ),
    );
  }

  void _showSnackBar(String message) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Gestione Immagini'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loading ? null : _refreshImages,
          ),
        ],
      ),
      body: Column(
        children: [
          // Storage info
          if (_storageInfo != null)
            Container(
              padding: const EdgeInsets.all(16),
              color: Theme.of(context).colorScheme.surfaceContainerHighest,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Storage ESP32',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 8),
                  LinearProgressIndicator(
                    value: _storageInfo!.usedPercent / 100,
                    backgroundColor: Colors.grey[300],
                    minHeight: 8,
                  ),
                  const SizedBox(height: 8),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Text(
                        'Usato: ${ImageService.formatBytes(_storageInfo!.usedBytes)}',
                        style: TextStyle(color: Colors.grey[700]),
                      ),
                      Text(
                        'Libero: ${ImageService.formatBytes(_storageInfo!.freeBytes)}',
                        style: TextStyle(color: Colors.grey[700]),
                      ),
                      Text(
                        'Totale: ${ImageService.formatBytes(_storageInfo!.totalBytes)}',
                        style: TextStyle(color: Colors.grey[700]),
                      ),
                    ],
                  ),
                ],
              ),
            ),

          // Status message
          if (_status.isNotEmpty)
            Container(
              padding: const EdgeInsets.all(12),
              color: Theme.of(context).colorScheme.primaryContainer,
              child: Row(
                children: [
                  if (_loading)
                    const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      _status,
                      style: TextStyle(
                        color: Theme.of(context).colorScheme.onPrimaryContainer,
                      ),
                    ),
                  ),
                ],
              ),
            ),

          // Images list
          Expanded(
            child: _loading && _images.isEmpty
                ? const Center(child: CircularProgressIndicator())
                : _images.isEmpty
                    ? Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(
                              Icons.image_not_supported,
                              size: 64,
                              color: Colors.grey[400],
                            ),
                            const SizedBox(height: 16),
                            Text(
                              'Nessuna immagine',
                              style: TextStyle(
                                fontSize: 18,
                                color: Colors.grey[600],
                              ),
                            ),
                            const SizedBox(height: 8),
                            Text(
                              'Carica un\'immagine per iniziare',
                              style: TextStyle(
                                color: Colors.grey[500],
                              ),
                            ),
                          ],
                        ),
                      )
                    : ListView.builder(
                        itemCount: _images.length,
                        itemBuilder: (context, index) {
                          final image = _images[index];
                          return Card(
                            margin: const EdgeInsets.symmetric(
                              horizontal: 16,
                              vertical: 8,
                            ),
                            child: ListTile(
                              leading: CircleAvatar(
                                backgroundColor: Theme.of(context).colorScheme.primaryContainer,
                                child: Icon(
                                  Icons.image,
                                  color: Theme.of(context).colorScheme.onPrimaryContainer,
                                ),
                              ),
                              title: Text(
                                image.name,
                                style: const TextStyle(
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              subtitle: Text(
                                'Dimensione: ${ImageService.formatBytes(image.size)} (64x64 RGB565)',
                              ),
                              trailing: Row(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  IconButton(
                                    icon: const Icon(Icons.visibility),
                                    color: Theme.of(context).colorScheme.primary,
                                    tooltip: 'Mostra sulla matrice',
                                    onPressed: _loading ? null : () => _showImageOnMatrix(image),
                                  ),
                                  IconButton(
                                    icon: const Icon(Icons.delete),
                                    color: Colors.red,
                                    tooltip: 'Elimina',
                                    onPressed: _loading ? null : () => _deleteImage(image),
                                  ),
                                ],
                              ),
                            ),
                          );
                        },
                      ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _loading ? null : _uploadImage,
        icon: const Icon(Icons.add_photo_alternate),
        label: const Text('Carica Immagine'),
      ),
    );
  }
}
