const LEVEL_CLASSES = {
  stable: 'intel-feed__meta--stable',
  alert: 'intel-feed__meta--alert',
  critical: 'intel-feed__meta--critical',
};

export class LogFeed {
  constructor(listElement, options = {}) {
    this.listElement = listElement;
    this.maxEntries = options.maxEntries ?? 12;
  }

  push(entry) {
    const item = document.createElement('li');
    item.className = 'intel-feed__item';

    const meta = document.createElement('div');
    meta.className = 'intel-feed__meta';
    if (entry.level && LEVEL_CLASSES[entry.level]) {
      meta.classList.add(LEVEL_CLASSES[entry.level]);
    }
    meta.textContent = `${entry.timestamp} // ${entry.channel}`;

    const content = document.createElement('div');
    content.className = 'intel-feed__content';
    content.textContent = entry.message;

    item.append(meta, content);
    this.listElement.prepend(item);

    const overflow = this.listElement.children.length - this.maxEntries;
    for (let i = 0; i < overflow; i += 1) {
      this.listElement.lastElementChild?.remove();
    }
  }
}
