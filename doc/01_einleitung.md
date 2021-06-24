# Einleitung

<!-- - Allgemeines Thema: Transient-Execution Angriffe
  - begann mit spectre und meltdown anfang 2018
    - spectre: angriffe beeinflussen speculative execution im kontext des opfers, um daten aus dem kontext des opfers zu extrahieren
    - meltdown: nutzerprozesse lesen daten des betriebssystem-kern
- Konkretes Thema: moderne Varianten dieser
  - seitdem stetig weiter forschung
  - mittlerweile viele verschiedene varianten der ursprünglichen sicherheitslücken
  - kollektiv als Spectre-Angriffe bezeichnet
  - drei/vier dieser varianten werden hier untersucht
- Relevanz des Themas:
  - thema allgemein ist relevant: praktische angriffe, die z.B. lokal passwörter extrahieren, cross-vm daten extrahieren in cloud-szenarien
  - konkretes thema: neue lücken die geschlossen werden müssen, neue und vllt allgemeinere verteidigungen -->

<!-- Seit die Sicherheitslücken bekannt als Spectre und Meltdown Anfang 2018 veröffentlicht wurden, wird auf dem Gebiet der Sicherheit spekulativer Ausführung stetig weiter geforscht. Mittlerweile existieren viele verschiedene Varianten der ursprünglich entdeckten Sicherheitslücken. Diese Bachelorarbeit evaluiert drei dieser Varianten. -->
<!-- Moderne Prozessoren versuchen das Ziel eines bedingten Sprunges vorherzusagen, um  -->
<!-- Moderne Prozessoren nutzen spekulative Ausführung um ihre Leistungsfähigkeit zu steigern.
Diese betreffend -->

Moderne Prozessoren verwenden eine Optimierung, die das Ziel bedingter Sprünge vorhersagt. Dabei wird der vorhergesagte Ausführungspfad spekulativ ausgeführt, bis das tatsächliche Ziel des Sprunges feststeht. Im Zusammenhang mit dieser spekulativen Ausführung wurden Anfang des Jahres 2018 die als Spectre und Meltdown bekannten Sicherheitslücken veröffentlicht. Spectre erlaubt einem Angreifer, im Kontext des Opfers auftretende spekulative Ausführung zu beeinflussen und dadurch Daten aus diesem zu extrahieren. Meltdown ermöglicht Nutzerprozessen sämtliche Daten des Betriebssystem-Kerns auszulesen. Durch die Entdeckung dieser Sicherheitslücken ist mit dem Gebiet der Sicherheit spekulativer Ausführung ein neuer, aktiver Forschungszweig entstanden. Mittlerweile existieren viele verschiedene Varianten der ursprünglich entdeckten Sicherheitslücken. Angriffe auf diese werden kollektiv auch als Spectre-Angriffe bezeichnet. Viele Spectre-Angriffe können beispielsweise dazu genutzt werden, auf lokalen Systemen Passwörter anderer Nutzer oder in Cloud-Szenarien Daten einer benachbarten virtuellen Maschine zu extrahieren. Aus diesem Grund sind Spectre-Angriffe von großer Bedeutung für die Sicherheit einer Vielzahl von Systemen. Für Spectre-Angriffe wurden verschiedene Gegenmaßnahmen entwickelt, sowohl in Hardware als auch in Software. Oft sind diese nur gegen einen Teil der Angriffe wirksam und mit Einbußen in der Performance verbunden. Daher sind Gegenmaßnahmen, genauso wie Spectre-Angriffe selber, ein aktiver Gegenstand der Forschung.

<!-- - Fragestellungen (abstrakt und in weitem Kontext)
  - wie funktionieren die angriffe?
  - wie performant/zuverlässig sind die angriffe?
- Ziele und Teilziele
  - funktionsweise dieser angriffe allgemein darstellen
  - funktionsweise der betrachteten angriffe konkret darstellen
  - (wiederverwendbare elemente identifizieren und implementieren)
  - angriffe implementieren
  - performance auswerten / evaluieren
    - verschiedene varianten der angriffe
    - nach einheitlichen metriken
  - vergleich mit ergebnissen der literatur -->

<!-- Das Ziel dieser Arbeit besteht darin, die Funktionsweise ausgewählter Spectre-Angriffe zu erklären, sowie verschiedene Varianten dieser Angriffe zu evaluieren. -->

Das Ziel dieser Arbeit besteht zunächst darin, die Funktionsweise ausgewählter Spectre-Angriffe zu erklären. Anschließend werden verschiedene Varianten dieser Angriffe implementiert und hinsichtlich einheitlicher Metriken evaluiert. Schließlich werden die Ergebnisse dieser Evaluation mit Ergebnissen der Literatur verglichen und in einen weiteren Kontext gesetzt. Als konkrete Spectre-Angriffe werden RIDL, ZombieLoad, Write Transient Forwarding und Store-to-Leak gewählt. Write Transient Forwarding und Store-to-Leak werden zusammen auch als Fallout bezeichnet.

<!-- - Vorgehen / Ausblick auf Kapitel / Grobe Gliederung der Arbeit
  - stand der forschung
  - erst grundlagen, die für verständnis der angriffe notwendig sind
    - Dies umfasst die Funktionsweise von Prozessor-Caches und den Aufbau der Mikroarchitektur moderner Intel-Prozessoren.
  - dann angriffe und ihre implementierung darstellen
    - sidechannel attacks
    - transient-execution angriffe allgemein
    - wiederverwendbare teile der angriffe implementieren
    - konkrete angriffe und details der implementierung
  - evaluation einzelner verwendeter techniken
  - evaluation der angriffe
    - hinsichtlich datenrate und fehlerrate
  - vergleich mit ergebnissen der literatur
  - zusammenfassung und ausblick -->

Zunächst werden in [@sec:einordnung] einige bekannte Spectre-Angriffe und damit der aktuelle Stand der Forschung dargestellt. In [@sec:grundlagen] werden die Grundlagen der Funktionsweise moderner Prozessoren behandelt, die für ein Verständnis von Spectre-Angriffen benötigt werden. Dies umfasst die Funktionsweise von Prozessor-Caches ([@sec:grundlagen-caches]) und den Aufbau der Mikroarchitektur moderner Intel-Prozessoren ([@sec:grundlagen-mikroarch]). Anschließend werden in [@sec:grundlagen-sidechannel] Flush+Reload und Flush+Flush als Cache-basierte Seitenkanalangriffe beschrieben. Diese bilden einen wichtigen Teil der Implementierung von Spectre-Angriffen. Danach wird in [@sec:impl-allgemein] die Methodik von Spectre-Angriffen im Allgemeinen dargestellt. Diese Angriffe werden zunächst in zwei Kategorien unterteilt: Spectre-Angriffe basierend auf Branch Prediction ([@sec:impl-spectre-prediction]) und Spectre-Angriffe basierend auf Prozessor-Exceptions ([@sec:impl-spectre-exception]). Daraufhin wird in [@sec:impl-angriffe] die Funktionsweise der gewählten konkreten Spectre-Angriffe erklärt und Details ihrer Implementierung beschrieben. Im Zuge der Evaluation in [@sec:eval] werden zunächst einzelne Techniken ausgewertet, die in Spectre-Angriffen Verwendung finden. Dazu gehören die beiden beschriebenen Cache-basierten Seitenkanalangriffe ([@sec:eval-sidechannel]) sowie eine Auswahl an Techniken zur Unterdrückung von Prozessor-Exceptions ([@sec:eval-transient]). Schließlich werden in [@sec:eval-angriffe] und [@sec:eval-stl] die konkreten Spectre-Angriffe selbst evaluiert. Von jedem Angriff werden dabei verschiedene Varianten nach einheitlichen Metriken ausgewertet und anschließend verglichen. In [@sec:schluss] werden die Ergebnisse dieser Arbeit zusammengefasst und ein Ausblick auf weitere Forschungsmöglichkeiten gegeben.

# Stand der Forschung {#sec:einordnung}

<!-- - (Wissenschaftliche Entwicklung des Bereichs)
- (Vorherige Arbeiten in dem Bereich) -->

<!-- - (anfangs seitenkanalangriffe auf den cache)
  - f+r, f+f
- spectre und meltdown, angriffe auf mikroarchitektur
  - spectre: angriffe beeinflussen speculative execution im kontext des opfers, um daten aus dem kontext des opfers zu extrahieren
  - meltdown: nutzerprozesse lesen daten des betriebssystem-kern
- viele neue varianten
  - spectre:
    - branchscope: beeinflusst den branch predictor, um ausgang von bedingten sprüngen zu extrahieren
    - ret2spec, spectrersb: wenden spectre an auf speculative execution bei rucksprüngen aus funktionen, parallele forschung
    - sgxpectre: benutzt spectre um daten aus einer enklave zu extrahieren
  - meltdown:
    - foreshadow: nutzt page fault exceptions um daten von enklaven aus dem l1d cache zu extrahieren
    - foreshadow-ng: verallgemeinerung von foreshadow, erlaubt es einer vm den gesamten l1d cache des hosts zu extrahieren
    - lazyfp: extrahiert inhalt der FPU register mithilfe von device not available exceptions
    - ridl: extrahiert daten aus dem line-fill buffer oder den load ports mithilfe von page-fault exceptions
    - fallout: extrahiert daten aus store buffer, nutzt unter anderem page-fault oder general protection exceptions
    - zombieload: extrahiert daten aus line-fill buffer mithilfe von microcode assists
- systematisierung: transient-fail -->

Die zuerst entdeckten Sicherheitslücken im Zusammenhang mit spekulativer Ausführung waren Spectre und Meltdown. Spectre wurde im Jahre 2018 von Kocher et al. vorgestellt und erlaubt einem Angreifer, im Kontext des Opfers auftretende spekulative Ausführung zu beeinflussen und dadurch Daten aus diesem zu extrahieren [@spectre]. Meltdown wurde im Jahre 2018 von Lipp et al. vorgestellt und ermöglicht Nutzerprozessen, sämtliche Daten des Betriebssystem-Kerns auszulesen [@meltdown]. Mittlerweile existieren viele verschiedene Varianten dieser ursprünglich entdeckten Sicherheitslücken.

Horn beschrieb im Jahre 2018 eine Variante von Spectre, die eine inkorrekte Vorhersage von schreibenden Speicherzugriffen ausnutzt [@spectre-4]. Evtyushkin et al. entdeckten BranchScope im Jahre 2018 als Variante von Spectre. BranchScope beeinflusst die Vorhersage bedingter Sprünge, um den Ausgang von bedingten Sprüngen des Opfers zu beobachten [@spectre-branchscope]. Maisuradze und Rossow, sowie Koruyeh et al. stoßen im Jahre 2018 unabhängig voneinander auf eine Variante von Spectre, die spekulative Ausführung nach Rücksprüngen aus Funktionen ausnutzt [@spectre-ret2spec] [@spectre-spectrersb]. Chen et al. stellten im Jahre 2019 SgxPectre vor. Dabei handelt es sich um eine Spectre-Variante, die Daten aus einer Intel SGX Enklave extrahiert [@spectre-sgxpectre].

Im Jahre 2018 wurde die Meltdown-Variante Foreshadow von Bulck et al. gefunden. Diese nutzt Prozessor-Exceptions, um Daten aus dem L1 Cache einer Intel SGX Enklave zu extrahieren [@foreshadow]. Weisse et al. verallgemeinerten Foreshadow im Jahre 2018 zu Foreshadow-NG, welches es einer virtuellen Maschine erlaubt den gesamten L1 Cache des Wirtsystems zu lesen [@foreshadow-ng]. Ebenfalls im Jahre 2018 wurde LazyFP als Variante von Meltdown entdeckt. LazyFP extrahiert den Inhalt von FPU-Prozessorregistern mithilfe von Device-Not-Available Prozessor-Exceptions [@lazyfp]. Van Schaik et al. fanden im Jahre 2019 die Meltdown-Variante RIDL. Diese nutzt Page-Fault Exceptions, um Daten aus dem prozessorinternen Line-Fill Buffer oder aus den Load Ports zu extrahieren [@ridl]. Canella et al. stellten im Jahre 2019 Fallout als Meltdown-Variante vor. Fallout extrahiert Daten aus dem Store Buffer und nutzt dafür unter anderem Page-Fault Exceptions oder General Protection Exceptions [@fallout]. Schwarz et al. entdeckten die Meltdown-Variante ZombieLoad im Jahre 2019. ZombieLoad extrahiert Daten aus dem Line-Fill Buffer mithilfe von Microcode Assists [@zombieload].

Um diese Varianten von Spectre und Meltdown zu systematisieren, entwickelten Canella et al. im Jahre 2019 einen Klassifizierungsbaum, der alle Varianten nach verschiedenen Kriterien kategorisiert. Außerdem beschrieben und evaluierten sie eine Vielzahl vorgeschlagener Gegenmaßnahmen im Hinblick auf ihre Wirksamkeit im Erschweren oder Unterbinden von Angriffe sowie die mit ihnen verbundenen Performance-Einbußen. [@transient-fail]

Bulck et al. entdeckten im Jahre 2020 eine neue Art von Spectre-Angriffen, genannt Load Value Injection. Angriffe dieser Art kombinieren Effekte, die bereits in Varianten von Spectre und Meltdown genutzt wurden, um Daten aus dem Kontext des Opfers zu extrahieren. [@lvi]

Neben Intel-Prozessoren wurden Spectre-Angriffe beispielsweise auch auf Prozessoren von AMD und ARM demonstriert [@spectre] [@transient-fail].
