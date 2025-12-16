async function getPin(pin) {
  const res = await fetch(`/api/blynk?pin=${pin}`);
  const data = await res.json();
  return data[0];
}

async function setPin(pin, value) {
  await fetch(`/api/blynk?pin=${pin}&value=${value}`);
}

// Refresh dashboard
async function refresh() {
  try {
    const temp = await getPin("V0");
    const hum  = await getPin("V1");
    const aqi  = await getPin("V2");
    const motion = await getPin("V3");
    const flame  = await getPin("V4");
    const occ    = await getPin("V5");
    const mode   = await getPin("V12");

    document.getElementById("temp").innerText = temp + " °C";
    document.getElementById("hum").innerText = hum + " %";
    document.getElementById("aqi").innerText = aqi;
    document.getElementById("motion").innerText = motion == 1 ? "Detected" : "No";
    document.getElementById("flame").innerText = flame == 1 ? "YES" : "NO";
    document.getElementById("occ").innerText = occ;

    document.getElementById("mode").innerText = mode == 1 ? "AUTO" : "MANUAL";
  } catch (e) {
    console.error("Fetch error", e);
  }
}

// Controls
function autoMode() {
  setPin("V12", 1);
}

function manualMode() {
  setPin("V12", 0);
}

function fanOn() {
  setPin("V10", 1);
}

function fanOff() {
  setPin("V10", 0);
}

// Theme
function toggleTheme() {
  document.body.classList.toggle("dark");
}

setInterval(refresh, 2000);
refresh();
