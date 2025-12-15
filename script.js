const TOKEN = import.meta.env.VITE_BLYNK_TOKEN;
const BASE  = import.meta.env.VITE_BLYNK_BASE;

async function getPin(pin) {
  const r = await fetch(`${BASE}/get?token=${TOKEN}&${pin}`);
  const j = await r.json();
  return j[0];
}

async function setPin(pin, value) {
  await fetch(`${BASE}/update?token=${TOKEN}&${pin}=${value}`);
}

async function refresh() {
  document.getElementById("temp").innerText   = await getPin("V0");
  document.getElementById("hum").innerText    = await getPin("V1");
  document.getElementById("aqi").innerText    = await getPin("V2");
  document.getElementById("motion").innerText = (await getPin("V3")) == 1 ? "YES" : "NO";
  document.getElementById("flame").innerText  = (await getPin("V4")) == 1 ? "YES" : "NO";
  document.getElementById("occ").innerText    = await getPin("V5");
  document.getElementById("mode").innerText   = (await getPin("V12")) == 1 ? "AUTO" : "MANUAL";
}

function setFan(v) {
  setPin("V10", v);
}

function setAuto(v) {
  setPin("V12", v);
}

setInterval(refresh, 2000);
refresh();

window.setFan = setFan;
window.setAuto = setAuto;
