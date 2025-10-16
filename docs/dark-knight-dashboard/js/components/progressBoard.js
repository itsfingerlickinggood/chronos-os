const STATE_COLOR = {
  stable: 'var(--status-stable)',
  alert: 'var(--status-alert)',
  critical: 'var(--status-critical)',
};

export class ProgressBoard {
  constructor(listElement) {
    this.entries = new Map();
    listElement.querySelectorAll('.progress-list__item').forEach((item) => {
      const team = item.dataset.team;
      this.entries.set(team, {
        element: item,
        bar: item.querySelector('.progress__bar'),
        container: item.querySelector('.progress'),
      });
    });
  }

  update(team, value, state = 'stable') {
    const entry = this.entries.get(team);
    if (!entry) {
      return;
    }

    const clamped = Math.max(0, Math.min(100, value));
    entry.bar.style.width = `${clamped}%`;
    entry.container.setAttribute('aria-valuenow', String(Math.round(clamped)));
    const color = STATE_COLOR[state] ?? STATE_COLOR.stable;
    entry.bar.style.background = color;
    entry.bar.style.boxShadow = `0 0 16px ${color}`;
  }
}
