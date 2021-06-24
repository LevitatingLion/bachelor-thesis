# Funktionsweise und Evaluation von modernen Spectre-Angriffen: Dokumentation des Quellcodes

Diese README-Datei beschreibt alle beigelegten Dateien, sowie benötigte externe Abhängigkeiten.

Sind alle externen Abhängigkeiten vorhanden, so kann die gesamte Evaluation mithilfe von `run.sh` ausgeführt werden.

## Beigelegte Dateien

Die folgenden Dateien sind der Bachelorarbeit beigelegt:

- `README.md`: Diese README-Datei.

- `run.sh`: Kompiliert die Angriffe mit `build.sh`, startet danach die Angriffe und protokolliert ermittelte Daten, berechnet schließlich Metriken mit `eval_and_plot.py`.

- `build.sh`: Kompiliert die Angriffe in allen untersuchten Varianten.

- `common.sh`: Von `run.sh` und `build.sh` gemeinsam verwendete Funktionen.

- `eval_and_plot.py`: Berechnet Metriken aus den erfassten Daten, generiert Abbildungen und Tabellen.

- `cyclecount.c`: Bestimmt die Zeit, die für die Latenzmessung selber benötigt wird.

- `sidechannel.c`: Evaluiert Flush+Reload oder Flush+Flush als Cache-basierte Seintenkanalangriffe.

- `misprediction.c`: Evaluiert Techniken, die Transient Execution durch eine falsche Vorhersage des Branch Predictors auslösen.

- `ridl.c`: Evaluiert RIDL.

- `wtf.c`: Evaluiert Write Transient Forwarding.

- `zombieload.c`: Evaluiert ZombieLoad.

- `storetoleak.c`: Evaluiert Store-to-Leak.

- `common.c`: Teile der Implementierung, die mehrere Angriffe gemeinsam haben.

- `common.h`: Teile der Implementierung, die mehrere Angriffe gemeinsam haben.

Innerhalb der Dateien sind Quellcode-Kommentare auf Englisch verfasst.

## Externe Abhängigkeiten

Die einzelnen Komponenten der Implementierung haben die folgenden externen Abhängigkeiten:

- Die Shell Skripte erfordern `bash`, `clang`, die GNU Coreutils und `pidof`.

- Das Python Skript erfordert Python 3 mit den Paketen `numpy`, `matplotlib` und `pandas`, sowie eine konfigurierte `de_DE.utf8` Locale.

- Der C Code muss mit GCC oder Clang kompiliert werden.
