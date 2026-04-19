# archvital

`archvital` ist ein schlanker Desktop-Systemmonitor in Qt, gebaut für Leute, die ihre Kiste im Blick behalten wollen, ohne sich durch eine überladene UI zu kämpfen.

Die App zeigt die wichtigsten Live-Daten zu CPU, RAM, GPU, Datenträgern, Netzwerk und Prozessen an, bringt eine Diagnostics-Seite mit und hat inzwischen auch eine Settings-Section, in der man Farben und sichtbare Bereiche anpassen kann.

## Was die App kann

- `Overview` für den schnellen Gesamtblick
- `CPU`, `Memory`, `GPU`, `Disk`, `Network` und `Tasks` als eigene Seiten
- `Diagnostics`, um typische Problemstellen einmal gesammelt zu prüfen
- `Settings`, um Farben anzupassen und Sections ein- oder auszublenden
- Theme- und UI-Einstellungen werden gespeichert

## Tech Stack

- `C++20`
- `Qt6` (`Widgets` und `Core`)
- `CMake`
- Linux-Systemdaten aus `/proc`, `/sys` und ein paar Systemtools wie `ip`, `systemctl` oder `journalctl`

## Build

Im Projektordner:

```bash
cmake -S . -B build
cmake --build build
```

Wenn der `build/`-Ordner schon existiert, reicht meistens:

```bash
cmake --build build
```

## Starten

```bash
./build/archvital
```

## Voraussetzungen

Du brauchst im Wesentlichen:

- einen Linux-Desktop
- `Qt6`
- `cmake`
- einen C++-Compiler mit C++20-Support

Je nach Distro heißen die Pakete etwas anders. Unter Arch wäre das grob sowas wie `qt6-base`, `cmake` und ein Compiler-Toolchain-Paket.

## Projektstruktur

Nicht kompliziert gehalten:

- [`src/core`](./src/core) enthält Datenerfassung und die Strukturen dahinter
- [`src/sections`](./src/sections) sind die einzelnen Seiten der App
- [`src/ui`](./src/ui) enthält wiederverwendbare UI-Bausteine wie Sidebar, Cards und Sparklines
- [`src/mainwindow.*`](./src/mainwindow.cpp) hält das Fenster, die Navigation und das Zusammensetzen der App zusammen

## Einstellungen

Die App merkt sich unter anderem:

- Fensterzustand
- eingeklappte Sidebar
- zuletzt geöffnete Section
- Theme-Farben
- welche Sections sichtbar sein sollen

Das läuft über `QSettings`, also so, wie man es von einer normalen Desktop-App erwarten würde.

## Status

Das Projekt ist benutzbar und fühlt sich schon ziemlich ordentlich an, aber es ist noch ein aktives Bastel- und Verbesserungsprojekt. Wenn dir beim Testen irgendwas komisch vorkommt: sehr gut, genau dafür ist GitHub da.

## Mitmachen

Wenn du etwas verbessern willst:

1. Repo klonen
2. Branch machen
3. Bauen und testen
4. PR aufmachen

Kleine saubere Verbesserungen sind meistens mehr wert als zehn große Ideen, die nie landen.
