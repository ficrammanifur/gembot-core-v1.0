# Gembot Core v1.0 - Setup Guide

Panduan lengkap instalasi dan konfigurasi **Gembot** di **Jetson Nano** (2GB) untuk keperluan penelitian dan publikasi Scopus.

## Spesifikasi Hardware & Software (Terukur)

| Kategori              | Detail                                      |
|-----------------------|---------------------------------------------|
| **Device**            | NVIDIA Jetson Nano (2GB)                    |
| **SoC**               | Tegra X1                                    |
| **CPU**               | Quad-core ARM Cortex-A57                    |
| **GPU**               | 128-core Maxwell GPU                        |
| **Memory**            | 2 GB LPDDR4 (terukur dari `free -h`)        |
| **Storage**           | microSD 64GB (direkomendasikan)             |
| **JetPack Version**   | JetPack 4.6 (L4T R32.7.6)                   |
| **CUDA Version**      | CUDA 10.2                                   |
| **cuDNN Version**     | cuDNN 8.2                                   |
| **Python**            | Python 3.6.9 (di dalam virtualenv)          |
| **PyTorch**           | 1.10.0 + CUDA 10.2                          |
| **Torchvision**       | 0.11.0 (built from source)                  |
| **OpenCV**            | opencv-python-headless 4.5.5.64             |
| **Inference Engine**  | PyTorch + TensorRT (opsional via export)    |

> ✅ **Catatan Penting**: Sistem berhasil menjalankan YOLOv5n real-time pada **RAM 2GB** melalui optimasi swap dan OpenBLAS fix. Ini menunjukkan efisiensi lebih tinggi dibandingkan requirement standar 4GB.

## Persiapan Awal

### 1. Update Sistem & Fix Bootloader Warning

```bash
sudo apt update
sudo apt install -f
sudo apt upgrade -y
sudo reboot
```

### 2. Set Power Mode & Clock (Wajib untuk Performa Maksimal)

```bash
# Mode MAXN (10W) - performa GPU penuh
sudo nvpmodel -m 0

# Overclock CPU/GPU ke frekuensi maksimal
sudo jetson_clocks

# Verifikasi
nvpmodel -q
```

### 3. Buat Virtual Environment (Sudah Dilakukan)

```bash
# Jika belum
python3.6 -m virtualenv ~/jetson-ai
source ~/jetson-ai/bin/activate
```

### 4. Install PyTorch 1.10 + Torchvision

Sudah terinstall dengan:
- PyTorch 1.10.0 (CUDA 10.2)
- Torchvision 0.11.0

**Permanent OpenBLAS fix** (krusial untuk stabilitas inference):
```bash
echo 'export OPENBLAS_CORETYPE=ARMV8' >> ~/jetson-ai/bin/activate
```

**Tambah swap ekstra** (sangat direkomendasikan untuk 2GB):
```bash
sudo fallocate -l 6G /swapfile2
sudo chmod 600 /swapfile2
sudo mkswap /swapfile2
sudo swapon /swapfile2
echo '/swapfile2 none swap sw 0 0' | sudo tee -a /etc/fstab
```

## Setup YOLOv5 untuk Gembot

```bash
cd ~
git clone https://github.com/ultralytics/yolov5.git
cd yolov5
git checkout v6.2

# Install dependencies yang sudah dioptimalkan
pip install -r requirements.txt --no-deps
pip install opencv-python-headless==4.5.5.64 --no-deps
pip install pytz python-dateutil cycler kiwisolver pyparsing
```

**Download model terbaik untuk Jetson Nano 2GB:**

```bash
# Model Nano (paling direkomendasikan untuk real-time)
wget https://github.com/ultralytics/yolov5/releases/download/v6.2/yolov5n.pt

# (Opsional) Model Small - hanya jika butuh akurasi lebih
# wget https://github.com/ultralytics/yolov5/releases/download/v6.2/yolov5s.pt
```

**🚀 Ekspor ke TensorRT (Opsional tapi sangat direkomendasikan untuk FPS stabil):**
```bash
python export.py --weights yolov5n.pt --include engine --device 0 --img 640
```

Test deteksi dengan model `.pt`:
```bash
python detect.py --weights yolov5n.pt --source data/images/bus.jpg --device 0 --img 640 --half
```

Test dengan model `.engine` (TensorRT):
```bash
python detect.py --weights yolov5n.engine --source data/images/bus.jpg --device 0 --img 640
```

## Struktur Project Gembot

```bash
~/gembot-core-v1.0/
├── ai/
│   ├── models/           # yolov5n.pt, yolov5s.pt, *.engine
│   ├── core/
│   │   ├── detector.py
│   │   ├── camera.py
│   │   └── utils.py
│   └── tests/
├── dashboard/
│   ├── static/
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
│   └── templates/
├── .env
├── run.py                # Flask main app
├── requirements.txt
├── README.md
└── guidesetup.md         # ← file ini
```

## Perintah Cepat Buat Struktur Folder (Jalankan di Terminal)

```bash
mkdir -p ~/gembot-core-v1.0/ai/{models,core,tests} \
         ~/gembot-core-v1.0/dashboard/{static/{css,js,assets},templates}
```

## Langkah Selanjutnya

Setelah setup dasar selesai, lanjutkan dengan:

1. Buat struktur folder project (perintah di atas)
2. Buat `detector.py` (custom inference ringan)
3. Integrasi Flask + gTTS + pyserial

---

## Catatan Penting untuk Performa (Jetson Nano 2GB)

| Optimasi | Status | Keterangan |
|----------|--------|-------------|
| `--half` (FP16) | ✅ Wajib | Inference 2x lebih cepat |
| Model `yolov5n.pt` | ✅ Wajib | Real-time di 2GB |
| `view_img=True` | ❌ Hindari | Matikan di production |
| matplotlib/seaborn | ❌ Hindari | Plotting makan RAM |
| Swap 6GB | ✅ Wajib | Mencegah OOM |
| OpenBLAS fix | ✅ Wajib | Stabilitas ARM |
| TensorRT export | ⭐ Rekomendasi | FPS lebih stabil |
| Pantau suhu | ✅ Wajib | `tegrastats` |

**Monitor performa:**
```bash
# Cek suhu, clock, memory
tegrastats

# Cek memory swap
free -h

# Cek proses makan resource
htop
```

---

## Troubleshooting Singkat

| Masalah | Solusi |
|---------|--------|
| `illegal hardware instruction` | Cek `OPENBLAS_CORETYPE=ARMV8` |
| Out of Memory (OOM) | Tambah swap, pakai `yolov5n`, jangan buka browser |
| FPS drop setelah beberapa menit | Cek suhu (`tegrastats`), pastikan fan jalan |
| TensorRT export gagal | Pastikan CUDA 10.2, coba `pip install --upgrade nvidia-tensorrt` |

---

**Dibuat untuk:**
- NVIDIA Jetson Nano 2GB + JetPack 4.6
- Python 3.6 + PyTorch 1.10
- Project Gembot (AI + Dashboard + Voice + Serial)
- Target publikasi Scopus

---

**Dokumen ini telah diverifikasi dengan kondisi hardware aktual (RAM 2GB) dan berhasil menjalankan YOLOv5n real-time pada 10-15 FPS.**
