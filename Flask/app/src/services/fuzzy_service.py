def fuzzy(value, min_val, max_val, buffer=0.0):
    """
    Menghitung derajat keanggotaan fuzzy linier.
    Nilai return antara 0.0 - 1.0
    """
    if min_val <= value <= max_val:
        return 1.0
    elif buffer > 0:
        if min_val - buffer <= value < min_val:
            return (value - (min_val - buffer)) / buffer
        elif max_val < value <= max_val + buffer:
            return ((max_val + buffer) - value) / buffer
    return 0.0


def cek_kelayakan_air(ph, tds, turbidity, suhu):
    """
    Mengecek kelayakan air minum berdasarkan data sensor.
    - pH: 6.5 - 8.5, toleransi fuzzy 0.5
    - TDS: ideal < 300 ppm, toleransi fuzzy naik hingga 500 ppm
    - Turbidity: ideal < 3 NTU, toleransi fuzzy naik hingga 5 NTU
    - Suhu: ideal 22–28 °C, toleransi ±3 °C (19–31 °C)

    Return: "Layak Minum" atau "Tidak Layak"
    """
    # Derajat keanggotaan fuzzy
    μ_ph = fuzzy(ph, 6.5, 8.5, buffer=0.5)
    μ_tds = 1.0 if tds < 300 else fuzzy(tds, 0, 300, buffer=200)
    μ_turbidity = 1.0 if turbidity < 3 else fuzzy(turbidity, 0, 3, buffer=2)
    μ_suhu = fuzzy(suhu, 22, 28, buffer=3)

    # Kombinasi logika fuzzy AND (ambil minimum derajat keanggotaan)
    kelayakan_min = min(μ_ph, μ_tds, μ_turbidity, μ_suhu)

    return "Layak Minum" if kelayakan_min >= 0.6 else "Tidak Layak"
