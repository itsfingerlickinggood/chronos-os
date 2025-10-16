export class Sparkline {
  constructor(canvas, options = {}) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d', { alpha: false });
    this.samples = [];
    this.bufferSize = options.bufferSize ?? 120;
    this.lineColor = options.lineColor ?? 'rgba(49, 198, 255, 1)';
    this.glowColor = options.glowColor ?? 'rgba(49, 198, 255, 0.45)';
    this.baselineColor = options.baselineColor ?? 'rgba(255, 255, 255, 0.15)';
    this.gridColor = options.gridColor ?? 'rgba(255, 255, 255, 0.08)';

    this.handleResize = this.handleResize.bind(this);
    window.addEventListener('resize', this.handleResize, { passive: true });
    this.handleResize();
  }

  handleResize() {
    const dpr = window.devicePixelRatio || 1;
    const rect = this.canvas.getBoundingClientRect();
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    this.ctx.setTransform(1, 0, 0, 1, 0, 0);
    this.ctx.scale(dpr, dpr);
    this.draw();
  }

  push(value) {
    this.samples.push(value);
    if (this.samples.length > this.bufferSize) {
      this.samples.shift();
    }
    this.draw();
  }

  draw() {
    const { width, height } = this.canvas.getBoundingClientRect();
    this.ctx.clearRect(0, 0, width, height);

    if (!this.samples.length) {
      return;
    }

    const min = Math.min(...this.samples);
    const max = Math.max(...this.samples);
    const range = max - min || 1;

    this.drawBackground(width, height);

    this.ctx.beginPath();
    const length = this.samples.length;
    const denom = Math.max(1, length - 1);
    this.samples.forEach((sample, index) => {
      const x = (index / denom) * width;
      const y = height - ((sample - min) / range) * height;
      if (index === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
    });

    this.ctx.strokeStyle = this.lineColor;
    this.ctx.lineWidth = 2;
    this.ctx.shadowBlur = 16;
    this.ctx.shadowColor = this.glowColor;
    this.ctx.stroke();
    this.ctx.shadowBlur = 0;
  }

  drawBackground(width, height) {
    const ctx = this.ctx;
    ctx.save();
    ctx.strokeStyle = this.gridColor;
    ctx.lineWidth = 1;
    ctx.setLineDash([6, 8]);

    const horizontalLines = 4;
    for (let i = 1; i < horizontalLines; i += 1) {
      const y = (height / horizontalLines) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }

    ctx.restore();

    ctx.save();
    ctx.strokeStyle = this.baselineColor;
    ctx.lineWidth = 1.5;
    ctx.setLineDash([]);
    ctx.beginPath();
    ctx.moveTo(0, height * 0.8);
    ctx.lineTo(width, height * 0.8);
    ctx.stroke();
    ctx.restore();
  }
}
