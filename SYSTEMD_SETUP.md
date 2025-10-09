# Systemd Setup pro TM1637 Temperature Display

## Instalace a konfigurace

### 1. Příprava projektu
```bash
# Zkompilovat projekt
make

# Otestovat manuálně (připojte displej s pull-up rezistory)
sudo ./tm1637_temperature
```

### 2. Instalace systemd service
```bash
# Zkopírovat service soubor
sudo cp tm1637_temperature.service /etc/systemd/system/

# Načíst novou konfiguraci
sudo systemctl daemon-reload

# Povolit autostart po bootu
sudo systemctl enable tm1637_temperature.service

# Spustit službu
sudo systemctl start tm1637_temperature.service
```

### 3. Ověření konfigurace
```bash
# Kontrola stavu služby
sudo systemctl status tm1637_temperature.service

# Sledování logů v reálném čase
sudo journalctl -u tm1637_temperature -f

# Zobrazit poslední logy
sudo journalctl -u tm1637_temperature -n 20
```

## Očekávané chování

### S připojeným displejem
```
● tm1637_temperature.service - TM1637 Temperature Display Service
   Loaded: loaded
   Active: active (running)
   
Logs:
TM1637 piny validovany a inicializovany (GPIO 24=DIO, GPIO 23=CLK)
```

### Bez připojeného displeje
```
● tm1637_temperature.service - TM1637 Temperature Display Service  
   Loaded: loaded
   Active: failed (Result: start-limit-hit)
   
Logs:
Chyba: Chybi pull-up rezistory
GPIO 24 (DIO): LOW, GPIO 23 (CLK): LOW
```

**Chování:** 2 pokusy po 10 minutách, pak 24h pauza

## Správa služby

### Základní příkazy
```bash
# Restart služby
sudo systemctl restart tm1637_temperature.service

# Zastavení služby  
sudo systemctl stop tm1637_temperature.service

# Spuštění služby
sudo systemctl start tm1637_temperature.service

# Zakázat autostart
sudo systemctl disable tm1637_temperature.service

# Povolí autostart
sudo systemctl enable tm1637_temperature.service
```

### Při připojení displeje (pokud je služba stopped)
```bash
# Manuální restart
sudo systemctl restart tm1637_temperature.service

# Nebo resetovat start-limit a spustit
sudo systemctl reset-failed tm1637_temperature.service
sudo systemctl start tm1637_temperature.service
```

### Troubleshooting
```bash
# Detailní status
sudo systemctl status tm1637_temperature.service -l

# Všechny logy od posledního bootu
sudo journalctl -u tm1637_temperature -b

# Logy s časovými razítky
sudo journalctl -u tm1637_temperature --since "1 hour ago"

# Test syntaxe systemd souboru
sudo systemd-analyze verify /etc/systemd/system/tm1637_temperature.service
```

## Hardware požadavky

- TM1637 CLK → GPIO 23 + 4.7kΩ pull-up na 3.3V
- TM1637 DIO → GPIO 24 + 4.7kΩ pull-up na 3.3V  
- External temperature sensor binary `r4dcb08` v projektové složce
- Root/sudo přístup pro GPIO operace

## Poznámky

- Service vyžaduje root privileges kvůli GPIO přístupu
- Pull-up rezistory jsou povinné pro detekci připojeného displeje
- Service se automaticky restartuje při připojení displeje během prvních 20 minut po bootu
- Po start-limit-hit je nutný manuální restart služby
