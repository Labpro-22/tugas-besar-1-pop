[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/b842RB8g)
# Tugas Besar 1 IF2010 Pemrograman Berorientasi Objek

## 🛠️ Cara Kompilasi dan Menjalankan Program (CMake)

Proyek ini menggunakan **CMake** (minimal versi 3.13) sebagai *build system* yang bersifat lintas platform (*cross-platform*). 

### Langkah-langkah Kompilasi & Eksekusi:

Buka terminal di direktori *root* repositori ini, lalu jalankan perintah berikut secara berurutan:

1. **Konfigurasi proyek dan buat folder build:**
   ```bash
   cmake -S . -B build
   ```
2. **Kompilasi kode program:**
   ```bash
   cmake --build build
   ```
3. **Jalankan aplikasi:**
   ```bash
   ./build/game
   ```
4. **Untuk melakukan clean:**
    ```bash
    rm -rf build
    ```
