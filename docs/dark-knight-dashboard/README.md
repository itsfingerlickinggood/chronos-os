# Dark Knight Intelligence Dashboard Prototype

This directory contains a static web application prototype for the Project Oracle intelligence console. The interface is inspired by the Dark Knight surveillance HUD and is designed for rapid situational awareness.

## Running the Prototype

Open `index.html` in any modern desktop browser. No build step or external tooling is required—the application is implemented in vanilla HTML, CSS, and JavaScript modules.

## Feature Highlights

- Dark, mono-chromatic interface with strategic accent colors reserved for status indicators.
- Responsive grid layout optimised for widescreen operational displays.
- Real-time threat probability sparkline driven by simulated telemetry.
- Animated readiness indicators for strategic assets and response teams.
- Modular visualization components (sparkline, circular meters, telemetry grid) implemented without external dependencies.
- Live intelligence feed with priority highlighting.

Feel free to extend the data providers in `js/app.js` to connect live back-end sources or replace the simulated telemetry with real metrics.
