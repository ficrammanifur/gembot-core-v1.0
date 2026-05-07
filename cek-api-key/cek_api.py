import os
from dotenv import load_dotenv
import google.generativeai as genai

load_dotenv()

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

if not GEMINI_API_KEY:
    print("❌ GEMINI_API_KEY not found in .env file")
    print("   Please add: GEMINI_API_KEY=your_api_key_here")
else:
    print(f"✅ API Key found: {GEMINI_API_KEY[:20]}...")
    
    try:
        genai.configure(api_key=GEMINI_API_KEY)
        
        # Coba models yang tersedia
        models = ['gemini-1.5-flash', 'gemini-2.0-flash-exp', 'gemini-1.5-flash-8b']
        
        for model_name in models:
            try:
                model = genai.GenerativeModel(model_name)
                response = model.generate_content("Halo, apa kabar?", generation_config={"max_output_tokens": 20})
                print(f"✅ Model {model_name} working! Response: {response.text[:50]}")
                break
            except Exception as e:
                print(f"⚠️ Model {model_name} failed: {str(e)[:50]}")
    except Exception as e:
        print(f"❌ Error: {e}")
