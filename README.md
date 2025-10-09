# TM1637 Temperature Display for Raspberry Pi

Zobrazování teploty na 4-ciferném 7-segmentovém TM1637 displeji pro Raspberry Pi pomocí knihovny pigpio. Program čte teplotu z externího senzoru a zobrazuje ji s rozlišením 0.1°C.

**⚠️ KOMPATIBILITA:** Funguje na Raspberry Pi 1, 2, 3, 4. **Nefunguje na RPi5** (pigpio není kompatibilní s RPi5 k datu 2025-10-09).

## Vlastnosti

- ✅ **Vysoká přesnost**: Zobrazení teploty s rozlišením 0.1°C
- ✅ **Konfigurovatelné GPIO piny**: Možnost změny pinů při kompilaci
- ✅ **Konfigurovatelný interval**: Nastavitelný interval měření z příkazové řádky
- ✅ **Open-drain komunikace**: Implementace TM1637 protokolu s externími pull-up rezistory
- ✅ **Robust error handling**: Zobrazení "Err" při chybě čtení teploty
- ✅ **Systemd podpora**: Připravený service soubor pro automatický start
- ✅ **Graceful shutdown**: Korektní ukončení při Ctrl+C
- ✅ **Hardware validace**: Kontrola přítomnosti pull-up rezistorů při startu

## Hardware požadavky

### Raspberry Pi
- Raspberry Pi 1, 2, 3, nebo 4
- **⚠️ POZOR:** Raspberry Pi 5 **NENÍ PODPOROVÁNO** - knihovna pigpio nefunguje správně na RPi5 (k datu 2025-10-09)
- Raspbian/Raspberry Pi OS
- Root přístup pro GPIO operace

### TM1637 displej
- 4-ciferný 7-segmentový displej s TM1637 řadičem
- **Napájení pouze 3.3V** (5V může poškodit GPIO piny RPi!)
- 2x 4.7kΩ pull-up rezistory

### Externí teplotní senzor
- Binárka `r4dcb08` pro čtení teploty
- Výstup ve formátu float na stdout

## Zapojení hardware

**⚠️ VAROVÁNÍ:** Používejte pouze **3.3V napájení** pro TM1637! Napájení 5V může **trvale poškodit GPIO piny** Raspberry Pi!

```
TM1637 Displej    Raspberry Pi      Pull-up rezistor
┌─────────────┐   ┌──────────────┐   ┌──────────────┐
│ VCC         │───│ 3.3V (Pin 1) │   │              │
│ GND         │───│ GND (Pin 6)  │   │              │
│ CLK         │───│ GPIO 23      │───│ 4.7kΩ na 3.3V│
│ DIO         │───│ GPIO 24      │───│ 4.7kΩ na 3.3V│
└─────────────┘   └──────────────┘   └──────────────┘
```

### Schéma zapojení

```
3.3V ────┬────────── TM1637 VCC
         │
         ├─── 4.7kΩ ─┬─── GPIO 23 (CLK) ─── TM1637 CLK
         │           │
         └─── 4.7kΩ ─┴─── GPIO 24 (DIO) ─── TM1637 DIO

GND ──────────────────────────────────────── TM1637 GND
```

**⚠️ Důležité:** 
- Pull-up rezistory jsou **povinné** pro správnou open-drain komunikaci!
- **Nikdy nepoužívejte 5V napájení** - pouze 3.3V!

## Instalace

### 1. Klonování projektu
```bash
cd /home/tch/tm1637_temperature
# nebo zkopírovat soubory ručně
```

### 2. Instalace závislostí
```bash
# Instalace pigpio knihovny
sudo apt update
sudo apt install -y libpigpio-dev libpigpio1

# Nebo použít Makefile
make install-pigpio
```

### 3. Ověření instalace
```bash
make check-pigpio
```

## Kompilace

### Základní kompilace
```bash
make
```

### Kompilace s vlastními GPIO piny
```bash
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

### Debug verze
```bash
make debug
```

### Dostupné Makefile příkazy
```bash
make              # kompilace programu
make clean        # vyčištění objektových souborů  
make run          # kompilace a spuštění (60s interval)
make run-fast     # spuštění s intervalem 10s
make debug        # kompilace debug verze
make install-pigpio # instalace pigpio knihovny
make check-pigpio # kontrola pigpio
make syntax-check # kontrola syntaxe
make info         # informace o projektu
make help         # zobrazí nápovědu
```

## Použití

### Základní spuštění
```bash
# Výchozí interval 60 sekund
sudo ./tm1637_temperature

# S vlastním intervalem
sudo ./tm1637_temperature -i 30    # 30 sekund
sudo ./tm1637_temperature -i 120   # 2 minuty

# Zobrazení nápovědy
./tm1637_temperature -h
```

### Příklady výstupu
```
Zobrazeni teploty na dispeji TM1637 na Raspberry Pi 1 (pigpio)
==============================================================
Interval mereni: 60 sekund
Pro ukonceni stiskni Ctrl+C

TM1637 piny validovany a inicializovany (GPIO 24=DIO, GPIO 23=CLK)
23.5
```

### Systemd služba

#### Instalace služby
```bash
# Zkopírovat service soubor
sudo cp tm1637_temperature.service /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Povolit automatický start
sudo systemctl enable tm1637_temperature

# Spustit službu
sudo systemctl start tm1637_temperature
```

#### Správa služby
```bash
# Status služby
sudo systemctl status tm1637_temperature

# Zobrazit logy
sudo journalctl -u tm1637_temperature -f

# Zastavit službu
sudo systemctl stop tm1637_temperature

# Zakázat automatický start
sudo systemctl disable tm1637_temperature
```

## Konfigurace

### GPIO piny
Výchozí GPIO piny lze změnit při kompilaci:

```bash
# Změna na GPIO 18 (DIO) a GPIO 19 (CLK)
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

### Interval měření
Interval lze nastavit pomocí argumentu `-i`:

```bash
sudo ./tm1637_temperature -i 10     # 10 sekund
sudo ./tm1637_temperature -i 300    # 5 minut
```

### Systemd service konfigurace
Pro změnu argumentů ve service souboru upravte:

```ini
ExecStart=/home/tch/tm1637_temperature/tm1637_temperature -i 30
```

## Architektura projektu

```
TM1637_temperature/
├── main.c                    # Hlavní program loop
├── tm1637_rpi_pigpio.c      # TM1637 protokol implementace  
├── tm1637_rpi_pigpio.h      # TM1637 header s GPIO definicemi
├── get_temp.c               # Čtení teploty z ext. senzoru
├── get_temp.h               # Header pro get_temp
├── Makefile                 # Build systém
├── tm1637_temperature.service # Systemd service soubor
├── CLAUDE.md                # Claude Code instrukce
└── README.md                # Tento soubor
```

### Moduly

#### main.c
- Hlavní program loop s parsing argumentů
- Signal handling pro graceful shutdown  
- Volání TM1637 a teplotních funkcí

#### tm1637_rpi_pigpio.c/.h
- Kompletní implementace TM1637 protokolu
- Open-drain komunikace s pigpio knihovnou
- Hardware validace pull-up rezistorů
- Zobrazení čísel a chybových hlášek

#### get_temp.c/.h  
- Interface pro externí teplotní senzor
- Volání `./r4dcb08 -f` příkazu
- Parsing float hodnot z stdout

## Troubleshooting

### Kompatibilita hardware

**⚠️ Raspberry Pi 5 není podporováno**
```
Problém: Knihovna pigpio nefunguje správně na Raspberry Pi 5
Status: K datu 2025-10-09 není pigpio kompatibilní s RPi5
```

**Řešení:**
- Použít Raspberry Pi 1, 2, 3, nebo 4
- Na RPi5 zvažte alternativní GPIO knihovny (gpiod, lgpio)
- Sledovat vývoj pigpio pro budoucí podporu RPi5

### Program se nespustí

**Chyba: "Permission denied"**
```bash
# Řešení: Spustit s sudo právy
sudo ./tm1637_temperature
```

**Chyba: "Chyba pri inicializaci knihovny pigpio"**
```bash
# Kontrola instalace pigpio
make check-pigpio

# Reinstalace pigpio
make install-pigpio
```

### Hardware problémy

**Chyba: "Chybi pull-up rezistory"**
```
Chyba: Chybi pull-up rezistory
GPIO 24 (DIO): LOW, GPIO 23 (CLK): LOW
Zkontrolujte zapojeni 4.7kΩ pull-up rezistoru na 3.3V
```

**Řešení:**
1. Zkontrolovat zapojení 4.7kΩ pull-up rezistorů
2. Rezistory musí být připojeny mezi GPIO piny a 3.3V
3. Použít multimetr pro ověření spojů

**Displej nesvítí**
1. Zkontrolovat napájení TM1637 (pouze 3.3V!)
2. Ověřit správnost GPIO pinů v konfiguraci
3. Zkusit jiný TM1637 displej

**⚠️ Poškozené GPIO piny**
- Pokud jste omylem použili 5V napájení, GPIO piny mohou být trvale poškozeny
- Symptomy: GPIO piny nereagují, program hlásí chyby inicializace
- Řešení: Použít jiné GPIO piny nebo vyměnit Raspberry Pi

### Software problémy

**Zobrazuje "Err" na displeji**
```bash
# Kontrola přítomnosti r4dcb08
ls -la ./r4dcb08

# Test r4dcb08 ručně
./r4dcb08 -f
```

**Vysoké CPU využití**
```bash
# Zkontrolovat interval měření
sudo ./tm1637_temperature -i 60    # Delší interval
```

### Systemd služba problémy

**Služba se nespouští**
```bash
# Kontrola statusu
sudo systemctl status tm1637_temperature

# Zobrazení detailních logů
sudo journalctl -u tm1637_temperature -n 50
```

**Služba se restartuje příliš často**
```bash
# Zkontrolovat StartLimitBurst v service souboru
# Změnit RestartSec na vyšší hodnotu
```

## Vývoj

### Debug režim
```bash
make debug
sudo ./tm1637_temperature -i 5
```

### Syntax kontrola
```bash
make syntax-check
```

### Přidání nových funkcí
1. Editovat příslušné .c/.h soubory
2. Aktualizovat Makefile pokud potřeba
3. Zkompilovat: `make`
4. Otestovat: `make run`

## Bezpečnostní upozornění

**⚠️ KRITICKÉ:** 
- **Vždy používejte pouze 3.3V napájení** pro TM1637 displej
- **Nikdy nepřipojujte 5V** na GPIO piny Raspberry Pi
- **5V napájení může trvale poškodit** GPIO piny a celý Raspberry Pi
- Vždy zkontrolujte napětí multimetrem před připojením

## Licence

MIT License - viz LICENSE soubor pro detaily.

## Autor

Původní implementace portována z ATtiny13A na Raspberry Pi s pigpio knihovnou.

## Changelog

### v1.1 (2025-09-03)
- ✅ Konfigurovatelné GPIO piny přes CFLAGS
- ✅ Konfigurovatelný interval měření z CLI
- ✅ Vylepšené Makefile příkazy
- ✅ Přidán README.md
- ✅ Bezpečnostní upozornění pro 3.3V napájení

### v1.0 (2025-08-28)
- ✅ Základní TM1637 implementace
- ✅ Čtení teploty z externího senzoru  
- ✅ Systemd service podpora
- ✅ Hardware validace pull-up rezistorů

## Podpora

Pro hlášení chyb nebo návrhy vylepšení vytvořte issue v repositáři nebo kontaktujte autora.