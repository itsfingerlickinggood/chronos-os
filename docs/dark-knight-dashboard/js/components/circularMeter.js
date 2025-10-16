const STATE_COLORS = {
  critical: 'var(--status-critical)',
  alert: 'var(--status-alert)',
  stable: 'var(--status-stable)',
};

export class CircularMeter {
  constructor(element, options = {}) {
    this.element = element;
    this.fill = element.querySelector('.dial__fill');
    this.valueEl = element.querySelector('.dial__value');
    this.min = options.min ?? 0;
    this.max = options.max ?? 100;
  }

  update(value) {
    const clamped = Math.max(this.min, Math.min(this.max, value));
    const ratio = (clamped - this.min) / (this.max - this.min || 1);
    const angle = Math.round(ratio * 360);
    const state = this.resolveState(ratio);
    const color = STATE_COLORS[state];

    this.fill.style.background = `conic-gradient(from -90deg, ${color} ${angle}deg, transparent ${angle}deg 360deg)`;
    this.valueEl.textContent = `${Math.round(clamped)}%`;
    this.valueEl.dataset.state = state;
    this.element.dataset.state = state;
  }

  resolveState(ratio) {
    if (ratio >= 0.72) {
      return 'stable';
    }
    if (ratio >= 0.45) {
      return 'alert';
    }
    return 'critical';
  }
}
