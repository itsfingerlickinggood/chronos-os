import { Sparkline } from './components/sparkline.js';
import { StatusBoard } from './components/statusBoard.js';
import { CircularMeter } from './components/circularMeter.js';
import { LogFeed } from './components/logFeed.js';
import { ProgressBoard } from './components/progressBoard.js';
import { TelemetryGrid } from './components/telemetryGrid.js';

const THREAT_VECTORS = [
  'GOTHAM-SKYLINE',
  'FINANCIAL-DISTRICT',
  'NARROWS-DELTA',
  'WAYNETECH SAT-LINK',
  'ACE-CHEMICALS PERIMETER',
  'BLACKGATE NODE-7',
];

const STATUS_DETAIL_PRESETS = {
  'Sentinel Grid': [
    'Triangulating anomalies',
    'Thermal resonance scanning',
    'Infrared sweeps synchronized',
    'Adaptive focus locked',
  ],
  'Aerial Recon': [
    'Thermal sweeps engaged',
    'Drone uplink stabilized',
    'Altitude vector holding',
    'Microburst filters active',
  ],
  'Cipher Division': [
    'Quantum lattice decrypting',
    'Cipher cores recalibrating',
    'Entropy shielding engaged',
    'Post-quantum matrix steady',
  ],
  'Field Operatives': [
    'Silent protocol sweep',
    'Shadow formation tracking',
    'Ground teams repositioning',
    'Surveillance relay secured',
  ],
};

const LOG_CHANNELS = ['NARROWS NODE', 'GCPD GRID', 'ORACLE FEED', 'OSIRIS ARRAY', 'WATCHTOWER'];
const LOG_EVENTS = [
  { level: 'critical', message: 'Unidentified heat bloom detected near financial core.' },
  { level: 'alert', message: 'Encrypted chatter spike intercepted. Pattern analysts engaged.' },
  { level: 'stable', message: 'Perimeter drones cycling without deviation. No anomalies.' },
  { level: 'alert', message: 'GCPD tactical requesting updated schematics. Dispatching overlays.' },
  { level: 'critical', message: 'Power fluctuation within Narrows sub-grid. Containment response deployed.' },
  { level: 'alert', message: 'Acoustic triangulation marking new contact. Confidence 67%.' },
  { level: 'stable', message: 'Satellite uplink latency normalized across all channels.' },
];

const clamp = (value, min, max) => Math.max(min, Math.min(max, value));
const randomBetween = (min, max) => Math.random() * (max - min) + min;
const randomItem = (arr) => arr[Math.floor(Math.random() * arr.length)];

const weightedStatus = () => {
  const roll = Math.random();
  if (roll > 0.92) {
    return 'critical';
  }
  if (roll > 0.68) {
    return 'alert';
  }
  return 'stable';
};

const resolveActivityState = (value) => {
  if (value >= 72) {
    return 'stable';
  }
  if (value <= 38) {
    return 'critical';
  }
  return 'alert';
};

function formatTime(date) {
  return `${String(date.getUTCHours()).padStart(2, '0')}:${String(date.getUTCMinutes()).padStart(2, '0')}:${String(date.getUTCSeconds()).padStart(2, '0')} UTC`;
}

function determineThreatState(value) {
  if (value >= 78) {
    return { state: 'critical', label: 'Critical' };
  }
  if (value >= 58) {
    return { state: 'alert', label: 'Elevated' };
  }
  return { state: 'stable', label: 'Stable' };
}

document.addEventListener('DOMContentLoaded', () => {
  const chart = new Sparkline(document.getElementById('threat-chart'));
  const statusBoard = new StatusBoard(document.getElementById('asset-status'));
  const logFeed = new LogFeed(document.getElementById('intel-feed-list'));
  const progressBoard = new ProgressBoard(document.getElementById('response-progress'));
  const telemetryGrid = new TelemetryGrid(document.getElementById('telemetry-grid'));

  const meters = {
    signal: new CircularMeter(document.querySelector('[data-meter="signal"]')),
    encryption: new CircularMeter(document.querySelector('[data-meter="encryption"]')),
    containment: new CircularMeter(document.querySelector('[data-meter="containment"]')),
  };

  const timeHud = document.getElementById('hud-time');
  const densityHud = document.getElementById('hud-density');
  const threatHud = document.getElementById('hud-threat');
  const threatChip = document.getElementById('threat-state');
  const riskVectorEl = document.getElementById('risk-vector');
  const riskVarianceEl = document.getElementById('risk-variance');
  const riskConfidenceEl = document.getElementById('risk-confidence');

  let threatValue = 72 + Math.random() * 6;
  const seedCount = chart.bufferSize ?? 120;
  for (let i = 0; i < seedCount; i += 1) {
    const seedValue = clamp(threatValue + Math.sin(i / 6) * 3 + (Math.random() - 0.5) * 4, 42, 96);
    chart.push(seedValue);
  }

  const updateTime = () => {
    const now = new Date();
    timeHud.textContent = formatTime(now);
  };

  const updateThreatVector = () => {
    threatValue = clamp(threatValue + (Math.random() - 0.5) * 6, 38, 96);
    chart.push(threatValue);

    const variance = randomBetween(1.6, 7.4);
    const state = determineThreatState(threatValue);
    const density = randomBetween(0.72, 1.28);
    const confidence = clamp(Math.round(randomBetween(58, 94)), 0, 100);
    const vector = randomItem(THREAT_VECTORS);

    threatHud.textContent = `${threatValue.toFixed(1)}%`;
    threatHud.dataset.state = state.state;
    densityHud.textContent = density.toFixed(2);
    threatChip.textContent = state.label;
    threatChip.className = `status-chip status-chip--${state.state}`;
    riskVectorEl.textContent = vector;
    riskVarianceEl.textContent = `${variance.toFixed(2)}`;
    riskConfidenceEl.textContent = `${confidence}%`;
  };

  const updateAssetStatuses = () => {
    const updates = Object.keys(STATUS_DETAIL_PRESETS).map((unit) => {
      const state = weightedStatus();
      const detail = randomItem(STATUS_DETAIL_PRESETS[unit]);
      return { unit, state, detail };
    });
    statusBoard.batchUpdate(updates);
  };

  const updateMeters = () => {
    meters.signal.update(randomBetween(65, 98));
    meters.encryption.update(randomBetween(54, 97));
    meters.containment.update(randomBetween(48, 92));
  };

  const updateProgress = () => {
    const updateTeam = (team, range) => {
      const value = randomBetween(range[0], range[1]);
      const state = resolveActivityState(value);
      progressBoard.update(team, value, state);
    };

    updateTeam('Vector-01', [52, 100]);
    updateTeam('Vector-02', [28, 92]);
    updateTeam('Vector-07', [36, 88]);
  };

  const pushLogEntry = () => {
    const event = randomItem(LOG_EVENTS);
    const timestamp = formatTime(new Date());
    logFeed.push({
      timestamp,
      channel: randomItem(LOG_CHANNELS),
      level: event.level,
      message: event.message,
    });
  };

  updateTime();
  updateThreatVector();
  updateAssetStatuses();
  updateMeters();
  updateProgress();
  telemetryGrid.update();
  pushLogEntry();
  pushLogEntry();

  setInterval(updateTime, 1000);
  setInterval(updateThreatVector, 1300);
  setInterval(updateAssetStatuses, 4800);
  setInterval(updateMeters, 2600);
  setInterval(updateProgress, 3900);
  setInterval(() => telemetryGrid.update(), 4300);
  setInterval(pushLogEntry, 4200);
});
