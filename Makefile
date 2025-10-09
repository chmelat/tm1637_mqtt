# Makefile pro TM1637 driver pro Raspberry Pi
# Kompatibilni s RPi1 a RPi5, vyžaduje pigpio knihovnu
#
# Použití:
#   make          - kompilace
#   make clean    - vyčištění
#   make install  - instalace pigpio (pokud není nainstalováno)
#   make run      - kompilace a spuštění

# Kompiler a flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lpigpio -lrt -lpthread

# Názvy souborů
TARGET = tm1637_temperature
SOURCE = tm1637_rpi_pigpio.c get_temp.c main.c
OBJECT = $(SOURCE:.c=.o)
HEAD = tm1637_rpi_pigpio.h get_temp.h

# Hlavní cíl - kompilace
$(TARGET): $(OBJECT)
	$(CC) $(OBJECT) -o $(TARGET) $(LDFLAGS)
	@echo "Kompilace dokončena. Spusťte: sudo ./$(TARGET)"

# Kompilace objektového souboru
%.o: %.c $(HEAD) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

# Vyčištění složky
clean:
	rm -f $(OBJECT) $(TARGET)
	@echo "Vyčištění dokončeno"

# Spuštění programu (vyžaduje sudo)
run: $(TARGET)
	@echo "Spouštění programu..."
	sudo ./$(TARGET)

# Spuštění s vlastním intervalem (příklad)
run-fast: $(TARGET)
	@echo "Spouštění s intervalem 10 sekund..."
	sudo ./$(TARGET) -i 10

# Kontrola instalace pigpio
check-pigpio:
	@echo "Kontrola pigpio knihovny..."
	@pkg-config --exists libpigpio || (echo "pigpio není nainstalováno. Spusťte: make install-pigpio" && exit 1)
	@echo "pigpio je dostupné"

# Instalace pigpio knihovny (pro Debian/Raspbian)
install-pigpio:
	@echo "Instalace pigpio knihovny..."
	sudo apt update
	sudo apt install -y libpigpio-dev libpigpio1

# Debug verze s debug symboly
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug verze zkompilována"

# Informace o systému
info:
	@echo "=== Systémové informace ==="
	@echo "Kompiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo "Cílový soubor: $(TARGET)"
	@echo "Zdrojový soubor: $(SOURCE)"
	@echo ""
	@echo "=== GPIO piny (výchozí) ==="
	@grep -E "#define.*PIN" tm1637_rpi_pigpio.h
	@echo ""
	@echo "=== Hardware připojení ==="
	@echo "TM1637 VCC -> RPi 3.3V"
	@echo "TM1637 GND -> RPi GND"
	@echo "TM1637 CLK -> RPi GPIO 23 + 4.7kΩ pull-up na 3.3V"
	@echo "TM1637 DIO -> RPi GPIO 24 + 4.7kΩ pull-up na 3.3V"

# Kontrola syntaxe bez kompilace
syntax-check:
	$(CC) $(CFLAGS) -fsyntax-only $(SOURCE)
	@echo "Syntax check OK"

# All cíl
all: clean $(TARGET)

# Help
help:
	@echo "Dostupné příkazy:"
	@echo "  make              - kompilace programu"
	@echo "  make clean        - vyčištění objektových souborů"
	@echo "  make run          - kompilace a spuštění (60s interval)"
	@echo "  make run-fast     - spuštění s intervalem 10s"
	@echo "  make debug        - kompilace debug verze"
	@echo "  make install-pigpio - instalace pigpio knihovny"
	@echo "  make check-pigpio - kontrola pigpio"
	@echo "  make syntax-check - kontrola syntaxe"
	@echo "  make info         - informace o projektu"
	@echo "  make help         - zobrazí tuto nápovědu"
	@echo ""
	@echo "Vlastní konfigurace:"
	@echo "  sudo ./$(TARGET) -i 30     - spuštění s intervalem 30s"
	@echo "  sudo ./$(TARGET) -h        - zobrazí nápovědu programu"
	@echo "  make CFLAGS=\"-DDIO_PIN=18 -DCLK_PIN=19\" - jiné GPIO piny"

# Phony targets (nejsou soubory)
.PHONY: clean run run-fast check-pigpio install-pigpio debug info syntax-check all help

# Výchozí cíl při spuštění make bez parametrů
.DEFAULT_GOAL := $(TARGET)
