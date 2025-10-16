const STATE_MAP = {
  stable: {
    label: 'STABLE',
    detail: 'Maintaining optimal efficiency',
  },
  alert: {
    label: 'ALERT',
    detail: 'Investigating anomaly signatures',
  },
  critical: {
    label: 'CRITICAL',
    detail: 'Immediate response required',
  },
  offline: {
    label: 'OFFLINE',
    detail: 'Systems in recovery protocol',
  },
};

export class StatusBoard {
  constructor(listElement) {
    this.listElement = listElement;
    this.units = new Map();

    this.listElement.querySelectorAll('.status-list__item').forEach((item) => {
      const unit = item.dataset.unit;
      this.units.set(unit, {
        element: item,
        pulse: item.querySelector('.status-pulse'),
        state: item.querySelector('.status-list__state'),
        detail: item.querySelector('.status-list__detail'),
      });
    });
  }

  update(unit, state, detailOverride) {
    const entry = this.units.get(unit);
    if (!entry) {
      return;
    }

    const resolvedState = STATE_MAP[state] ? state : 'stable';
    const status = STATE_MAP[resolvedState];
    entry.pulse.dataset.state = resolvedState;
    entry.state.textContent = status.label;
    entry.detail.textContent = detailOverride ?? status.detail;
  }

  batchUpdate(updates) {
    updates.forEach((config) => {
      this.update(config.unit, config.state, config.detail);
    });
  }
}
