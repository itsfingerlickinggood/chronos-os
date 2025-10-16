const DEFAULT_METRICS = [
  { label: 'Latency Vector', unit: 'ms', format: (value) => `${value.toFixed(1)} ms`, range: [8, 42] },
  { label: 'Signal Gain', unit: 'dB', format: (value) => `${value.toFixed(2)} dB`, range: [14, 38] },
  { label: 'Quantum Hash Rate', unit: 'TH/s', format: (value) => `${value.toFixed(1)} TH/s`, range: [120, 320] },
  { label: 'Perimeter Integrity', unit: '%', format: (value) => `${Math.round(value)}%`, range: [68, 99] },
  { label: 'Drone Flight Time', unit: 'min', format: (value) => `${value.toFixed(0)} min`, range: [46, 92] },
  { label: 'Containment Field', unit: 'T', format: (value) => `${value.toFixed(2)} T`, range: [2.2, 5.8] },
];

export class TelemetryGrid {
  constructor(container, metrics = DEFAULT_METRICS) {
    this.container = container;
    this.metrics = metrics.map((metric) => ({ ...metric }));
    this.cards = new Map();
    this.initialize();
  }

  initialize() {
    this.metrics.forEach((metric) => {
      const card = document.createElement('article');
      card.className = 'telemetry-card';

      const label = document.createElement('span');
      label.className = 'telemetry-card__label';
      label.textContent = metric.label;

      const valueEl = document.createElement('span');
      valueEl.className = 'telemetry-card__value';
      valueEl.textContent = '--';

      const trend = document.createElement('span');
      trend.className = 'telemetry-card__trend';
      trend.textContent = '—';

      card.append(label, valueEl, trend);
      this.container.append(card);

      this.cards.set(metric.label, {
        valueEl,
        trend,
        previous: null,
        metric,
      });
    });
  }

  update() {
    this.cards.forEach((entry) => {
      const { metric } = entry;
      const value = this.sample(metric.range, entry.previous);
      entry.valueEl.textContent = metric.format ? metric.format(value) : value.toFixed(2);

      const delta = entry.previous != null ? value - entry.previous : 0;
      entry.trend.textContent = this.composeTrend(delta, metric.unit);
      entry.trend.classList.remove('telemetry-card__trend--up', 'telemetry-card__trend--down');
      if (delta > 0.4) {
        entry.trend.classList.add('telemetry-card__trend--up');
      } else if (delta < -0.4) {
        entry.trend.classList.add('telemetry-card__trend--down');
      }

      entry.previous = value;
    });
  }

  sample([min, max], previous) {
    const bias = previous ?? (min + max) / 2;
    const variance = (max - min) * 0.12;
    const next = bias + (Math.random() - 0.5) * variance;
    return Math.max(min, Math.min(max, next));
  }

  composeTrend(delta, unit) {
    if (Math.abs(delta) < 0.3) {
      return '— NEUTRAL';
    }

    const direction = delta > 0 ? '▲' : '▼';
    const magnitude = Math.abs(delta).toFixed(1);
    return `${direction} ${magnitude} ${unit}`;
  }
}
