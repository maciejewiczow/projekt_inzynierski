# Projekt inżynierski
Podsystem symulujący temperaturę wsadu w maszynie do ciągnienia bezmatrycowego za pomocą metody elementów skończonych, opraty o Arduino.

## Klonowanie
Projekt korzysta z `git submodules`, więc przy klonowaniu trzeba użyć opcji `--recurse-submodules`, w następujący sposób:
```bash
git clone --recurse-submodules https://github.com/maciejewiczow/inzynierka.git
```

## Pobieranie
Przy pobieraniu submodules nie są niestety dołączane do powstałego zipa, więc trzeba je pobrać ręcznie i umieścić w odpowiednich podfolderach w [./lib](./lib). Linki do odpowiednich repozytoriów na github znajdują sie w pliku [.gitmodules](./.gitmodules).

## Przegląd projektu

Dostępne parametry kompilacji - [config.h](./config.h)
Symulacja - [Mesh.h](./Mesh.h#L38-89)
Obsługa menu - [Menu.h](./Menu.h)
Pakiety do komunikacji binarnej między Arduino a PC - [communication.h](./communication.h), [communication.py](./communication.py)
Skrypt symulujący termoparę oraz odczytujący dane iteracji z Arduino - [simulateTempSensor.py](./simulateTempSensor.py)
Wersja PC projektu -  [PCversion](./PCversion)

