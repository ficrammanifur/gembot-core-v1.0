# Gembot Core v1.0 - Setup Guide

Panduan lengkap instalasi dan konfigurasi **Gembot** di **Jetson Nano**.

## Spesifikasi Hardware & Software

| Kategori              | Detail                                      |
|-----------------------|---------------------------------------------|
| **Device**            | NVIDIA Jetson Nano (4GB)                    |
| **SoC**               | Tegra X1                                    |
| **CPU**               | Quad-core ARM Cortex-A57                    |
| **GPU**               | 128-core Maxwell GPU                        |
| **Memory**            | 4 GB LPDDR4                                 |
| **Storage**           | microSD (direkomendasikan minimal 64GB)     |
| **JetPack Version**   | JetPack 4.6 (L4T R32.7.6)                   |
| **CUDA Version**      | CUDA 10.2                                   |
| **cuDNN Version**     | cuDNN 8.2                                   |
| **Python**            | Python 3.6.9 (di dalam virtualenv)          |
| **PyTorch**           | 1.10.0 + CUDA 10.2                          |
| **Torchvision**       | 0.11.0 (built from source)                  |
| **OpenCV**            | opencv-python-headless 4.5.5.64             |

## Persiapan Awal

### 1. Update Sistem & Fix Bootloader Warning

```bash
sudo apt update
sudo apt install -f
sudo apt upgrade -y
sudo reboot
```

### 2. Buat Virtual Environment (Sudah Dilakukan)

```bash
# Jika belum
python3.6 -m virtualenv ~/jetson-ai
source ~/jetson-ai/bin/activate
```

### 3. Install PyTorch 1.10 + Torchvision

Sudah terinstall dengan:
- PyTorch 1.10.0 (CUDA 10.2)
- Torchvision 0.11.0

**Permanent OpenBLAS fix** (penting!):
```bash
echo 'export OPENBLAS_CORETYPE=ARMV8' >> ~/jetson-ai/bin/activate
```

Tambah swap ekstra (sangat direkomendasikan):
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

**Download model terbaik untuk Jetson Nano:**

```bash
# Model Nano (paling direkomendasikan untuk real-time)
wget https://github.com/ultralytics/yolov5/releases/download/v6.2/yolov5n.pt

# Atau model Small (lebih akurat tapi lebih lambat)
# wget https://github.com/ultralytics/yolov5/releases/download/v6.2/yolov5s.pt
```

Test deteksi:
```bash
python detect.py --weights yolov5n.pt --source data/images/bus.jpg --device 0 --img 640 --half
```

## Struktur Project Gembot

```bash
~/gembot-core-v1.0/
├── ai/
│   ├── models/           # yolov5n.pt, yolov5s.pt
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

## Langkah Selanjutnya

Setelah setup dasar selesai, lanjutkan dengan:

1. Buat struktur folder project
2. Buat `detector.py` (custom inference ringan)
3. Integrasi Flask + gTTS + pyserial

---

**Catatan Penting untuk Performa:**

- Selalu gunakan `--half` (FP16) untuk inference lebih cepat
- Gunakan model `yolov5n.pt` untuk real-time
- Jangan gunakan `view_img=True` di production
- Matikan plotting (`matplotlib`, `seaborn`) jika tidak diperlukan
- Pantau suhu Jetson (`tegrastats`)

---

**Dibuat untuk:**
- Jetson Nano 4GB + JetPack 4.6
- Python 3.6 + PyTorch 1.10
- Project Gembot (AI + Dashboard + Voice + Serial)

---
