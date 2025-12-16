export default async function handler(req, res) {
  const { pin, value } = req.query;

  const TOKEN = process.env.BLYNK_TOKEN;
  const BASE = "https://blynk.cloud/external/api";

  if (!pin) {
    return res.status(400).json({ error: "Pin missing" });
  }

  try {
    // WRITE
    if (value !== undefined) {
      await fetch(`${BASE}/update?token=${TOKEN}&${pin}=${value}`);
      return res.status(200).json({ status: "ok" });
    }

    // READ
    const response = await fetch(`${BASE}/get?token=${TOKEN}&${pin}`);
    const data = await response.json();
    return res.status(200).json(data);
  } catch (err) {
    return res.status(500).json({ error: "Blynk error" });
  }
}
