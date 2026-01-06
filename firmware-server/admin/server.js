const express = require('express');
const multer = require('multer');
const basicAuth = require('basic-auth');
const cors = require('cors');
const fs = require('fs').promises;
const path = require('path');
const crypto = require('crypto');
const { execSync } = require('child_process');

const app = express();
const PORT = process.env.PORT || 3000;
const BINARIES_DIR = '/app/binaries';

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// Basic Auth middleware
const auth = (req, res, next) => {
  const credentials = basicAuth(req);
  const username = process.env.ADMIN_USERNAME || 'admin';
  const password = process.env.ADMIN_PASSWORD || 'changeme';

  if (!credentials || credentials.name !== username || credentials.pass !== password) {
    res.set('WWW-Authenticate', 'Basic realm="Firmware Admin"');
    return res.status(401).json({ error: 'Authentication required' });
  }

  next();
};

// Configure multer for file uploads
const storage = multer.diskStorage({
  destination: async (req, file, cb) => {
    const version = req.body.version;
    if (!version) {
      return cb(new Error('Version is required'));
    }

    const versionDir = path.join(BINARIES_DIR, version);
    await fs.mkdir(versionDir, { recursive: true });
    cb(null, versionDir);
  },
  filename: (req, file, cb) => {
    cb(null, 'firmware.bin');
  }
});

const upload = multer({
  storage,
  limits: { fileSize: 10 * 1024 * 1024 }, // 10MB max
  fileFilter: (req, file, cb) => {
    if (file.originalname.endsWith('.bin')) {
      cb(null, true);
    } else {
      cb(new Error('Only .bin files are allowed'));
    }
  }
});

// Helper: Generate manifest.json
async function generateManifest() {
  try {
    const versions = [];
    const entries = await fs.readdir(BINARIES_DIR, { withFileTypes: true });

    for (const entry of entries) {
      if (!entry.isDirectory() || entry.name.startsWith('.')) continue;

      const version = entry.name;
      const versionDir = path.join(BINARIES_DIR, version);
      const firmwarePath = path.join(versionDir, 'firmware.bin');

      try {
        const stats = await fs.stat(firmwarePath);
        const buffer = await fs.readFile(firmwarePath);
        const md5 = crypto.createHash('md5').update(buffer).digest('hex');

        // Parse version (format: X.Y.Z+N)
        const match = version.match(/^(\d+\.\d+\.\d+)\+(\d+)$/);
        if (!match) {
          console.warn(`Invalid version format: ${version}`);
          continue;
        }

        const [, versionString, buildNumber] = match;

        versions.push({
          version: versionString,
          buildNumber: parseInt(buildNumber, 10),
          fullVersion: version,
          url: `/binaries/${version}/firmware.bin`,
          size: stats.size,
          md5: md5,
          releaseDate: stats.mtime.toISOString()
        });
      } catch (err) {
        console.warn(`Skipping ${version}: ${err.message}`);
      }
    }

    // Sort by build number (newest first)
    versions.sort((a, b) => b.buildNumber - a.buildNumber);

    const manifest = {
      versions,
      generatedAt: new Date().toISOString()
    };

    await fs.writeFile(
      path.join(BINARIES_DIR, 'manifest.json'),
      JSON.stringify(manifest, null, 2)
    );

    console.log(`✓ Manifest generated with ${versions.length} versions`);
    return manifest;
  } catch (err) {
    console.error('Error generating manifest:', err);
    throw err;
  }
}

// Routes

// GET / - Admin UI
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// GET /api/versions - List all versions (with auth)
app.get('/api/versions', auth, async (req, res) => {
  try {
    const manifestPath = path.join(BINARIES_DIR, 'manifest.json');
    const manifest = JSON.parse(await fs.readFile(manifestPath, 'utf8'));
    res.json(manifest);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// POST /api/upload - Upload new firmware (with auth)
app.post('/api/upload', auth, upload.single('firmware'), async (req, res) => {
  try {
    const { version } = req.body;

    if (!version) {
      return res.status(400).json({ error: 'Version is required' });
    }

    // Validate version format
    if (!/^\d+\.\d+\.\d+\+\d+$/.test(version)) {
      return res.status(400).json({
        error: 'Invalid version format. Use: X.Y.Z+N (e.g., 1.0.0+1)'
      });
    }

    console.log(`✓ Uploaded firmware ${version}`);

    // Regenerate manifest
    const manifest = await generateManifest();

    res.json({
      success: true,
      version,
      file: req.file.filename,
      size: req.file.size,
      manifest
    });
  } catch (err) {
    console.error('Upload error:', err);
    res.status(500).json({ error: err.message });
  }
});

// DELETE /api/versions/:version - Delete a version (with auth)
app.delete('/api/versions/:version', auth, async (req, res) => {
  try {
    const { version } = req.params;
    const versionDir = path.join(BINARIES_DIR, version);

    await fs.rm(versionDir, { recursive: true, force: true });
    console.log(`✓ Deleted version ${version}`);

    // Regenerate manifest
    const manifest = await generateManifest();

    res.json({ success: true, version, manifest });
  } catch (err) {
    console.error('Delete error:', err);
    res.status(500).json({ error: err.message });
  }
});

// POST /api/regenerate-manifest - Manually regenerate manifest (with auth)
app.post('/api/regenerate-manifest', auth, async (req, res) => {
  try {
    const manifest = await generateManifest();
    res.json({ success: true, manifest });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Start server
app.listen(PORT, async () => {
  console.log(`🚀 Admin server running on port ${PORT}`);
  console.log(`📁 Binaries directory: ${BINARIES_DIR}`);
  console.log(`🔐 Username: ${process.env.ADMIN_USERNAME || 'admin'}`);
  console.log(`🔐 Password: ${process.env.ADMIN_PASSWORD || 'changeme'}`);

  // Generate initial manifest
  try {
    await generateManifest();
  } catch (err) {
    console.warn('Could not generate initial manifest:', err.message);
  }
});
