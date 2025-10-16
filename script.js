(() => {
  const DEFAULT_SETTINGS = {
    cityName: "Gotham City",
    latitude: 40.7128,
    longitude: -74.006,
    openWeatherKey: ""
  };

  const STORAGE_KEY = "darkKnightConfig";
  const MAX_LOG_ITEMS = 24;
  const FALLBACK_INCIDENTS = [
    {
      id: "fallback-01",
      type: "Robbery",
      description: "Coordinated armored car interception",
      block: "Park Row & 5th Ave",
      sector: "Financial District",
      occurredAt: new Date(Date.now() - 1000 * 60 * 18),
      location: { lat: 40.7126, lng: -74.0098 },
      severity: classifyIncident("Robbery")
    },
    {
      id: "fallback-02",
      type: "Arson",
      description: "Explosive device neutralized",
      block: "Otisburg Warehouse 12",
      sector: "Otisburg",
      occurredAt: new Date(Date.now() - 1000 * 60 * 52),
      location: { lat: 40.7251, lng: -74.0021 },
      severity: classifyIncident("Arson")
    },
    {
      id: "fallback-03",
      type: "Battery",
      description: "League of Shadows skirmish",
      block: "Gotham Narrows",
      sector: "The Narrows",
      occurredAt: new Date(Date.now() - 1000 * 60 * 78),
      location: { lat: 40.7084, lng: -74.0015 },
      severity: classifyIncident("Battery")
    },
    {
      id: "fallback-04",
      type: "Burglary",
      description: "Wayne R&amp;D lab breach attempt",
      block: "Wayne Tower",
      sector: "Midtown",
      occurredAt: new Date(Date.now() - 1000 * 60 * 140),
      location: { lat: 40.7547, lng: -73.9863 },
      severity: classifyIncident("Burglary")
    },
    {
      id: "fallback-05",
      type: "Weapons Violation",
      description: "Illegal arms cache uncovered",
      block: "Iceberg Lounge",
      sector: "Diamond District",
      occurredAt: new Date(Date.now() - 1000 * 60 * 210),
      location: { lat: 40.7421, lng: -73.9924 },
      severity: classifyIncident("Weapons Violation")
    }
  ];

  const state = {
    settings: { ...DEFAULT_SETTINGS },
    incidents: [],
    systemStatus: [],
    operationsLog: [],
    crimeChart: null,
    map: null,
    mapLayer: null,
    statusInterval: null,
    logInterval: null
  };

  const dom = {};

  document.addEventListener("DOMContentLoaded", () => {
    cacheDom();
    hydrateSettings();
    initClock();
    initSettingsPanel();
    initSystemStatus();
    initOperationsLog();
    initMap();
    fetchWeather();
    fetchNews();
    fetchIncidentData();
    window.addEventListener("resize", () => {
      if (state.map) {
        state.map.invalidateSize();
      }
    });
  });

  function cacheDom() {
    dom.date = document.getElementById("currentDate");
    dom.time = document.getElementById("currentTime");
    dom.statusGrid = document.getElementById("statusGrid");
    dom.statusUpdated = document.getElementById("statusUpdated");
    dom.alertsCount = document.getElementById("alertsCount");
    dom.alertsList = document.getElementById("alertsList");
    dom.mapLastUpdated = document.getElementById("mapLastUpdated");
    dom.weatherTemperature = document.getElementById("weatherTemperature");
    dom.weatherSummary = document.getElementById("weatherSummary");
    dom.weatherFeels = document.getElementById("weatherFeels");
    dom.weatherWind = document.getElementById("weatherWind");
    dom.weatherHumidity = document.getElementById("weatherHumidity");
    dom.weatherForecast = document.getElementById("weatherForecast");
    dom.weatherMeta = document.getElementById("weatherMeta");
    dom.newsFeed = document.getElementById("newsFeed");
    dom.newsUpdated = document.getElementById("newsUpdated");
    dom.crimeUpdated = document.getElementById("crimeUpdated");
    dom.crimeInsights = document.getElementById("crimeInsights");
    dom.operationsLog = document.getElementById("operationsLog");
    dom.settingsToggle = document.getElementById("settingsToggle");
    dom.settingsPanel = document.getElementById("settingsPanel");
    dom.settingsBackdrop = document.getElementById("settingsBackdrop");
    dom.settingsForm = document.getElementById("settingsForm");
    dom.closeSettings = document.getElementById("closeSettings");
    dom.cityInput = document.getElementById("cityName");
    dom.latInput = document.getElementById("latitude");
    dom.lonInput = document.getElementById("longitude");
    dom.weatherKeyInput = document.getElementById("openWeatherKey");
    dom.mapContainer = document.getElementById("map");
  }

  function hydrateSettings() {
    try {
      const cached = localStorage.getItem(STORAGE_KEY);
      if (cached) {
        const parsed = JSON.parse(cached);
        state.settings = { ...DEFAULT_SETTINGS, ...parsed };
      }
    } catch (error) {
      console.warn("Unable to read config from storage", error);
      state.settings = { ...DEFAULT_SETTINGS };
    }

    dom.cityInput.value = state.settings.cityName;
    dom.latInput.value = state.settings.latitude;
    dom.lonInput.value = state.settings.longitude;
    dom.weatherKeyInput.value = state.settings.openWeatherKey;
  }

  function initClock() {
    const update = () => {
      const now = new Date();
      const dateFormatter = new Intl.DateTimeFormat("en-US", {
        weekday: "short",
        month: "short",
        day: "2-digit",
        year: "numeric"
      });
      const timeFormatter = new Intl.DateTimeFormat("en-US", {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        hour12: false
      });
      dom.date.textContent = dateFormatter.format(now).toUpperCase();
      dom.time.textContent = timeFormatter.format(now);
    };
    update();
    setInterval(update, 1000);
  }

  function initSettingsPanel() {
    dom.settingsToggle.addEventListener("click", openSettingsPanel);
    dom.closeSettings.addEventListener("click", closeSettingsPanel);
    dom.settingsBackdrop.addEventListener("click", closeSettingsPanel);

    dom.settingsForm.addEventListener("submit", (event) => {
      event.preventDefault();
      const formData = new FormData(dom.settingsForm);
      const cityName = formData.get("cityName").trim() || DEFAULT_SETTINGS.cityName;
      const latitude = parseFloat(formData.get("latitude"));
      const longitude = parseFloat(formData.get("longitude"));
      const openWeatherKey = formData.get("openWeatherKey").trim();

      const sanitizedLatitude = Number.isFinite(latitude) ? latitude : DEFAULT_SETTINGS.latitude;
      const sanitizedLongitude = Number.isFinite(longitude) ? longitude : DEFAULT_SETTINGS.longitude;

      state.settings = {
        cityName,
        latitude: sanitizedLatitude,
        longitude: sanitizedLongitude,
        openWeatherKey
      };

      try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(state.settings));
      } catch (error) {
        console.warn("Unable to persist settings", error);
      }

      appendLogEntry(`Configuration updated for ${cityName}`, "info");
      closeSettingsPanel();

      if (state.map) {
        state.map.setView([sanitizedLatitude, sanitizedLongitude], state.map.getZoom() || 12);
      }

      fetchWeather();
      if (state.incidents.length === 0) {
        fetchIncidentData();
      }
    });
  }

  function openSettingsPanel() {
    dom.settingsPanel.classList.remove("hidden");
    dom.settingsBackdrop.classList.remove("hidden");
    requestAnimationFrame(() => {
      dom.settingsPanel.classList.add("visible");
      dom.settingsBackdrop.classList.add("visible");
      dom.settingsToggle.setAttribute("aria-expanded", "true");
      dom.settingsPanel.setAttribute("aria-hidden", "false");
      dom.settingsBackdrop.setAttribute("aria-hidden", "false");
    });
  }

  function closeSettingsPanel() {
    dom.settingsPanel.classList.remove("visible");
    dom.settingsBackdrop.classList.remove("visible");
    dom.settingsToggle.setAttribute("aria-expanded", "false");
    dom.settingsPanel.setAttribute("aria-hidden", "true");
    dom.settingsBackdrop.setAttribute("aria-hidden", "true");

    const handlePanelTransition = () => {
      dom.settingsPanel.classList.add("hidden");
      dom.settingsPanel.removeEventListener("transitionend", handlePanelTransition);
    };

    const handleBackdropTransition = () => {
      dom.settingsBackdrop.classList.add("hidden");
      dom.settingsBackdrop.removeEventListener("transitionend", handleBackdropTransition);
    };

    dom.settingsPanel.addEventListener("transitionend", handlePanelTransition);
    dom.settingsBackdrop.addEventListener("transitionend", handleBackdropTransition);
  }

  function initSystemStatus() {
    state.systemStatus = [
      createStatusMetric("power", "Power Grid", randomBetween(78, 92)),
      createStatusMetric("network", "Network Uplink", randomBetween(70, 90)),
      createStatusMetric("surveillance", "Surveillance Grid", randomBetween(65, 88)),
      createStatusMetric("drones", "Drone Fleet", randomBetween(60, 85)),
      createStatusMetric("batmobile", "Batmobile Telemetry", randomBetween(72, 95)),
      createStatusMetric("vault", "Evidence Vault Integrity", randomBetween(82, 97))
    ];

    renderSystemStatus();
    updateStatusTimestamp();

    if (state.statusInterval) {
      clearInterval(state.statusInterval);
    }

    state.statusInterval = setInterval(() => {
      state.systemStatus = state.systemStatus.map((metric) => {
        const variance = randomBetween(-4, 4);
        const nextValue = clamp(metric.value + variance, 35, 100);
        const trend = nextValue > metric.value ? "up" : nextValue < metric.value ? "down" : "steady";
        const { level, label: statusLabel } = resolveStatusLevel(nextValue);
        return {
          ...metric,
          value: nextValue,
          trend,
          level,
          statusLabel
        };
      });

      renderSystemStatus();
      updateStatusTimestamp();
      appendLogEntry("System diagnostics refreshed", "info");
    }, 12000);
  }

  function createStatusMetric(id, title, value) {
    const { level, label: statusLabel } = resolveStatusLevel(value);
    return {
      id,
      title,
      value,
      unit: "%",
      level,
      statusLabel,
      trend: "steady"
    };
  }

  function renderSystemStatus() {
    if (!dom.statusGrid) return;

    dom.statusGrid.innerHTML = "";
    state.systemStatus.forEach((metric) => {
      const item = document.createElement("article");
      item.className = "status-card";
      item.setAttribute("role", "listitem");

      const trendSymbol = metric.trend === "up" ? "▲" : metric.trend === "down" ? "▼" : "■";
      const trendClass = `status-trend status-trend--${metric.trend}`;
      const labelClass = `status-label status-label--${metric.level}`;

      item.innerHTML = `
        <h4>${metric.title}</h4>
        <div class="status-metric">${metric.value}<span>${metric.unit}</span></div>
        <div class="status-bar"><span style="width: ${metric.value}%"></span></div>
        <div class="status-footer">
          <span class="${labelClass}">${metric.statusLabel}</span>
          <span class="${trendClass}" aria-label="${metric.trend} trend">${trendSymbol}</span>
        </div>
      `;

      dom.statusGrid.appendChild(item);
    });
  }

  function updateStatusTimestamp() {
    if (!dom.statusUpdated) return;
    dom.statusUpdated.textContent = `Updated ${formatTime(new Date())}`;
  }

  function initOperationsLog() {
    const initialEntries = [
      "Initializing Dark Knight Intelligence Network",
      "All subsystems routed through secure uplink",
      "Real-time feeds synchronized",
      "Thermal imaging calibration nominal"
    ];

    initialEntries.forEach((entry) => appendLogEntry(entry, "success"));

    if (state.logInterval) {
      clearInterval(state.logInterval);
    }

    state.logInterval = setInterval(() => {
      const heartbeatMessages = [
        "Background AI sweep complete",
        "Satellite uplink stabilized",
        "Encrypted data channels verified",
        "Sensor grid integrity within acceptable range",
        "Nightwatch drone telemetry synchronized"
      ];
      const message = heartbeatMessages[Math.floor(Math.random() * heartbeatMessages.length)];
      appendLogEntry(message, "info");
    }, 15000);
  }

  function appendLogEntry(message, type = "info") {
    const entry = {
      message,
      type,
      time: new Date()
    };
    state.operationsLog.unshift(entry);
    if (state.operationsLog.length > MAX_LOG_ITEMS) {
      state.operationsLog.pop();
    }
    renderOperationsLog();
  }

  function renderOperationsLog() {
    if (!dom.operationsLog) return;
    dom.operationsLog.innerHTML = "";

    state.operationsLog.forEach((entry) => {
      const li = document.createElement("li");
      li.className = `log-item log-item--${entry.type}`;
      li.innerHTML = `
        <span class="log-time">${formatTime(entry.time)}</span>
        <span class="log-message">${entry.message}</span>
      `;
      dom.operationsLog.appendChild(li);
    });
  }

  function initMap() {
    if (!dom.mapContainer || typeof L === "undefined") {
      dom.mapLastUpdated.textContent = "Map module unavailable";
      return;
    }

    state.map = L.map(dom.mapContainer, {
      center: [state.settings.latitude, state.settings.longitude],
      zoom: 12,
      minZoom: 3,
      maxZoom: 18,
      zoomControl: false,
      attributionControl: false
    });

    L.tileLayer("https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png", {
      subdomains: "abcd",
      maxZoom: 19,
      detectRetina: true
    }).addTo(state.map);

    L.control
      .zoom({
        position: "bottomright"
      })
      .addTo(state.map);

    dom.mapLastUpdated.textContent = "Map initialized";
  }

  async function fetchIncidentData() {
    const endpoint = "https://data.cityofchicago.org/resource/ijzp-q8t2.json?$limit=40&$order=date DESC";
    try {
      const response = await fetch(endpoint);
      if (!response.ok) {
        throw new Error(`Incident feed error: ${response.status}`);
      }
      const payload = await response.json();
      const incidents = payload
        .filter((incident) => incident.latitude && incident.longitude)
        .map(transformIncident)
        .slice(0, 30);

      if (!incidents.length) {
        throw new Error("No incidents returned");
      }

      state.incidents = incidents;
      renderIncidents();
      updateCrimeAnalytics();
      updateAlerts();
      appendLogEntry("Incident feed synchronized", "success");
    } catch (error) {
      console.warn(error);
      state.incidents = FALLBACK_INCIDENTS;
      renderIncidents(true);
      updateCrimeAnalytics(true);
      updateAlerts(true);
      appendLogEntry("Incident feed offline — using cached intelligence", "warn");
    }
  }

  function transformIncident(rawIncident) {
    const severity = classifyIncident(rawIncident.primary_type || "Unknown");
    const generatedId =
      rawIncident.case_number ||
      rawIncident.id ||
      (typeof crypto !== "undefined" && typeof crypto.randomUUID === "function"
        ? crypto.randomUUID()
        : `incident-${Date.now()}-${Math.random().toString(16).slice(2)}`);

    return {
      id: generatedId,
      type: rawIncident.primary_type || "Unknown",
      description: rawIncident.description || "No description",
      block: rawIncident.block || "Unknown block",
      sector: rawIncident.beat || rawIncident.district || "Unknown sector",
      occurredAt: rawIncident.date ? new Date(rawIncident.date) : new Date(),
      location: {
        lat: parseFloat(rawIncident.latitude),
        lng: parseFloat(rawIncident.longitude)
      },
      severity
    };
  }

  function classifyIncident(type) {
    const normalized = type ? type.toUpperCase() : "UNKNOWN";
    const criticalCategories = ["HOMICIDE", "ARSON", "ASSAULT", "BATTERY"];
    const highCategories = ["ROBBERY", "WEAPONS VIOLATION", "BURGLARY", "KIDNAPPING"];

    if (criticalCategories.includes(normalized)) {
      return { level: "critical", color: "#ff5555", priority: 3, label: "Critical" };
    }
    if (highCategories.includes(normalized)) {
      return { level: "high", color: "#ff8c32", priority: 2, label: "High" };
    }
    return { level: "elevated", color: "#00d1ff", priority: 1, label: "Elevated" };
  }

  function renderIncidents(isFallback = false) {
    if (!state.map || typeof L === "undefined") return;

    if (state.mapLayer) {
      state.mapLayer.clearLayers();
      state.map.removeLayer(state.mapLayer);
    }

    if (!state.incidents.length) {
      dom.mapLastUpdated.textContent = "No incident data available";
      return;
    }

    const markers = state.incidents.map((incident) => {
      const marker = L.circleMarker([incident.location.lat, incident.location.lng], {
        radius: 8 + incident.severity.priority * 2,
        color: incident.severity.color,
        weight: 1.4,
        fillColor: incident.severity.color,
        fillOpacity: 0.65
      });

      marker.bindPopup(
        `
          <div class="map-popup">
            <strong>${incident.type}</strong>
            <p>${incident.description}</p>
            <p>${incident.block}</p>
            <span>${formatTime(incident.occurredAt)} — ${incident.severity.label}</span>
          </div>
        `
      );
      return marker;
    });

    state.mapLayer = L.layerGroup(markers).addTo(state.map);

    try {
      const bounds = L.latLngBounds(state.incidents.map((incident) => [incident.location.lat, incident.location.lng]));
      state.map.fitBounds(bounds.pad(0.1));
    } catch (error) {
      state.map.setView([state.settings.latitude, state.settings.longitude], 12);
    }

    dom.mapLastUpdated.textContent = `${isFallback ? "Cached" : "Updated"} ${formatTime(new Date())}`;
  }

  function updateAlerts(isFallback = false) {
    if (!dom.alertsList) return;
    const alerts = state.incidents
      .filter((incident) => incident.severity.priority >= 2)
      .sort((a, b) => b.severity.priority - a.severity.priority)
      .slice(0, 4);

    dom.alertsList.innerHTML = "";

    if (!alerts.length) {
      const item = document.createElement("li");
      item.className = "alert-item";
      item.innerHTML = "<span>All sectors nominal</span><span>--</span>";
      dom.alertsList.appendChild(item);
      dom.alertsCount.textContent = "0";
      return;
    }

    alerts.forEach((alert) => {
      const item = document.createElement("li");
      item.className = "alert-item";
      item.innerHTML = `
        <span>${alert.type} — ${alert.block}</span>
        <span>${alert.severity.label}</span>
      `;
      dom.alertsList.appendChild(item);
    });

    dom.alertsCount.textContent = `${alerts.length}${isFallback ? "*" : ""}`;
  }

  function updateCrimeAnalytics(isFallback = false) {
    const counts = state.incidents.reduce((acc, incident) => {
      const key = incident.type || "Unknown";
      acc[key] = (acc[key] || 0) + 1;
      return acc;
    }, {});

    const sorted = Object.entries(counts)
      .sort(([, a], [, b]) => b - a)
      .slice(0, 6);

    const labels = sorted.map(([label]) => label);
    const data = sorted.map(([, value]) => value);

    if (typeof Chart !== "undefined") {
      if (!state.crimeChart) {
        const ctx = document.getElementById("crimeChart");
        state.crimeChart = new Chart(ctx, {
          type: "radar",
          data: {
            labels,
            datasets: [
              {
                label: "Incident Density",
                data,
                backgroundColor: "rgba(0, 209, 255, 0.2)",
                borderColor: "rgba(0, 209, 255, 0.8)",
                borderWidth: 2,
                pointBackgroundColor: "#00d1ff",
                pointBorderColor: "#081521"
              }
            ]
          },
          options: {
            responsive: true,
            scales: {
              r: {
                angleLines: {
                  color: "rgba(0, 209, 255, 0.2)"
                },
                grid: {
                  color: "rgba(0, 209, 255, 0.18)"
                },
                ticks: {
                  showLabelBackdrop: false,
                  color: "rgba(234, 249, 255, 0.8)",
                  backdropColor: "transparent"
                },
                pointLabels: {
                  color: "rgba(234, 249, 255, 0.88)",
                  font: {
                    family: "Orbitron",
                    size: 12
                  }
                }
              }
            },
            plugins: {
              legend: {
                labels: {
                  color: "rgba(234, 249, 255, 0.8)",
                  font: {
                    family: "Orbitron",
                    size: 12
                  }
                }
              }
            }
          }
        });
      } else {
        state.crimeChart.data.labels = labels;
        state.crimeChart.data.datasets[0].data = data;
        state.crimeChart.update("none");
      }
    }

    dom.crimeInsights.innerHTML = "";
    sorted.forEach(([label, value]) => {
      const li = document.createElement("li");
      li.textContent = `${label}: ${value} incident${value !== 1 ? "s" : ""}`;
      dom.crimeInsights.appendChild(li);
    });

    dom.crimeUpdated.textContent = `${isFallback ? "Cached" : "Updated"} ${formatTime(new Date())}`;
  }

  async function fetchWeather() {
    const { latitude, longitude, cityName, openWeatherKey } = state.settings;
    if (!dom.weatherMeta) return;

    if (!openWeatherKey) {
      dom.weatherMeta.textContent = "OpenWeatherMap key required — using fallback";
      appendLogEntry("Weather feed using fallback provider", "warn");
      return fetchWeatherFallback();
    }

    try {
      const [currentRes, forecastRes] = await Promise.all([
        fetch(`https://api.openweathermap.org/data/2.5/weather?lat=${latitude}&lon=${longitude}&units=metric&appid=${openWeatherKey}`),
        fetch(
          `https://api.openweathermap.org/data/2.5/forecast?lat=${latitude}&lon=${longitude}&units=metric&appid=${openWeatherKey}&cnt=5`
        )
      ]);

      if (!currentRes.ok || !forecastRes.ok) {
        throw new Error(`Weather feed error: ${currentRes.status}/${forecastRes.status}`);
      }

      const current = await currentRes.json();
      const forecast = await forecastRes.json();

      renderWeather({ current, forecast, source: "OpenWeatherMap", cityName });
      appendLogEntry("Weather data synchronized", "success");
    } catch (error) {
      console.warn(error);
      dom.weatherMeta.textContent = "OpenWeatherMap unavailable — fallback active";
      appendLogEntry("Weather feed offline — fallback engaged", "warn");
      return fetchWeatherFallback();
    }
  }

  async function fetchWeatherFallback() {
    const { latitude, longitude, cityName } = state.settings;
    try {
      const fallbackRes = await fetch(
        `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current_weather=true&hourly=temperature_2m,relativehumidity_2m,windspeed_10m&timezone=auto`
      );
      if (!fallbackRes.ok) {
        throw new Error(`Fallback weather error: ${fallbackRes.status}`);
      }
      const fallback = await fallbackRes.json();
      renderWeather({ fallback, source: "Open-Meteo", cityName });
    } catch (error) {
      console.warn(error);
      dom.weatherTemperature.textContent = "--";
      dom.weatherSummary.textContent = "Weather feed unavailable";
      dom.weatherFeels.textContent = "-";
      dom.weatherWind.textContent = "-";
      dom.weatherHumidity.textContent = "-";
      dom.weatherForecast.innerHTML = "";
      dom.weatherMeta.textContent = "Weather systems offline";
    }
  }

  function renderWeather({ current, forecast, fallback, source, cityName }) {
    if (!dom.weatherTemperature) return;

    if (current) {
      const temperature = Math.round(current.main.temp);
      const feelsLike = Math.round(current.main.feels_like);
      const wind = Math.round(current.wind.speed * 3.6); // m/s -> km/h
      const humidity = current.main.humidity;
      const summary = current.weather?.[0]?.description || "Stable";

      dom.weatherTemperature.textContent = `${temperature}°C`;
      dom.weatherSummary.textContent = summary.toUpperCase();
      dom.weatherFeels.textContent = `Feels like ${feelsLike}°C`;
      dom.weatherWind.textContent = `Wind ${wind} km/h`;
      dom.weatherHumidity.textContent = `Humidity ${humidity}%`;

      const forecastItems = forecast?.list?.map((entry) => {
        const time = new Date(entry.dt * 1000);
        return {
          label: formatShortTime(time),
          temp: Math.round(entry.main.temp),
          summary: entry.weather?.[0]?.main || "" 
        };
      });
      renderForecastCards(forecastItems || []);
      dom.weatherMeta.textContent = `${source} • ${cityName} • Updated ${formatTime(new Date())}`;
      return;
    }

    if (fallback?.current_weather) {
      const currentWeather = fallback.current_weather;
      const temperature = Math.round(currentWeather.temperature);
      const wind = Math.round(currentWeather.windspeed);
      const humiditySeries = fallback.hourly?.relativehumidity_2m || [];
      const humidity = humiditySeries.length ? humiditySeries[0] : "--";

      dom.weatherTemperature.textContent = `${temperature}°C`;
      dom.weatherSummary.textContent = weatherCodeToSummary(currentWeather.weathercode);
      dom.weatherFeels.textContent = `Feels like ${temperature}°C`;
      dom.weatherWind.textContent = `Wind ${wind} km/h`;
      dom.weatherHumidity.textContent = `Humidity ${humidity}%`;

      const forecastItems = buildFallbackForecast(fallback);
      renderForecastCards(forecastItems);
      dom.weatherMeta.textContent = `${source} • ${cityName} • Updated ${formatTime(new Date())}`;
    }
  }

  function renderForecastCards(items) {
    dom.weatherForecast.innerHTML = "";
    if (!items.length) {
      dom.weatherForecast.innerHTML = "<div class=\"forecast-card\">No forecast data</div>";
      return;
    }

    items.slice(0, 6).forEach((item) => {
      const card = document.createElement("div");
      card.className = "forecast-card";
      card.innerHTML = `
        <span>${item.label}</span>
        <span class="forecast-temp">${item.temp}°C</span>
        <span>${item.summary}</span>
      `;
      dom.weatherForecast.appendChild(card);
    });
  }

  function buildFallbackForecast(fallback) {
    const temps = fallback.hourly?.temperature_2m || [];
    const times = fallback.hourly?.time || [];
    return times.slice(1, 7).map((timeIso, index) => {
      const time = new Date(timeIso);
      return {
        label: formatShortTime(time),
        temp: Math.round(temps[index + 1] || temps[index] || fallback.current_weather.temperature),
        summary: weatherCodeToSummary(fallback.current_weather.weathercode)
      };
    });
  }

  function weatherCodeToSummary(code) {
    const mapping = {
      0: "Clear",
      1: "Mainly Clear",
      2: "Partly Cloudy",
      3: "Overcast",
      45: "Fog",
      48: "Rime Fog",
      51: "Light Drizzle",
      53: "Drizzle",
      55: "Heavy Drizzle",
      61: "Light Rain",
      63: "Rain",
      65: "Heavy Rain",
      66: "Freezing Rain",
      67: "Freezing Rain",
      71: "Light Snow",
      73: "Snow",
      75: "Heavy Snow",
      77: "Snow Grains",
      80: "Rain Showers",
      81: "Rain Showers",
      82: "Violent Showers",
      95: "Thunderstorm",
      96: "Thunderstorm",
      99: "Thunderstorm"
    };
    return mapping[code] ? mapping[code].toUpperCase() : "STABLE";
  }

  async function fetchNews() {
    const endpoint = "https://hn.algolia.com/api/v1/search?query=security&tags=story&hitsPerPage=8";
    try {
      const response = await fetch(endpoint);
      if (!response.ok) {
        throw new Error(`News feed error: ${response.status}`);
      }
      const payload = await response.json();
      const articles = (payload.hits || []).map((hit) => ({
        title: hit.title,
        url: hit.url || `https://news.ycombinator.com/item?id=${hit.objectID}`,
        source: hit._highlightResult?.author?.value || hit.author,
        publishedAt: hit.created_at ? new Date(hit.created_at) : new Date()
      }));
      renderNews(articles);
      appendLogEntry("News intelligence feed updated", "success");
    } catch (error) {
      console.warn(error);
      dom.newsFeed.innerHTML = "<li class=\"news-item\">Unable to retrieve intelligence feed</li>";
      dom.newsUpdated.textContent = "Feed offline";
      appendLogEntry("News feed unavailable", "warn");
    }
  }

  function renderNews(articles) {
    dom.newsFeed.innerHTML = "";
    if (!articles.length) {
      dom.newsFeed.innerHTML = "<li class=\"news-item\">No intelligence available</li>";
      return;
    }

    articles.forEach((article) => {
      const item = document.createElement("li");
      item.className = "news-item";
      item.innerHTML = `
        <a href="${article.url}" target="_blank" rel="noopener">${article.title}</a>
        <p>Source: ${article.source || "Unknown"}</p>
        <span>${formatTime(article.publishedAt)}</span>
      `;
      dom.newsFeed.appendChild(item);
    });
    dom.newsUpdated.textContent = `Updated ${formatTime(new Date())}`;
  }

  function resolveStatusLevel(value) {
    if (value < 50) {
      return { level: "alert", label: "Critical" };
    }
    if (value < 70) {
      return { level: "warn", label: "Monitor" };
    }
    return { level: "ok", label: "Nominal" };
  }

  function formatTime(date) {
    return new Intl.DateTimeFormat("en-US", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
      hour12: false
    }).format(date);
  }

  function formatShortTime(date) {
    return new Intl.DateTimeFormat("en-US", {
      hour: "2-digit",
      minute: "2-digit",
      hour12: false
    }).format(date);
  }

  function randomBetween(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
  }

  function clamp(value, min, max) {
    return Math.min(Math.max(value, min), max);
  }
})();
