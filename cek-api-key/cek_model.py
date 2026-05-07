# Untuk mengetahui model apa saja yang tersedia
import os
from dotenv import load_dotenv
import google.generativeai as genai

# Load API Key dari .env
load_dotenv()
api_key = os.getenv("GEMINI_API_KEY")

if not api_key:
    print("Error: API Key tidak ditemukan di file .env")
else:
    genai.configure(api_key=api_key)

    print(f"--- Daftar Model yang Bisa Anda Gunakan ---")
    try:
        for m in genai.list_models():
            if 'generateContent' in m.supported_generation_methods:
                # Menampilkan nama model dan versi yang tersedia
                print(f"Model: {m.name}")
    except Exception as e:
        print(f"Terjadi kesalahan: {e}")
