# Zusammenfassung und Ausblick {#sec:schluss}

<!-- - (auch ohne Hauptteil verständlich)
- (Essenz der Arbeit darstellen) -->

<!-- - Das war das Ziel
  - grundlagen erläutern, die zum verständnis notwendig sind
  - spectre-angriffe beschreiben und implementieren: cache-basierte seitenkanalangriffe, allgemeine funktionsweise, wiederverwendbare elemente, ausgewählte konkrete angriffe
  - spectre-angriffe evaluieren: cache-basierte seitenkanalangriffe, training des branch predictors, ausgewählte konkrete angriffe
- Dieses wurde wie folgt erfüllt
  - grundlagen erklärt: speicher, caches, prozessororganisation/mikroarchitektur
  - funktionsweise dargestellt: cache-basierte seitenkanalangriffe, allgemeine funktionsweise, ausgewählte konkrete angriffe
  - implementierung besprochen: cache-basierte seitenkanalangriffe, ausgewählte konkrete angriffe
  - evaluiert: cache-basierte seitenkanalangriffe, training des branch predictors, ausgewählte konkrete angriffe -->

Das Ziel dieser Arbeit bestand zunächst in einer Erklärung der Funktionsweise moderner Spectre-Angriffe, gefolgt von einer Implementierung und Evaluation ausgewählter konkreter Spectre-Angriffe. Als konkrete Spectre-Angriffe wurden dabei RIDL, ZombieLoad, Write Transient Forwarding und Store-to-Leak gewählt. Zur Erfüllung dieses Ziels wurden zu Beginn die Grundlagen der Funktionsweise moderner Prozessoren erläutert, die für ein Verständnis von Spectre-Angriffen benötigt werden. Dies umfasst die Funktionsweise von Prozessor-Caches und den Aufbau der Mikroarchitektur moderner Intel-Prozessoren. Anschließend wurden Flush+Reload und Flush+Flush als Cache-basierte Seitenkanalangriffe beschrieben. Diese bilden einen wichtigen Teil der Implementierung von Spectre-Angriffen. Danach wurde die Methodik von Spectre-Angriffen im Allgemeinen dargestellt. Diese Angriffe wurden zunächst in zwei Kategorien unterteilt: Spectre-Angriffe basierend auf Branch Prediction und Spectre-Angriffe basierend auf Prozessor-Exceptions. Daraufhin wurde die Funktionsweise der oben genannten konkreten Spectre-Angriffe erklärt und Details ihrer Implementierung beschrieben. Im Zuge der Evaluation wurden zunächst einzelne Techniken ausgewertet, die in Spectre-Angriffen Verwendung finden. Dazu gehören die beiden beschriebenen Cache-basierten Seitenkanalangriffe sowie eine Auswahl an Techniken zur Unterdrückung von Prozessor-Exceptions. Schließlich wurden die konkreten Spectre-Angriffe selbst evaluiert. Von jedem Angriff wurden dabei verschiedene Varianten nach einheitlichen Metriken ausgewertet und anschließend verglichen.

<!-- - wichtigste ergebnisse der evaluation
  - flush+reload langsamer und wesentlich zuverlässiger als flush+flush (+ beides funktioniert)
    - 100,0 und 99,99 vs 49,68 und 93,33 %
  - training des branch predictors funktioniert zuverlässig mit verschiedenen techniken
    - 99,93 % oder besser
  - ridl und zombieload funktionieren
    - ridl: bis zu 443,9 B/s bei 97,96 %
    - zombieload: bis zu 780,3 B/s bei 99,70 %
  - write transient forwarding funktioniert nicht
  - store-to-leak funktioniert
    - bis zu 99,89 und 100,0 % -->

Die Evaluation hat ergeben, dass Flush+Reload wesentlich zuverlässiger, aber in vielen Fällen langsamer ist als Flush+Flush. Die untersuchten Techniken zur Unterdrückung von Prozessor-Exceptions haben zuverlässig funktioniert. Jede dieser Techniken erreichte eine Erfolgsrate von $99{,}93\,\%$ oder mehr. Von den vier betrachteten Spectre-Angriffen zeigten drei die erwarteten Ergebnisse in den meisten ihrer Varianten. RIDL und ZombieLoad extrahierten erfolgreich Daten aus einem anderen Prozess. RIDL erreichte dabei eine Datenrate von $443{,}9$ B/s bei einer Erfolgsrate von $97{,}96\,\%$, ZombieLoad eine Datenrate von $780{,}3$ B/s bei einer Erfolgsrate von $99{,}70\,\%$. Mithilfe von Store-to-Leak gelang es, Speicherzuordnungen des Betriebssystem-Kerns korrekt zu klassifizieren mit einer Sensitivität von $99{,}98\,\%$ und einer Spezifität von $100{,}0\,\%$. Durch Write Transient Forwarding hingegen konnten in keiner der evaluierten Varianten die anvisierten Daten erfolgreich extrahiert werden. Insgesamt konnten damit Techniken aus den Papern zu Flush+Reload [@flush-reload], Flush+Flush [@flush-flush], RIDL [@ridl], ZombieLoad [@zombieload] und Store-to-Leak [@fallout] erfolgreich implementiert und Ergebnisse dieser Paper reproduziert werden.

<!-- - Ausblick auf mögliche weiterführende Arbeiten
- an einigen stellen hier sachen nicht untersucht (wie erwähnt)
  - angriffe basierend auf branch prediction
  - mehr varianten
- angriffe können noch optimiert werden
- auf mehr systemen testen
- mehr kombinationen/varianten testen
- andere angriffe betrachten, z.b. basierend auf branch prediction
- andere angriffsszenarien betrachten
  - andere angreifer: kernel, tee wie sgx, aus einer vm
  - andere ziele: tee wie sgx, andere vms unter dem gleichen hypervisor, hypervisor
  - andere hardware: prozessorerweiterungen wie tsx
  - andere caches als l1? -->

Für aufbauende Arbeiten bietet sich beispielsweise eine Optimierung der implementierten Angriffe an. Außerdem wurden in dieser Arbeit ausschließlich konkrete Spectre-Angriffe betrachtet, die auf Prozessor-Exceptions basieren, sowie von diesen nur ausgewählte Varianten. Weiterführende Forschung kann demnach Spectre-Angriffe basierend auf Branch Prediction oder weitere Kombinationen der beschriebenen Techniken untersuchen. Zusätzlich fand die Evaluation im Rahmen dieser Arbeit auf einem einzigen System statt. Um die betrachteten Varianten in einem größerem Kontext zu evaluieren, können die Untersuchungen auf weiteren Systemen wiederholt werden. Desweiteren wurden in dieser Arbeit nur Angriffe betrachtet, die von unprivilegierten Nutzerprozessen auf Linux-Systemen durchgeführt werden können. In weiterführenden Arbeiten können folglich andere Angriffsszenarien betrachtet werden. Dies umfasst Angreifer in anderen Kontexten (z.B. im Betriebssystem-Kern oder in Trusted Execution Environments wie Intel SGX), andere Angriffsziele (z.B. Trusted Execution Environments, Hypervisor oder benachbarte virtuelle Maschinen) sowie andere Hardwarekonfigurationen (z.B. Cross-Core Angriffe oder Unterstützung von Intel TSX). Da in dieser Arbeit außerdem keine Gegenmaßnahmen für Spectre-Angriffe betrachtet wurden, können zukünftige Arbeiten die hier implementierten Angriffe im Kontext aktivierter Gegenmaßnahmen untersuchen.
