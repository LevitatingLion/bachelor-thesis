# Evaluation {#sec:eval}

<!-- - in diesem kapitel:
  - zunächst system auf dem die evaluation ausgeführt wird
  - dann unterteilt nach verschiedenen angriffen/techniken
    - vorgehensweise der evaluation
    - ergebnisse der evaluation
    - phänomene beschreiben und schlüsse ziehen
    - vergleich mit anderen arbeiten
  - evaluiert werden:
    - cache-basierte seitenkanalangriffe
    - mistraining strategien
    - implementierte angriffe (ridl, wtf, zombieload, stl) -->

In diesem Kapitel werden die in [@sec:impl] beschriebenen Techniken und Angriffe in verschiedenen Varianten evaluiert. Zunächst wird in [@sec:eval-system] das System beschrieben, auf dem diese Evaluation durchgeführt wird. Dabei handelt es sich um ein aktuelles Linux-System, auf dem Maßnahmen gegen Spectre-Angriffe deaktiviert wurden. Anschließend werden in [@sec:eval-sidechannel] die Cache-basierten Seitenkanalangriffe betrachtet, die in [@sec:grundlagen-sidechannel] beschrieben wurden. Diese Seitenkanalangriffe werden evaluiert hinsichtlich ihrer Fähigkeit, den Zustand von Cachezeilen korrekt zu klassifizieren. Danach werden in [@sec:eval-transient] die Techniken, die Transient Execution durch eine falsche Vorhersage des Branch Predictors auslösen, hinsichtlich ihrer Zuverlässigkeit evaluiert. Diese wurden in [@sec:impl-teile-exception] beschrieben. Schließlich evaluiert [@sec:eval-angriffe] verschiedene Varianten von drei der konkreten Spectre-Angriffe, die in [@sec:impl-angriffe] beschrieben wurden. Dabei handelt es sich um RIDL, Write Transient Forwarding und ZombieLoad. Dies geschieht hinsichtlich der erreichten Datenrate mit zugehöriger Erfolgsrate, sowie der Dauer des Angriffs. Innerhalb jedes Unterkapitels wird zunächst die Vorgehensweise der Evaluation beschrieben. Anschließend werden die ermittelten Ergebnisse dargestellt, um schließlich einen Vergleich mit Ergebnissen anderer Arbeiten zu ziehen. Der verbleibende Angriff aus [@sec:impl-angriffe], Store-to-Leak, unterscheidet sich in seiner Funktionsweise von den anderen drei Angriffen. Deshalb wird Store-to-Leak separat in [@sec:eval-stl] evaluiert.

<!-- - aller code ist der bachelorarbeit beigelegt
- mit dokumentation in README
- run.sh um alle angriffe zu bauen und alle daten zur evaluation zu erheben
- eval_and_plot.py um erhobene daten auszuwerten und alle plots und tabellen zu erstellen -->

Der Programmcode aller erstellten Implementierungen ist dieser Bachelorarbeit beigelegt. Diese und weitere beigelegte Dateien, sowie benötigte externe Abhängigkeiten sind in der Datei `README.md` aufgelistet. Außerdem enthält `run.sh` ein Programm, das alle zur Evaluation verwendeten Daten erhebt. `eval_and_plot.py` enthält ein Programm, das die erhobenen Daten auswertet, sowie alle in diesem Kapitel verwendeten Diagramme und Tabellen erstellt.

## Hard- und Softwareumgebung {#sec:eval-system}

<!-- - System auf dem evaluiert wird beschreiben
  - Thinkpad E580
  - Intel(R) Core(TM) i5-8250U CPU, Kaby Lake R microarchitecture
  - 4 physische, 8 logische kerne
  - L1d, L1i cache jeweils: 128 KiB 8-way set associative
  - L2 cache: 1 MiB 4-way set associative
  - L3 cache: 6 MiB 12-way set associative
  - fußnote: cache infos aus `lscpu` und `/sys/devices/system/cpu/cpu0/cache/index*/ways_of_associativity`
  - 16GiB RAM
  - Manjaro Linux, Kernel 5.10.18-1
  - kernel boot parameter: mitigations=off nokaslr
  - hyperthreading aktiv, performance governor
  - microcode version: 0xe0
    - neueste version, system ist aber noch verwundbar, da viele mitigations durch das betriebssystem ausgeschaltet sind
  - clang 11.1.0, GNU C Library 2.33
  - mit minimaler last, neben systemprogrammen läuft nichts -->

Das System, auf dem die Evaluation durchgeführt wird, basiert auf einem Thinkpad E580. Als Prozessor kommt ein Intel Core i5-8250U der Kaby Lake R Mikroarchitektur zum Einsatz. Dieser besitzt 4 physische und 8 logische Prozessorkerne. Die L1-Caches für Instruktionen und für Daten sind jeweils 8-Way Set-Associative mit einer Kapazität von 32 KiB pro physischem Prozessorkern. Die L2-Caches sind 4-Way Set-Associative mit einer Kapazität von 256 KiB pro physischem Prozessorkern. Der L3-Cache ist 12-Way Set-Associative mit einer Kapazität von 6 MiB und wird von allen physischen Prozessorkernen gemeinsam verwendet[^system-cache-info]. Das System verfügt über 16 GiB Hauptspeicher. Als Betriebssystem wird Manjaro Linux mit einem Kernel der Version `5.10.18-1` eingesetzt. Dieser wurde mit den zusätzlichen Parametern `nokaslr` und `mitigations=off` gestartet [@linux-doc-cmdline], um KASLR (siehe [@sec:grundlagen-speicher-adressbereich]) und Maßnahmen gegen Spectre-Angriffe zu deaktivieren. Insbesondere ist Hyper-Threading aktiviert (siehe [@sec:grundlagen-mikroarch]). Das neueste Mikrocode-Update der Version `0xE0` ist installiert. Da viele Maßnahmen gegen Spectre-Angriffe durch das Betriebssystem deaktiviert wurden, ist das System dennoch angreifbar. Als *Governor* der Prozessorfrequenz wird der `performance` Governor eingesetzt [@linux-doc-governor]. Die relevanten installierten Softwarepakete sind `clang` in der Version `11.1.0` und die GNU C Library in der Version `2.33`. Die Evaluation wird mit minimaler Systemlast durchgeführt; neben den notwendigen Systemprogrammen werden keine weiteren Programme gestartet.

[^system-cache-info]: Die Größe und Assoziativität der Caches wurde über das Kommando `lscpu` und die Dateien `/sys/devices/system/cpu/cpu0/cache/index*/ways_of_associativity` ermittelt.

<!-- - andere arbeiten führen außerdem noch real-world attacks durch, in settings die hier out-of-scope sind (e.g. kernel, vm, sgx, tsx) -->

## Cache-basierte Seitenkanalangriffe {#sec:eval-sidechannel}

<!-- - evaluiert werden cache-basierte seitenkanalangriffe die in sec:grundlagen-sidechannel und sec:impl-teile-cache beschrieben wurden
- also flush+reload und flush+flush -->

<!-- In diesem Kapitel werden die Cache-basierten Seitenkanalangriffe evaluiert, die in [@sec:grundlagen-sidechannel] und [@sec:impl-teile-cache] beschrieben wurden. Dies umfasst also Flush+Reload und Flush+Flush. -->

In diesem Kapitel werden Flush+Reload und Flush+Flush als Cache-basierte Seitenkanalangriffe evaluiert. Diese wurden in [@sec:grundlagen-sidechannel] und [@sec:impl-teile-cache] beschrieben.

<!-- - vorgehensweise:
  - seitenkanalangriffe evaluiert so wie diese auch in den angriffen verwendet werden, wie in sec:impl-teile-cache beschrieben
  - fester wert wird wiederholt in den cache kodiert und durch den jeweiligen/gewählten seitenkanalangriff dekodiert
  - anstatt cachezeile als im cache oder nicht im cache zu klassifizieren, wird gemessene latenz protokolliert, für cacheline die tatsächlich im cache ist und eine ausgewählte andere
  - jeweils 10 millionen messungen -->

<!-- Die Seitenkanalangriffe werden ungefähr in dem Kontext evaluiert, in dem sie auch in den konkreten Angriffen verwendet werden (siehe [@sec:impl-teile-cache]). Um konsistente Daten erfassen zu können, müssen jedoch einzelne Änderungen an der Vorgehensweise vorgenommen werden. -->
<!-- Diese gemessene Latenz wird anschließend protokolliert für die Cachezeile, die sich tatsächlich im Cache befindet, sowie für eine beliebige andere. -->

Zum Zweck der Evaluation wird ein fester Wert wiederholt in den Cache kodiert und anschließend mithilfe des zu evaluierenden Seitenkanalangriffs dekodiert. Anstelle wie in [@sec:impl-teile-cache] beschrieben jede der 256 beobachteten Cachezeilen als im Cache oder nicht im Cache zu klassifizieren, wird dabei direkt die gemessene Zugriffslatenz erfasst. Von zwei ausgewählten Cachezeilen wird diese Latenz protokolliert: Von der Cachezeile, die sich tatsächlich im Cache befindet, sowie von einer beliebigen derer, die sich nicht im Cache befinden. Auf diese Art und Weise werden jeweils zehn Millionen Zugriffslatenzen erfasst. Programmcode, der die Messungen wie beschrieben durchführt, ist dieser Arbeit in der Datei `sidechannel.c` beigelegt.

<!-- - code ist der bachelorarbeit beigelegt, in sidechannel.c -->

<!-- - dabei wird die im zuge des angriffes gemessene latenz protokolliert
- jeweilige operation (fr: load, ff: flush) ausführen auf daten im cache oder nicht im cache und latenz der operation messen
- latenzen in histogram plotten -->

<!-- - latenz wenn im cache vs nicht im cache
  - plot: histogram der beobachteten latenzen, mit threshold als vertikale linie eingezeichnet
  - x: latenz in cycles, y: anteil der messwerte in %
- daraus threshold ermitteln, von hand -->

#### Ergebnisse

<!-- ![caption](plots/sidechannel_reload.pdf){#fig:eval-sidechannel-reload} -->

![Histogramm der gemessenen Latenzen für die ausgewählten Seitenkanalangriffe.](plots/sidechannel.pdf){#fig:eval-sidechannel}

<!-- - abbildung fig:eval-sidechannel zeigt ein histogramm der gemessenen latenzen für beide seitenkanäle
- gemessene latenzen beinhalten den overhead der messung / die zeit die für die messung selber benötigt wird, siehe sec:impl-teile-latenz
- wie in [@sec:impl-teile-cache-threshold] beschrieben wird auf grundlage dieses histogramms manuell ein geeigneter threshold gewählt
- dieser ist als vertikale linie eingezeichnet -->

[@fig:eval-sidechannel] zeigt ein Histogramm der gemessenen Zugriffslatenzen für beide Seitenkanalangriffe (Flush+Reload und Flush+Flush) und jeweils beide Kategorien von Cachezeilen (im Cache und nicht im Cache). Die gemessenen Zugriffslatenzen beinhalten dabei die Zeit, die für die Messung selber benötigt wird (siehe [@sec:impl-teile-latenz]). Auf Grundlage dieses Histogramms wird manuell ein geeigneter Threshold gewählt, wie in [@sec:impl-teile-cache-threshold] beschrieben. Dieser Threshold ist in [@fig:eval-sidechannel] als vertikale Linie eingezeichnet.

<!-- - anteil der korrekt eingeordneten zustände mit ermitteltem threshold (tabelle)
  - true-positive und true-negative raten -->

| Seitenkanalangriff | Threshold (Zyklen) | Sensitivität (%) | Spezifität (%) |
| ------------------ | ------------------ | ---------------- | -------------- |
| Flush+Reload       | $100$              | $100{,}0$        | $99{,}99$      |
| Flush+Flush        | $124$              | $49{,}68$        | $93{,}33$      |

: Der gewählte Threshold und die ermittelten Metriken für jeden Seitenkanalangriff, angegeben auf 4 gültige Ziffern. {#tbl:eval-sidechannel}

<!-- - tabelle tbl:eval-sidechannel zeigt diese thresholds
- zeigt auch die sich daraus ergebende sensitivität und spezifität mit der der jew. seitenkanalangriff eine cacheline einordnet / den zustand einer cacheline bestimmt
- sensitivität / richtig-positiv-rate: anteil der cachezeilen im cache, die als im cache erkannt wurden [@classification, Kap. 2.3]
  - alternativ: geschätzte wahrscheinlichkeit einer cachezeile im cache, richtig erkannt zu werden
- spezifität / richtig-negativ-rate:: anteil der cachezeilen nicht im cache, die als nicht im cache erkannt wurden [@classification, Kap. 2.3]
  - alternativ: geschätzte wahrscheinlichkeit einer cachezeile nicht im cache, richtig erkannt zu werden
- accuracy/Treffergenauigkeit oder precision/Genauigkeit werden üblicherweise verwendet, um binäre klassifikatoren zu beurteilen [@classification, Kap. 2.1, 2.2]
  - können hier nicht sinnvoll angewendet werden, da diese metriken von unbalancierten datensätzen beeinflusst werden [@classification, Kap. 2.1]
    - da diese metriken davon abhängen, wie balanciert die klassifizierten datensätze sind
    - balanciertheit/prävalenz ~ verhältnis zwischen positives und negatives
  - wie balanciert der datensatz ist ist abhängig vom konkreten angriff
  - daher werden hier sensitivität und spezifität verwendet, diese sind unabhängig von der prävalenz/balanciertheit -->

Die ermittelten Thresholds sind außerdem in [@tbl:eval-sidechannel] angegeben. Diese zeigt auch die *Sensitivität* und *Spezifität*, mit der der jeweilige Seitenkanalangriff den Zustand einer Cachezeile klassifiziert. Die Sensitivität (auch Richtig-positiv-Rate) bezeichnet den Anteil der Cachezeilen im Cache, die korrekt als solche erkannt werden [@classification, Kap. 2.3]. Alternativ kann die Sensitivität auch interpretiert werden als die Wahrscheinlichkeit, dass eine Cachezeile, die sich im Cache befindet, richtig erkannt wird. Die Spezifität (auch Richtig-negativ-Rate) bezeichnet den Anteil der Cachezeilen nicht im Cache, die korrekt als solche erkannt werden [@classification, Kap. 2.3]. Alternativ kann die Spezifität auch interpretiert werden als die Wahrscheinlichkeit, dass eine Cachezeile, die sich nicht im Cache befindet, richtig erkannt wird.

Anstelle der Sensitivität und Spezifität werden üblicherweise die *Treffergenauigkeit* (engl. *accuracy*) und die *Genauigkeit* (engl. *precision*) verwendet, um binäre Klassifikatoren zu beurteilen [@classification, Kap. 2.1, 2.2]. Diese Metriken können hier jedoch nicht sinnvoll angewandt werden, da sie davon beeinflusst werden, wie balanciert die klassifizierten Datensätze sind [@classification, Kap. 2.1]. Ein Datensatz wird *balanciert* genannt, wenn dieser ähnlich viele tatsächlich positive wie tatsächlich negative Messpunkte beinhaltet [@classification, Kap. 2.1]. Dem gegenüber wird ein Datensatz, der wesentlich mehr tatsächlich positive oder tatsächlich negative Messpunkte beinhaltet, *unbalanciert* genannt [@classification, Kap. 2.1]. Im Fall der hier betrachteten Seitenkanalangriffe hängt die Balance der gemessenen Daten von dem konkreten Anwendungsfall ab. Um die berechneten Metriken für verschiedene Anwendungsfälle verwenden zu können, werden hier die Sensitivität und die Spezifität erfasst. Diese Metriken sind unabhängig von der Balance der gemessenen Daten.

#### Auftretende Phänomene

<!-- - auftretende phänomene beschreiben
  - flush+reload: verteilungen sehr weit auseinander, hits sehr schnell (30-50), misses sehr langsam (150-200)
  - flush+flush: verteilungen sehr nah beieinander und sogar signifikant überlappend, hits und misses ähnliche latenz (100-150)

- schlüsse ziehen
  - flush+reload viel besser zu unterscheiden als flush+flush
    - deckt sich mit ermittelter sensitivität und spezifität
  - bei wenigen erwarteten cache hits ist flush+flush deutlich schneller
    - das trifft hier zu, da normalerweise von den 256 beobachteten cachelines (siehe sec:impl-teile-cache) eine im cache ist -->

Wie in [@fig:eval-sidechannel] zu erkennen ist, haben bei Verwendung von Flush+Reload die beiden Verteilungen für Cachezeilen, die im Cache sind, und Cachezeilen, die nicht im Cache sind, einen Abstand von ungefähr 150 Zyklen. Ein Latenzmessung lässt sich also in den meisten Fällen eindeutig einer der beiden Verteilungen zuordnen. Dies deckt sich mit der hohen Sensitivität und Spezifität, die für Flush+Reload berechnet wurde. Außerdem hat dieser Abstand Auswirkungen auf die Ausführungszeit von Flush+Reload. Ein höheres Verhältnis von beobachteten Cache Misses zu beobachteten Cache Hits erhöht die Ausführungszeit des Angriffs.

Bei Verwendung von Flush+Flush hingegen liegen die Verteilungen für Cachezeilen, die im Cache sind, und Cachezeilen, die nicht im Cache sind, mit einem Abstand von ungefähr 2 Zyklen nah beieinander. Eine Latenzmessung lässt sich also weniger zuverlässig einer der beiden Verteilungen zuordnen. Dies deckt sich mit der reduzierten Sensitivität und Spezifität von Flush+Flush im Vergleich zu Flush+Reload. Insbesondere die niedrige Sensitivität von Flush+Flush stimmt damit überein, dass die Verteilung der Cachezeilen im Cache zu einem großen Teil unter dem Threshold liegt. Da im Fall von Flush+Flush Cache Hits und Cache Misses ähnliche Latenzen haben, wird die Ausführungszeit von Flush+Flush kaum von dem Verhältnis von beobachteten Cache Misses zu beobachteten Cache Hits beeinflusst. Werden wesentlich weniger Cache Hits als Cache Misses erwartet, so ist Flush+Flush folglich schneller als Flush+Reload. Dies ist bei den in [@sec:eval-angriffe] evaluierten Angriffen der Fall, da bei diesen normalerweise genau eine der 256 beobachteten Cachezeilen eingeladen ist (siehe [@sec:impl-teile-cache]).

#### Vergleich mit anderen Arbeiten

<!-- - vergleich mit anderen arbeiten
  - flush+reload:
    - from l1d cache 44 cycles including measuring overhead, from memory mostly 270-290 very few ~1000 including measuring overhead
    - hier: 99.98% aus dem l1d cache brauchen höchstens 50 cycles, 97.41% aus dem speicher brauchen 120-250, 2.070% aus dem speicher brauchen 600-800
    - leicht andere anzahl von zyklen, aber die form/art der verteilung stimmt überein
    - ergebnisse stimmen also mit dem f+r paper überein
  - flush+flush:
    - distance between peaks ~10 cycles, 100-200 cycles for both
    - hier: distanz zwischen maxima sind 2 zyklen, 99.99% cached und 99.98% uncached liegen zwischen 100 und 200 zyklen
    - die verteilungen sind sehr viel näher beieinander, die form beider verteilungen stimmt aber mit dem paper überein
  - insgesamt stimmen die hier erhaltenen ergebnisse mit denen aus den papern überein -->

In der Literatur zu Flush+Reload wurde für Cache Hits eine Latenz von $44$ Zyklen angegeben, sowie für Cache Misses eine Latenz von $270$ bis $290$ Zyklen mit wenigen Ausreißern um $1\,000$ Zyklen [@flush-reload, Kap. 3]. In dieser Arbeit wurde für $99{,}97\,\%$ der Cache Hits eine Latenz von unter $50$ Zyklen gemessen. Außerdem lagen $97{,}00\,\%$ der Cache Misses bei einer Latenz von $150$ bis $200$ Zyklen und $2{,}070\,\%$ der Cache Misses zwischen $600$ und $800$ Zyklen. Die Form der Verteilungen aus der Literatur stimmt also mit der Form der hier beobachteten Verteilungen überein. Die unterschiedliche konkrete Anzahl von Zyklen kann durch Unterschiede in der Leistungsfähigkeit der verwendeten Systeme erklärt werden.

In der Literatur zu Flush+Flush wurde eine Distanz von ungefähr $10$ Zyklen zwischen den jeweils am häufigsten gemessenen Latenzen von Cache Hits und Cache Misses angegeben [@flush-flush, Kap. 3]. Außerdem wurde für alle Messungen eine Latenz zwischen $100$ und $200$ Zyklen angegeben [@flush-flush, Abb. 1]. In dieser Arbeit wurde eine Distanz von $2$ Zyklen zwischen den am häufigsten gemessenen Latenzen ermittelt. Zusätzlich lagen $99{,}99\,\%$ der Cache Hits und $99{,}98\,\%$ der Cache Misses bei einer Latenz zwischen $100$ und $200$ Zyklen. Die Form der Verteilungen aus der Literatur stimmt also mit der Form der hier beobachteten Verteilungen überein. Die unterschiedliche Distanz zwischen den am häufigsten gemessenen Latenzen lässt sich durch Unterschiede in der Mikroarchitektur der verwendeten Prozessoren erklären.

Insgesamt können also die Ergebnisse der Literatur für beide Cache-basierten Seitenkanalangriffe reproduziert werden.

## Unterdrückung von Prozessor-Exceptions {#sec:eval-transient}

<!-- - evaluiert werden techniken, um transient execution durch falsche vorhersage des branch predictors auszulösen
- dies ermöglicht die unterdrückung von exceptions, wie in sec:impl-teile-exception beschrieben
- wie schon erwähnt können dabei verschiedene Arten von Sprüngen verwendet werden:
  - bedingte sprünge
  - indirekte sprünge
  - rücksprünge aus funktionen
- alle arten werden hier evaluiert und verglichen -->

In diesem Kapitel werden verschiedene Techniken, die Transient Execution durch eine falsche Vorhersage des Branch Predictors auslösen, hinsichtlich ihrer Zuverlässigkeit evaluiert. Diese Techniken ermöglichen die Unterdrückung von Prozessor-Exceptions (siehe [@sec:impl-teile-exception]). Wie bereits in [@sec:impl-spectre-prediction] erwähnt, kann eine falsche Vorhersage des Branch Predictors durch verschiedene Arten von Sprüngen ausgelöst werden: Bedingte Sprünge, indirekte Sprünge und Rücksprünge aus Funktionen. Diese drei Arten werden im Folgenden evaluiert und verglichen.

<!-- - evaluiert wird technik zur unterdrückung von exceptions durch transient execution die in sec:impl-teile-exception beschrieben wurde
- technik erfordert auslösen von transient execution durch falsche vorhersage des branch predictors -->

<!-- - vorgehensweise jeweils:
  - training: target branch mehrmals ausführen in richtung die den zugriff ausführt
  - target branch ausführen in die richtung die nichts tut
    - so, dass das ziel des branches von daten abhängt, die aus dem hauptspeicher geladen werden müssen
    - auf diese weise wird das transient execution window verlängert, bis diese daten geladen wurden
    - in transient execution einen festen wert in den cache kodieren
  - per flush+reload feststellen, ob wert korrekt kodiert wurde, und dies protokollieren
  - außerdem werden benötigte cycles für training und angriff ermittelt -->
<!-- Erst wenn die angeforderten Daten aus dem Hauptspeicher geladen wurden, kann die Vorhersage als falsch erkannt und die Transient Execution abgebrochen werden. -->

Wie in [@sec:impl-teile-exception] beschrieben, wird bei den betrachteten Techniken zunächst der Branch Predictor trainiert[^ret-no-training], um anschließend zuverlässig eine falsche Vorhersage auslösen zu können. Zum Zweck der Evaluation wird der Branch Predictor trainiert, indem der anvisierte Sprung wiederholt in die gewünschte Richtung ausgeführt wird. Anschließend wird dieser Sprung in die andere Richtung ausgeführt. Dies geschieht dabei so, dass das Ziel des Sprunges von Daten abhängt, die aus dem Hauptspeicher geladen werden müssen. Auf diese Weise wird der Branch Predictor zu einer Vorhersage gezwungen. Die gewünschte Richtung des Sprunges wird also in einer Transient Execution ausgeführt. Innerhalb dieser Transient Execution wird anschließend ein fester Wert in den Cache kodiert. Nach Ende der Transient Execution wird mittels Flush+Reload festgestellt und protokolliert, ob dieser feste Wert korrekt kodiert wurde. Außerdem wird die Anzahl der benötigten Zyklen für das Training des Branch Predictors und für den eigentlichen Angriff ermittelt und protokolliert.

<!-- - ret braucht kein training: anders als bei bedingten sprüngen basiert die vorhersage von rücksprüngen nicht auf vorherigen rücksprüngen, sondern auf vorhergehenden call instruktionen [@spectre-spectrersb, Kap. 3.1] -->

[^ret-no-training]: Die Technik, die Rücksprünge aus Funktionen verwendet, erfordert kein Training des Branch Predictors. Anders als bei bedingten oder indirekten Sprüngen basiert die Vorhersage des Branch Predictors bei Rücksprüngen nicht auf vorherigen Sprüngen, sondern auf vorhergehenden Funktionsaufrufen [@spectre-spectrersb, Kap. 3.1].

<!-- - code ist der bachelorarbeit beigelegt, in misprediction.c -->

#### Ergebnisse

<!-- - ergebnisse in tabelle
  - mistraining strategie vs. anteil der erfolgreichen versuche vs. cycles -->

| Art verwendeter Sprünge | Erfolgsrate (%)         | Training (Zyklen)       | Angriff (Zyklen)      |
| ----------------------- | ----------------------- | ----------------------- | --------------------- |
| Direkte Sprünge         | $99{,}93 \pm 0{,}1006$  | $3{,}446 \pm 0{,}03336$ | $224{,}0 \pm 10{,}17$ |
| Indirekte Sprünge       | $99{,}99 \pm 0{,}05006$ | $3{,}208 \pm 0{,}01991$ | $254{,}7 \pm 8{,}253$ |
| Rücksprünge             | $99{,}99 \pm 0{,}05985$ | ---                     | $232{,}2 \pm 9{,}705$ |

: Die ermittelten Metriken für jede Art verwendeter Sprünge, angegeben als $\mu\pm\sigma$ auf 4 gültige Ziffern. {#tbl:eval-mistraining}

<!-- - training besteht aus 4096 sprüngen
- für jede art von sprung wird training und angriff 10000 mal durchgeführt und erfasste metriken zu einem messpunkt gemittelt
- insgesamt werden auf diese weise 1000 messpunkte ermittelt, mithilfe derer die mittelwerte und standardabweichungen der metriken berechnet werden
- diese ergebnisse sind in tabelle tbl:eval-mistraining zu sehen
- spalte erfolgsrate ist anteil der angriffe, die den festen wert erfolgreich in den cache kodiert haben
- spalte dauer training ist zahl benötigter zyklen für einen ausgeführten sprung während des trainings
- spalte dauer angriff ist zahl benötigter zyklen für den falsch vorhergesagten sprung während des angriffes / sprung, der transient execution auslöst -->

Das Training besteht aus $4\,096$ ausgeführten Sprüngen. Für jede betrachtete Art von Sprung wird das Training und der eigentliche Angriff jeweils $10\,000$ mal durchgeführt. Die dabei erfassten Metriken werden zu einem einzelnen Messpunkt gemittelt. Insgesamt werden auf diese Weise $1\,000$ Messpunkte ermittelt, aus denen anschließend die Mittelwerte und Standardabweichungen der Metriken berechnet werden. Programmcode, der die beschriebenen Techniken ausführt und die benötigten Daten ermittelt, ist dieser Arbeit in der Datei `misprediction.c` beigelegt. Die daraus berechneten Ergebnisse, unterteilt nach der Art der verwendeten Sprünge, sind in [@tbl:eval-mistraining] dargestellt. Die Spalte *Erfolgsrate* zeigt dabei den Anteil der Angriffe, die den festen Wert erfolgreich in den Cache kodiert haben. *Training* bezeichnet die Zahl benötigter Zyklen für einen einzelnen Sprung während des Trainings und *Angriff* bezeichnet die Zahl benötigter Zyklen für den Sprung während des Angriffs.

#### Auftretende Phänomene

<!-- - auftretende phänomene beschreiben
  - alle techniken funktionieren sehr zuverlässig
  - dauer des trainings ist in beiden fällen sehr gering
    - branch predictor liegt während des trainings meistens richtig, sprung braucht wenig zyklen
  - angriff dauert wesentlich länger als training
    - branch predictor liegt fast immer falsch
    - falsche vorhersage kann erst erkannt werden, wenn die benötigten daten aus dem hauptspeicher geladen wurden
    - falsche vorhersage des branch predictors muss zurückgerollt werden (siehe sec:grundlagen-mikroarch-transient)
  - ind angriff dauert etwas länger als cond angriff
    - indirekter sprung ist komplexer, prozessor braucht vllt länger um falsche vorhersage zu korrigieren
  - dauer des angriffes von ret liegt zwischen der dauer der beiden anderen
    - stimmt mit der komplexität relativ zu den anderen überein -->

Wie in [@tbl:eval-mistraining] zu sehen ist, funktionieren alle Techniken sehr zuverlässig mit einer durchschnittlichen Erfolgsrate über $99{,}9\,\%$. Die Dauer eines einzelnen Sprunges während des Trainings ist mit weniger als $4$ Zyklen in allen Fällen sehr gering. Dies ist darauf zurückzuführen, dass der Branch Predictor während des Trainings fast alle Sprünge korrekt vorhersagt, wodurch diese sehr schnell ausgeführt werden können. Dem gegenüber benötigen die Angriffe mit $220$ bis $250$ Zyklen wesentlich mehr Zeit. In diesem Fall trifft der Branch Predictor in fast allen Fällen eine falsche Vorhersage. Wie in [@sec:grundlagen-mikroarch-transient] erklärt, ist dies mit einem starken Performanceverlust verbunden. Außerdem braucht ein Angriff unter Verwendung direkter Sprünge im Schnitt ungefähr $10$ Zyklen weniger als ein Angriff, der Rücksprünge verwendet. Dieser wiederum braucht im Schnitt ungefähr $20$ Zyklen weniger als ein Angriff durch indirekte Sprünge. Dieser Umstand legt die Vermutung nahe, dass der Prozessor umso länger für die Korrektur einer falschen Vorhersage braucht, je komplexer die Art des vorhergesagten Sprunges ist. Direkte Sprünge kodieren die möglichen Sprungziele in der Instruktion selber, Rücksprünge laden das Sprungziel von einer bekannten Speicheradresse, und indirekte Sprünge können das Sprungziel von einer Speicheradresse laden, die aus mehreren Registern zusammengesetzt ist.

<!-- - schlüsse ziehen
  - return benötigt kein training, ist einfacher zu implementieren
  - wird aus diesen gründen zur evaluation der angriffe benutzt -->

Wie durch die genannten Ergebnisse bestätigt, kann bei Verwendung von Rücksprüngen eine Transient Execution zuverlässig ausgelöst werden. Dabei ist kein Training des Branch Predictors nötig. Dies vereinfacht eine Implementierung der Technik und erhöht außerdem ihre Geschwindigkeit. Aus diesen Gründen wird in [@sec:eval-angriffe], in dem konkrete Angriffe evaluiert werden, Transient Execution zur Unterdrückung von Exceptions stets mithilfe von Rücksprüngen ausgelöst.

<!-- - vergleich mit anderen arbeiten -->

## Angriffe auf Daten anderer Prozesse {#sec:eval-angriffe}

<!-- - in sec:impl-angriffe beschriebene angriffe werden evaluiert, in verschiedenen varianten
- alle varianten der hier betrachteten angriffe auf die gleiche art und weise
- hier nur ridl, wtf, zombieload; store-to-leak separat in sec:eval-stl -->

In diesem Kapitel werden RIDL, Write Transient Forwarding und ZombieLoad, die in [@sec:impl-angriffe] beschrieben wurden, in verschiedenen Varianten evaluiert. Dies geschieht hinsichtlich der erreichten Datenrate mit zugehöriger Erfolgsrate, sowie der Dauer des Angriffs. Wie schon in [@sec:impl-angriffe-stl] beschrieben, unterscheidet sich Store-to-Leak in seiner Funktionsweise von den anderen drei Angriffen. Deshalb wird Store-to-Leak separat in [@sec:eval-stl] evaluiert.

<!-- - vorgehensweise:
  - opfer prozess wird ausgeführt, der wiederholt einen festen wert liest oder schreibt
  - um ein byte zu ermitteln, wird angriff wiederholt (200x) ausgeführt und das am meisten aus dem cache dekodierte byte als ergebnis genommen
  - angriff wird genutzt, um wiederholt (100x) bytes zu ermitteln
    - erhobene metriken werden über diese wiederholungen gemittelt, um einen messpunkt zu erhalten
  - es werden nacheinander 1000 dieser messpunkte ermittelt, um zeitlich separierte messergebnisse zu erhalten
    - mittelwert und standardabweichung werden über diese gebildet -->

Alle hier betrachteten Varianten werden auf die gleiche Art und Weise evaluiert. Diese wird im Folgenden beschrieben. Neben dem angreifenden Prozess wird stets ein Opfer-Prozess ausgeführt. Dieser liest oder schreibt wiederholt einen festen Wert, sodass dieser von dem angreifenden Prozess aus einem Puffer der Mikroarchitektur extrahiert werden kann. Um ein einzelnes Byte zu extrahieren, führt der angreifende Prozess den jeweiligen Angriff $200$ mal aus. Dabei wird das am häufigsten aus dem Cache dekodierte Byte übernommen. Dies wird wiederholt, um $100$ Bytes zu extrahieren. Über diese Wiederholungen werden alle erfassten Metriken gemittelt, wodurch ein einziger Messpunkt erhalten wird. Auf diese Art und Weise werden nacheinander $1\,000$ Messpunkte ermittelt, um die Messungen zeitlich zu separieren. Aus diesen Messpunkten werden anschließend die Mittelwerte und Standardabweichungen der Metriken berechnet.

<!-- - konkretes szenario -->

Die Vorgehensweise dieser Evaluation stellt das Szenario eines angreifenden Nutzerprozesses nach, der Daten eines anderen (nicht kooperierenden) Prozesses extrahiert. Beispielsweise kann ein solcher Nutzerprozess wiederholt das `passwd`-Programm aufrufen und angreifen, um das root-Passwort des Systems zu extrahieren [@ridl, Kap. VI.A]. Andere konkrete Angriffe aus diesem Szenario ermöglichen das Extrahieren von Seitenverläufen aus Webbrowsern [@zombieload, Kap. 6.4] oder geheimen Schlüsseln aus kryptographischen Anwendungen [@zombieload, Kap. 6.1].

<!-- - dabei werden verschiedene metriken erfasst
  - erfolgsrate des angriffes: anteil der erfolgreich/korrekt ermittelten bytes
  - erreichte datenrate: berechnet aus der dauer des angriffes (in echtzeit)
  - dauer des unberechtigten datenzugriffes und der kodierung in den cache in zyklen
  - dauer des dekodieren aus dem cache in zyklen
- von allen metriken werden die mittelwerte und standardweichungen berechnet -->

Es werden die folgenden Metriken erfasst:

- Die **Erfolgsrate** des Angriffs. Dies ist der Anteil der korrekt ermittelten Bytes.

- Die erreichte **Datenrate**. Diese wird berechnet aus der Dauer des Angriffs. Die Erfolgsrate fließt nicht in die Berechnung ein; für sinnvolle Vergleiche muss die Datenrate also stets mit der Erfolgsrate gemeinsam betrachtet werden.

- Die Dauer des unberechtigten Datenzugriffs und des **Kodieren** in den Cache.

- Die Dauer des **Dekodierens** der übertragenen Daten aus dem Cache.

<!-- - wie in [@sec:impl-angriffe] erwähnt: code ist der bachelorarbeit beigelegt (ridl.c, wtf.c, storetoleak.c, zombieload.c) -->

Wie bereits in [@sec:impl-angriffe] erwähnt, ist der Programmcode aller Angriffe und ihrer Varianten dieser Arbeit beigelegt in den Dateien `ridl.c`, `wtf.c` und `zombieload.c`.

<!-- - Welche Größen ermitteln
  - Fehlerrate
  - Dauer des Angriffs und des decodieren in Cycles
  - Wall Time des gesamten Angriffs
  - daraus berechnet effektive Datenrate
  - Jeweils Sample Size, Mittelwert, Standardabweichung -->

<!-- - es werden verschiedene varianten der angriffe untersucht
- dabei ändern sich folgende bestandteile/aspekte:
  - der opfer prozess führt wiederholt einen speicherzugriff durch, der entweder lesend oder schreibend sein kann
  - als cache-basierter seitenkanalangriff wird entweder flush+reload oder flush+flush verwendet (siehe sec:grundlagen-sidechannel)
  - auftretende exceptions werden durch eine der techniken aus sec:impl-teile-exception behandelt. entweder wird die exception über einen signal handler behandelt, oder per transient execution vermieden
    - bei ridl kann die exception alternativ vermieden werden, indem der speicherzugriff auf eine ausgelagerte page erfolgt, siehe sec:impl-angriffe-ridl
  - zusätzlich gibt es für ridl und wtf spezifische variationen:
    - bei ridl werden daten entweder aus dem line-fill buffer, oder aus den load ports extrahiert, siehe sec:impl-angriffe-ridl
    - bei wtf erfolgt der initiale speicherzugriff entweder auf eine nicht-kanonische adresse, oder auf eine adresse des betriebssystem-kerns, siehe sec:impl-angriffe-wtf
- alle möglichen kombinationen können aus platzgründen nicht untersucht werden
  - daher wird für jeden angriff eine basiskonfiguration gewählt, und ausgehend von dieser nur einzelne aspekte variiert
  - beiliegender code ist jedoch so aufgebaut, dass einfach weitere konfigurationen evaluiert werden können -->

Von allen Angriffen werden verschiedene Varianten untersucht. Dabei werden folgende Aspekte der Angriffe verändert:

- Der **Opfer-Prozess** führt wiederholt einen Speicherzugriff durch. Dieser ist entweder lesend oder schreibend.

- Als **Cache-basierter Seitenkanalangriff** wird entweder Flush+Reload oder Flush+Flush verwendet (siehe [@sec:grundlagen-sidechannel]).

- Auftretende **Exceptions** werden durch eine der Techniken aus [@sec:impl-teile-exception] vermieden. Entweder wird die Exception über einen Signal Handler behandelt oder durch Transient Execution unterdrückt. Bei RIDL kann die Exception alternativ vermieden werden, indem der auslösende Speicherzugriff auf eine ausgelagerte Page erfolgt (siehe [@sec:impl-angriffe-ridl]).

- Zusätzlich gibt es für RIDL und Write Transient Forwarding **spezifische Variationen**: Bei RIDL werden Daten entweder aus dem Line-Fill Buffer oder aus den Load Ports extrahiert (siehe [@sec:impl-angriffe-ridl]), bei Write Transient Forwarding erfolgt der initiale Speicherzugriff entweder auf eine nicht-kanonische Adresse oder auf eine Adresse des Betriebssystem-Kerns (siehe [@sec:impl-angriffe-wtf]).

Alle möglichen Kombinationen der verschiedenen Aspekte zu untersuchen würde den Rahmen dieser Bachelorarbeit sprengen. Daher werden für jeden Angriff eine Basiskonfiguration gewählt und ausgehend von dieser nur einzelne Aspekte variiert. Der dieser Arbeit beigelegte Programmcode ist jedoch so aufgebaut, dass mit geringem Aufwand weitere Konfigurationen evaluiert werden können.

<!-- - varianten:
  - (8 bit auf einmal leaken oder nur 1 bit)
  - (optimize zero)
  - (cross-thread oder same-thread)
  - Cache Leak Technik (F+R, F+F)
  - Exception handling (custom signal handler) oder exception suppression (transient execution)
    - ridl: oder keine exception durch demand paging
  - ridl lfb und zombieload: alternatives victim
  - statt alle kombinationen, eine base konfiguration und dann nur einzelne teile ändern
    - alle kombinationen wären zu viel -->

<!-- - im folgenden werden die varianten der drei angriffe und ihre ergebnisse beschrieben/dargestellt
- anschließend werden allgemeine/übergreifende phänomene erläutert -->

In den folgenden Unterkapiteln werden die verschiedenen Varianten der drei betrachteten Angriffe untersucht. Anschließend werden die Beobachtungen der einzelnen Angriffe verglichen und übergreifende Phänomene erläutert.

### RIDL: Rogue In-Flight Data Load {#sec:eval-ridl}

<!-- - basiskonfiguration von ridl (siehe auch sec:impl-angriffe-ridl):
  - opfer prozess führt schreibenden speicherzugriff durch
  - als cache-basierter seitenkanalangriff wird flush+reload verwendet
  - die auftretende exception wird vermieden, indem auf eine ausgelagerte page zugegriffen wird
  - daten werden aus dem line-fill buffer extrahiert
- zusätzliche varianten, die jeweils einzelne aspekte ändern:
  - flush+flush: als cache-basierter seitenkanalangriff wird flush+flush verwendet
  - load: der opfer prozess führt lesende speicherzugriffe durch
  - load port: daten werden aus den load ports extrahiert, und opfer prozess führt lesende speicherzugriffe durch
  - signal: auftretende exceptions werden per signal handler behandelt (siehe sec:impl-teile-exception)
  - transient: auftretende exceptions werden durch transient execution vermieden (siehe sec:impl-teile-exception) -->

In der Basiskonfiguration von RIDL (siehe auch [@sec:impl-angriffe-ridl]) führt der Opfer-Prozess schreibende Speicherzugriffe durch. Als Cache-basierter Seitenkanalangriff wird Flush+Reload verwendet. Das Auftreten einer Exception wird vermieden, indem auf eine ausgelagerte Page zugegriffen wird. Daten werden aus dem Line-Fill Buffer extrahiert.

Zusätzlich zu dieser Basiskonfiguration werden verschiedene Varianten betrachtet, die jeweils in einzelnen Aspekten abweichen:

- **Flush+Flush**: Als Cache-basierter Seitenkanalangriff wird Flush+Flush verwendet.

- **Load**: Der Opfer-Prozess führt lesende Speicherzugriffe durch.

- **Load Port**: Daten werden aus den Load Ports extrahiert und der Opfer-Prozess führt lesende Speicherzugriffe durch[^load-port-load].

- **Signal**: Auftretende Exceptions werden mittels eines Signal Handlers behandelt (siehe [@sec:impl-teile-exception]).

- **Transient**: Auftretende Exceptions werden durch Transient Execution unterdrückt (siehe [@sec:impl-teile-exception]).

[^load-port-load]: Da aus den Load Ports nur lesende Speicherzugriffe extrahiert werden können, wird in diesem Fall auch der Opfer-Prozess angepasst.

<!-- - varianten:
  - default: target lfb, fault unmapped, cache fr, victim store
  - target lp
  - fault transient
  - cache ff
  - victim load -->

| Variante    | Erfolgsrate (%)       | Datenrate (B/s)       | Kodieren (Zyklen)     | Dekodieren (Zyklen)   |
| ----------- | --------------------- | --------------------- | --------------------- | --------------------- |
| Basis       | $97{,}96 \pm 9{,}721$ | $443{,}9 \pm 0{,}308$ | $1\,229 \pm 17{,}12$  | $94\,200 \pm 83{,}84$ |
| Flush+Flush | $2{,}515 \pm 3{,}918$ | $616{,}8 \pm 0{,}664$ | $1\,218 \pm 16{,}29$  | $48\,750 \pm 57{,}24$ |
| Load        | $56{,}27 \pm 19{,}02$ | $450{,}6 \pm 0{,}571$ | $1\,178 \pm 24{,}17$  | $91\,860 \pm 181{,}8$ |
| Load Port   | $11{,}40 \pm 5{,}126$ | $450{,}6 \pm 0{,}570$ | $1\,182 \pm 23{,}53$  | $91\,880 \pm 180{,}0$ |
| Signal      | $74{,}69 \pm 32{,}20$ | $745{,}5 \pm 0{,}490$ | $1\,789 \pm 11{,}97$  | $94\,640 \pm 61{,}11$ |
| Transient   | $0{,}000 \pm 0{,}000$ | $757{,}1 \pm 0{,}564$ | $184{,}0 \pm 0{,}423$ | $94\,760 \pm 70{,}62$ |

: Die ermittelten Metriken für jede RIDL-Variante, angegeben als $\mu\pm\sigma$ auf 4 gültige Ziffern. {#tbl:eval-ridl}

<!-- - tabelle tbl:eval-ridl zeigt die ermittelten metriken der verschiedenen varianten
- metriken wurden in sec:eval-angriffe erklärt
- wurden ermittelt wie in sec:eval-angriffe beschrieben
- gemessene latenzen beinhalten den overhead der messung, siehe sec:impl-teile-latenz -->

[@tbl:eval-ridl] zeigt die ermittelten Metriken für alle betrachteten Varianten von RIDL. Die Bedeutung dieser Metriken und die Vorgehensweise ihrer Erfassung wurden bereits in [@sec:eval-angriffe] erläutert. Die gemessenen Latenzen beinhalten dabei auch die Zeit, die für die Messung selber benötigt wird (siehe [@sec:impl-teile-latenz]).

<!-- - auftretende phänomene beschreiben
  - basiskonfiguration funktioniert zuverlässig
  - aus dem lfb leaken stores besser als loads
  - aus dem lp leakt weniger gut als aus dem lfb
  - transient funktioniert nicht
    - vllt wird die transient execution bei dem invaliden speicherzugriff terminiert
    - oder der bug tritt während transient execution nicht auf, daten werden nicht aus dem lfb geladen
  - fault signal ein bisschen weniger zuverlässig und mehr zyklen für kodierung als fault unmapped
    - durch zusätzlichen overhead durch signal handler
    - aber: weniger datenrate bei unmapped, da page jedes mal ausgelagert werden muss -->

Wie in [@tbl:eval-ridl] zu sehen ist, funktioniert die Basiskonfiguration von RIDL zuverlässig mit einer Erfolgsrate von $97{,}96\,\%$. Die Daten lesender Speicherzugriffe lassen sich mit einer Erfolgsrate von $56{,}27\,\%$ wesentlich weniger zuverlässig aus dem Line-Fill Buffer extrahieren als die Daten schreibender Speicherzugriffe. Außerdem werden Daten aus den Load Ports mit einer Erfolgsrate von $11{,}40\,\%$ deutlich weniger zuverlässig extrahiert als aus dem Line-Fill Buffer.

Die Variante, die Exceptions durch Transient Execution unterdrückt, extrahierte in keinem der beobachteten Fälle Daten erfolgreich. Eine mögliche Erklärung dafür ist, dass die Transient Execution bereits bei dem invaliden Speicherzugriff terminiert und so keine Daten in den Cache kodiert werden. Eine andere Möglichkeit ist, dass der Prozessorfehler, auf dem RIDL basiert, während einer Transient Execution nicht auftritt, also keine Daten aus dem Line-Fill Buffer geladen werden.

Auftretende Exceptions mittels eines Signal Handlers zu behandeln resultiert in einer verringerten Erfolgsrate (von $97{,}96\,\%$ auf $74{,}69\,\%$) und erfordert mehr Zyklen für Angriff und Kodierung ($1\,229$ ggü. $1\,789$). Dies wird durch den Aufruf des Signal Handlers verursacht, der zusätzliche Zeit benötigt und dadurch potenzielle Interferenz durch das Betriebssystem oder dritte Prozesse erhöht. Andererseits erreicht die Vermeidung von Exceptions durch ausgelagerte Pages eine geringere Datenrate als die Behandlung durch Signal Handler ($443{,}9$ ggü. $745{,}5$ B/s), da für erstere die betreffende Page vor jedem Angriff erneut ausgelagert werden muss. Die benötigte Zeit der Auslagerung überwiegt dabei die des Signal Handlers.

Die Verwendung von Flush+Flush als Cache-basierten Seitenkanalangriff reduziert die Erfolgsrate wesentlich, von $97{,}96\,\%$ auf $2{,}515\,\%$. Zusätzlich reduziert Flush+Flush die Dauer des Dekodierens ungefähr auf die Hälfte, von $94\,200$ auf $48\,750$ Zyklen. Beides lässt sich mit den Ergebnissen aus [@sec:eval-sidechannel] vereinbaren.

<!-- - vergleich mit anderen arbeiten
  - ridl table 1: without TSX 100-1000 B/s cross-process covert channel
  - hier: ~400 B/s cross-process übertragung bei 98% erfolgsrate
  - erreichte übertragungsrate ist also vergleichbar mit der aus dem ridl paper -->

In der Literatur werden durch RIDL Daten zwischen Prozessen übertragen mit Datenraten von $100$ bis $1\,000$ B/s [@ridl, Tab. 1]. In diesen Bereich fällt auch die hier beobachtete Datenrate von $443{,}9$ B/s bei $97{,}96\,\%$ Erfolgsrate. Die beobachteten Ergebnisse sind in diesem Punkt also mit denen aus der Literatur vereinbar.

### Write Transient Forwarding {#sec:eval-wtf}

<!-- - basiskonfiguration von wtf:
  - opfer prozess führt schreibenden speicherzugriff durch
  - als cache-basierter seitenkanalangriff wird flush+reload verwendet
  - die auftretende exception wird durch einen signal handler behandelt (siehe sec:impl-teile-exception)
  - der initiale speicherzugriff erfolgt auf eine nicht-kanonische adresse (siehe sec:impl-angriffe-wtf)
- zusätzliche varianten, die jeweils einzelne aspekte ändern:
  - flush+flush: als cache-basierter seitenkanalangriff wird flush+flush verwendet
  - kernel: der initiale speicherzugriff erfolgt auf eine adresse des betriebssystems (siehe sec:impl-angriffe-wtf)
  - transient: auftretende exceptions werden durch transient execution vermieden (siehe sec:impl-teile-exception)
  - opfer prozess wird nicht variiert, da wtf daten aus dem store buffer extrahiert und daher nur schreibende zugriffe beobachtet werden können -->

In der Basiskonfiguration von Write Transient Forwarding (siehe auch [@sec:impl-angriffe-wtf]) führt der Opfer-Prozess schreibende Speicherzugriffe durch. Als Cache-basierter Seitenkanalangriff wird Flush+Reload verwendet. Auftretende Exceptions werden mithilfe eines Signal Handlers behandelt (siehe [@sec:impl-teile-exception]). Der initiale Speicherzugriff erfolgt auf eine nicht-kanonische Adresse.

Zusätzlich zu dieser Basiskonfiguration werden verschiedene Varianten betrachtet, die jeweils in einzelnen Aspekten abweichen:

- **Flush+Flush**: Als Cache-basierter Seitenkanalangriff wird Flush+Flush verwendet.

- **Kernel**: Der initiale Speicherzugriff erfolgt auf eine Adresse des Betriebssystem-Kerns (siehe [@sec:impl-angriffe-wtf]).

- **Transient**: Auftretende Exceptions werden durch Transient Execution unterdrückt (siehe [@sec:impl-teile-exception]).

Der Opfer-Prozess wird nicht variiert, da Write Transient Forwarding Daten aus dem Store Buffer extrahiert und daher nur schreibende Zugriffe beobachtet werden können.

<!-- - varianten:
  - default: target noncanon, fault signal, cache fr, victim store
  - target kernel, fault transient, cache ff
  - victim nicht variiert, da wtf aus store buffer leakt -> victim load kann nicht funktionieren -->

<!-- | Kernel      | $93.29 \pm 9.506$ | $745.6 \pm 0.447$ | $1728 \pm 12.52$        | $94700 \pm 54.81$         | -->

<!-- | Basis       | $92{,}80 \pm 9{,}797$ | $745{,}6 \pm 0{,}482$ | $1\,731 \pm 11{,}45$  | $94\,690 \pm 60{,}35$ |
| Flush+Flush | $19{,}50 \pm 25{,}83$ | $1\,424 \pm 2{,}426$  | $1\,730 \pm 10{,}42$  | $48\,690 \pm 78{,}31$ |
| Kernel      | $0{,}000 \pm 0{,}000$ | $751{,}0 \pm 0{,}333$ | $1\,725 \pm 10{,}29$  | $94\,010 \pm 40{,}09$ |
| Transient   | $0{,}000 \pm 0{,}000$ | $756{,}8 \pm 0{,}485$ | $184{,}8 \pm 0{,}411$ | $94\,800 \pm 60{,}86$ | -->

<!-- - tabelle tbl:eval-wtf zeigt die ermittelten metriken der verschiedenen varianten
- metriken wurden in sec:eval-angriffe erklärt
- wurden ermittelt wie in sec:eval-angriffe beschrieben
- gemessene latenzen beinhalten den overhead der messung, siehe sec:impl-teile-latenz -->

[@tbl:eval-wtf] zeigt die ermittelten Metriken für alle betrachteten Varianten von Write Transient Forwarding. Die Bedeutung dieser Metriken und die Vorgehensweise ihrer Erfassung wurden bereits in [@sec:eval-angriffe] erläutert. Die gemessenen Latenzen beinhalten dabei auch die Zeit, die für die Messung selber benötigt wird (siehe [@sec:impl-teile-latenz]).

| Variante    | Erfolgsrate (%)       | Datenrate (B/s)       | Kodieren (Zyklen)     | Dekodieren (Zyklen)   |
| ----------- | --------------------- | --------------------- | --------------------- | --------------------- |
| Basis       | $0{,}000 \pm 0{,}000$ | $752{,}1 \pm 0{,}573$ | $1\,594 \pm 16{,}30$  | $93\,990 \pm 66{,}28$ |
| Flush+Flush | $0{,}300 \pm 0{,}555$ | $1\,428 \pm 2{,}766$  | $1\,589 \pm 15{,}92$  | $48\,710 \pm 87{,}06$ |
| Kernel      | $0{,}000 \pm 0{,}000$ | $750{,}4 \pm 0{,}554$ | $1\,726 \pm 12{,}21$  | $94\,070 \pm 65{,}21$ |
| Transient   | $0{,}000 \pm 0{,}000$ | $764{,}1 \pm 0{,}741$ | $269{,}7 \pm 1{,}999$ | $93\,810 \pm 91{,}53$ |

: Die ermittelten Metriken für jede WTF-Variante, angegeben als $\mu\pm\sigma$ auf 4 gültige Ziffern. {#tbl:eval-wtf}

<!-- - auftretende phänomene beschreiben
  - keine der varianten funktioniert
  - vllt behebt die mikrocode version den fehler auch ohne unterstützung vom betriebssystem
  - vllt ist der verwendete prozessor nicht anfällig gegen wtf
  - geringe erfolgsrate bei f+f wahrscheinlich ausgelöst durch falsch-positive erkennung des übertragenen byte -->

Wie in [@tbl:eval-wtf] zu sehen ist, funktioniert keine der untersuchten Varianten zuverlässig. Die meisten Varianten extrahierten in keinem der beobachteten Fälle Daten erfolgreich. Einzig die Variante, die Flush+Flush als Cache-basierten Seitenkanalangriff verwendet, weist eine Erfolgsrate von $0{,}3\,\%$ auf. Aufgrund der im Vergleich zu Flush+Reload niedrigen Spezifität von Flush+Flush (siehe [@sec:eval-sidechannel]) wird diese jedoch wahrscheinlich durch eine falsch-positive Erkennung des übertragenen Bytes ausgelöst. Eine mögliche Erklärung für das Fehlschlagen dieses Angriffs ist, dass der verwendete Prozessor (siehe [@sec:eval-system]) nicht anfällig gegen Write Transient Forwarding ist. Eine andere Erklärung ist, dass die verwendete Version des Mikrocodes auch ohne eine Unterstützung des Betriebssystems den Prozessorfehler behebt, auf dem Write Transient Forwarding basiert.

<!-- - vergleich mit anderen arbeiten -->

Die Ergebnisse der Literatur [@fallout] können für Write Transient Forwarding in dieser Arbeit folglich nicht reproduziert werden.

### ZombieLoad {#sec:eval-zombieload}

<!-- - basiskonfiguration von zombieload:
  - opfer prozess führt schreibenden speicherzugriff durch
  - als cache-basierter seitenkanalangriff wird flush+reload verwendet
  - die auftretende exception wird durch einen signal handler behandelt (siehe sec:impl-teile-exception)
- zusätzliche varianten, die jeweils einzelne aspekte ändern:
  - flush+flush: als cache-basierter seitenkanalangriff wird flush+flush verwendet
  - load: der opfer prozess führt lesende speicherzugriffe durch
  - transient: auftretende exceptions werden durch transient execution vermieden (siehe sec:impl-teile-exception) -->

In der Basiskonfiguration von ZombieLoad (siehe auch [@sec:impl-angriffe-zombieload]) führt der Opfer-Prozess schreibende Speicherzugriffe durch. Als Cache-basierter Seitenkanalangriff wird Flush+Reload verwendet. Auftretende Exceptions werden mithilfe eines Signal Handlers behandelt (siehe [@sec:impl-teile-exception]).

Zusätzlich zu dieser Basiskonfiguration werden verschiedene Varianten betrachtet, die jeweils in einzelnen Aspekten abweichen:

- **Flush+Flush**: Als Cache-basierter Seitenkanalangriff wird Flush+Flush verwendet.

- **Load**: Der Opfer-Prozess führt lesende Speicherzugriffe durch.

- **Transient**: Auftretende Exceptions werden durch Transient Execution unterdrückt (siehe [@sec:impl-teile-exception]).

<!-- - varianten:
  - default: fault signal, cache fr, victim store
  - fault transient
  - cache ff
  - victim load -->

<!-- - tabelle tbl:eval-zombieload zeigt die ermittelten metriken der verschiedenen varianten
- metriken wurden in sec:eval-angriffe erklärt
- wurden ermittelt wie in sec:eval-angriffe beschrieben
- gemessene latenzen beinhalten den overhead der messung, siehe sec:impl-teile-latenz -->

[@tbl:eval-zombieload] zeigt die ermittelten Metriken für alle betrachteten Varianten von ZombieLoad. Die Bedeutung dieser Metriken und die Vorgehensweise ihrer Erfassung wurden bereits in [@sec:eval-angriffe] erläutert. Die gemessenen Latenzen beinhalten dabei auch die Zeit, die für die Messung selber benötigt wird (siehe [@sec:impl-teile-latenz]).

| Variante    | Erfolgsrate (%)       | Datenrate (B/s)       | Kodieren (Zyklen)     | Dekodieren (Zyklen)   |
| ----------- | --------------------- | --------------------- | --------------------- | --------------------- |
| Basis       | $94{,}31 \pm 23{,}14$ | $747{,}1 \pm 0{,}542$ | $1\,722 \pm 15{,}45$  | $94\,510 \pm 66{,}87$ |
| Flush+Flush | $67{,}51 \pm 43{,}55$ | $1\,424 \pm 2{,}597$  | $1\,720 \pm 15{,}63$  | $48\,690 \pm 83{,}72$ |
| Load        | $93{,}25 \pm 24{,}98$ | $768{,}2 \pm 1{,}413$ | $1\,636 \pm 19{,}49$  | $91\,950 \pm 169{,}6$ |
| Transient   | $99{,}70 \pm 5{,}472$ | $780{,}3 \pm 1{,}366$ | $263{,}1 \pm 5{,}020$ | $91\,870 \pm 161{,}4$ |

: Die ermittelten Metriken für jede ZombieLoad-Variante, angegeben als $\mu\pm\sigma$ auf 4 gültige Ziffern. {#tbl:eval-zombieload}

<!-- - auftretende phänomene beschreiben
  - basiskonfiguration funktioniert zuverlässig
  - stores leaken besser als loads aus dem lfb (wie bei ridl, aber nicht so stark)
  - transient funktioniert sehr gut (anders als bei den anderen)
    - reduziert interferenz durch betriebssystem
  - effekt von wechsel auf ff ist weniger stark als bei ridl
    - wahrscheinlich dadurch ausgelöst, dass der angriff insgesamt besser funktioniert -->

Wie in [@tbl:eval-zombieload] zu sehen, funktioniert die Basiskonfiguration von ZombieLoad zuverlässig mit einer Erfolgsrate von $94{,}31\,\%$. Die Daten lesender Speicherzugriffe lassen sich mit einer Erfolgsrate von $93{,}25\,\%$ nur unwesentlich weniger zuverlässig extrahieren als die Daten schreibender Speicherzugriffe. Der gleiche Effekt wurde bei RIDL beobachtet (siehe [@sec:eval-ridl]), ist dort aber wesentlich stärker ausgeprägt.

Ebenfalls im Gegensatz zu den RIDL-Varianten funktioniert bei ZombieLoad die Variante, die Exceptions durch Transient Execution unterdrückt, sehr zuverlässig. Die Erfolgsrate dieser Variante liegt hier bei $99{,}70\,\%$ und damit über der Erfolgsrate der Basiskonfiguration. Außerdem benötigt die Transient-Execution-Variante wesentlich weniger Zyklen für den Angriff und die Kodierung in den Cache ($263{,}1$ ggü. $1\,722$), wodurch sich die Datenrate leicht erhöht[^datenrate-nur-leicht]. Beides lässt sich durch die Abwesenheit einer architekturell auftretenden Prozessor-Exception erklären. Das Betriebssystem muss die Exception nicht behandeln, wodurch der Ablauf beschleunigt und potenzielle Quellen der Interferenz reduziert werden.

Die Verwendung von Flush+Flush als Cache-basierten Seitenkanalangriff reduziert die Erfolgsrate von $94{,}31\,\%$ auf $67{,}51\,\%$. Zusätzlich reduziert Flush+Flush die Dauer des Dekodierens ungefähr auf die Hälfte, von $94\,510$ auf $48\,690$ Zyklen. Da die Dauer des Dekodierens in beiden Fällen die Gesamtdauer dominiert, erhöht sich dadurch die Datenrate ungefähr auf das Doppelte, von $747{,}1$ auf $1\,424$ B/s. Beide Effekte stimmen mit den Ergebnissen aus [@sec:eval-sidechannel] überein.

[^datenrate-nur-leicht]: Da in beiden Fällen die Dauer des Dekodierens dominiert, erhöht auch eine wesentliche Reduktion der Dauer des Kodierens die Datenrate nur leicht.

<!-- - vergleich mit anderen arbeiten
  - 3.2: load data from uncacheable page -> 5.91 B/s (std_err_of_mean=0.18, n=100)
    - wir sind wesentlich schneller, laden aber von cacheable pages
    - nicht wirklich vergleichbar
  - 5.4: variant 1 with TSX: average transmission rate of 5.30 kB/s (std_err_of_mean=0.076, n=1000) and a true positive rate of 85.74 % (std_err_of_mean=0.0046, n=1000), ein sample ist ein angriff über 10s
    - wir erreichen 780 B/s mit 99.7% erfolgsrate mit transient execution (overhead sollte vergleichbar mit tsx sein)
    - der hier implementierte angriff ist um eine größenordnung langsamer, aber deutlich zuverlässiger
    - angriff hier wurde nicht auf übertragungsgeschwindigkeit optimiert
  - 5.4: bei variant 3 macht TSX faktor 100 in der transmission rate aus
    - nicht wirklich relevant, einfach weglassen -->

In der Literatur werden durch ZombieLoad Daten zwischen Prozessen übertragen mit einer Datenrate von $5{,}30$ kB/s und einer Richtig-positiv-Rate von $85{,}74\,\%$, unter Verwendung von TSX [@zombieload, Kap. 5.4]. Die hier implementierte Transient-Execution-Variante von ZombieLoad erreicht eine Datenrate von $780{,}3$ B/s bei einer Erfolgsrate von $99{,}70\,\%$. Diese ist also um eine Größenordnung langsamer, aber wesentlich zuverlässiger als der Angriff aus der Literatur. Für diese Unterschiede gibt es zwei mögliche Erklärungen. Einerseits können sie ausgelöst werden durch den Unterschied zwischen TSX und Transient Execution. Andererseits wurden die in dieser Arbeit implementierten Angriffe nicht auf ihre Datenrate optimiert und die verwendete Hardware ist unterschiedlich leistungsstark.

Insgesamt stimmen die hier erlangten Ergebnisse also im Wesentlichen mit denen aus der Literatur überein.

### Übergreifende Phänomene {#sec:eval-uebergreifend}

<!-- - auftretende phänomene beschreiben
  - wtf funktioniert gar nicht
  - default von ridl und zombieload funktionieren
  - dauer wird immer durch dekodierung dominiert
  - sidechannel:
    - fr ist immer zuverlässig und langsam
    - ff ist weniger zuverlässig und doppelt so schnell
    - deckt sich mit beobachtungen aus sec:eval-sidechannel
  - transient sehr viel zuverlässiger und schneller, funktioniert aber nur bei zombieload
    - zuverlässiger und schneller durch verringerung der interferenz durch betriebssystem
  - der funktionierende transient angriff braucht ~1.5x so lang wie der nicht funktionierende bei ridl
    - vllt wird die speculative execution in bei ridl gar nicht ausgelöst
  - dauer kodieren mit signal handler immer ungefähr gleich groß, kein unterschied zwischen angriffen -->

Bei einem Vergleich der Ergebnisse zwischen den untersuchten Angriffen fällt auf, dass Flush+Reload als Cache-basierter Seitenkanalangriff immer zuverlässiger als Flush+Flush ist. Andererseits benötigt Flush+Flush nur rund halb so viele Zyklen zum Dekodieren wie Flush+Reload. Dies deckt sich mit den Beobachtungen aus [@sec:eval-sidechannel]. Auch die konkrete Zahl der zum Dekodieren benötigten Zyklen stimmt über verschiedene Angriffe hinweg größtenteils überein, sowohl für Flush+Reload als auch für Flush+Flush. Außerdem wird die Gesamtdauer des Angriffs stets durch die Dauer der Dekodierung dominiert. Eine Beschleunigung der Dekodierung führt damit auch zu einer Verbesserung der erreichten Datenrate.

Insgesamt konnten einige Ergebnisse der Paper, die RIDL [@ridl] und ZombieLoad [@zombieload] beschreiben, in dieser Arbeit reproduziert werden. Write Transient Forwarding [@fallout, Kap. 3.1] hingegen konnte in der hier angefertigten Implementierung keine Daten erfolgreich extrahieren.

## Store-to-Leak {#sec:eval-stl}

<!-- - stl separat von den anderen angriffen betrachtet, da er keine daten direkt aus puffern der mikroarchitektur extrahiert
- store-to-leak wird benutzt um die anwesenheit von Speicherzuordnungen im adressbereich des kernel festzustellen
- kann dadurch benutzt werden um die basisadresse des kernels zu bestimmen, womit kaslr umgangen/gebrochen wird -->

Anders als die drei in [@sec:eval-angriffe] untersuchten Angriffe extrahiert Store-to-Leak keine Daten direkt aus Puffern der Mikroarchitektur. Daher wird Store-to-Leak separat in diesem Kapitel betrachtet. Wie in [@sec:impl-angriffe-stl] dargestellt, kann Store-to-Leak verwendet werden, um die Anwesenheit von Speicherzuordnungen im Adressbereich des Betriebssystem-Kerns festzustellen. Auf diese Weise kann mittels Store-to-Leak die Basisadresse des Betriebssystem-Kerns bestimmt und damit KASLR umgangen werden.

<!-- - store-to-leak: gibt keinen victim prozess
  - erkenne unterschied zwischen kernel mapping und kein mapping
  - kann benutzt werden um kaslr zu brechen -->

<!-- - vorgehensweise
  - wie in sec:impl-angriffe-stl beschrieben
  - um zu ermitteln ob speicherzuordnung für anvisierte virtuelle adresse existiert, wird fetch+bounce mehrmals ausgeführt (8x) und geprüft wie oft die page als in den cache geladen erkannt wurde. wenn das einen festen threshold (2x) überschreitet, wird speicherzuordnung als vorhanden angenommen
  - dies wird an einer vorhandenen und einer nicht vorhandenen speicherzuordnung jeweils mehrmals (10000x) getestet
    - erhobene metriken werden über diese wiederholungen gemittelt, um einen messpunkt zu erhalten
  - es werden nacheinander 1000 dieser messpunkte ermittelt, um zeitlich separierte messergebnisse zu erhalten
    - mittelwert und standardabweichung werden über diese gebildet -->

Zum Zweck der Evaluation wird Store-to-Leak wie in [@sec:impl-angriffe-stl] beschrieben ausgeführt. Um zu ermitteln, ob eine Speicherzuordnung für eine anvisierte virtuelle Adresse existiert, werden acht Store-to-Leak Iterationen ausgeführt. Dabei wird erfasst, wie oft Store-to-Leak einen validen TLB Eintrag für diese Adresse detektiert. Erreicht diese Zahl einen festen Threshold von $2$, so wird die Speicherzuordnung als vorhanden angenommen. Auf diese Art und Weise werden eine vorhandene und eine nicht vorhandene Speicherzuordnung jeweils $10\,000$-mal klassifiziert. Die erhobenen Metriken werden über diese Klassifikationen gemittelt, um einen einzelnen Messpunkt zu erhalten. Dies wird wiederholt, bis nacheinander $1\,000$ zeitlich separierte Messpunkte ermittelt wurden. Aus diesen Messpunkten werden anschließend die Mittelwerte und Standardabweichungen der Metriken berechnet.

<!-- - ermittelte metriken
  - fehlerrate und zur klassifikation benötigte zeit
  - getrennt sowohl für vorhandene als auch für nicht vorhandene speicherzuordnungen
- daraus berechnet wird sensitivität und spezifität mit der die anwesenheit einer speicherzuordnung bestimmt wird
  - wie in [@sec:eval-sidechannel] beschrieben -->

Dabei werden die Fehlerrate der Klassifikation und die zur Klassifikation benötigte Zeit erfasst. Diese Messungen werden getrennt für den Fall der vorhandenen und der nicht vorhandenen Speicherzuordnung. Aus allen erfassten Messungen werden anschließend die Sensitivität und Spezifität der Klassifikation berechnet. Die Bedeutung und Berechnung dieser Metriken wurde bereits in [@sec:eval-sidechannel] erläutert.

<!-- - wie bei den anderen angriffen werden verschiedene varianten evaluiert
- aspekte die variiert werden können:
  - verwendeter cache-basierter seitenkanalangriff; basiskonfiguration verwendet f+r
  - technik um mit entstehenden exceptions umzugehen; basiskonfiguration verwendet signal handling
- zusätzliche varianten:
  - flush+flush: verwendet f+f als cache-basierten seitenkanalangriff
  - transient: verwendet transient execution zur vermeidung der entstehenden exceptions -->

Wie die in [@sec:eval-angriffe] betrachteten Angriffe, wird auch Store-to-Leak anhand verschiedener Varianten evaluiert. Dabei kann einerseits der verwendete Cache-basierte Seitenkanalangriff variiert werden, sowie andererseits die Technik zur Vermeidung von Exceptions. Die Basiskonfiguration von Store-to-Leak verwendet Flush+Reload als Cache-basierten Seitenkanalangriff und behandelt Exceptions durch einen Signal Handler. Zusätzlich werden folgende Varianten untersucht:

- **Flush+Flush**: Als Cache-basierter Seitenkanalangriff wird Flush+Flush verwendet.

- **Transient**: Auftretende Exceptions werden durch Transient Execution unterdrückt (siehe [@sec:impl-teile-exception]).

| Variante    | Sensitivität (%)      | Spezifität (%)            | Mapped (µs)             | Unmapped (µs)            |
| ----------- | --------------------- | ------------------------- | ----------------------- | ------------------------ |
| Basis       | $69{,}22 \pm 7{,}399$ | $100{,}0 \pm 0{,}01168$   | $8{,}752 \pm 0{,}06659$ | $8{,}951 \pm 0{,}03694$  |
| Flush+Flush | $2{,}185 \pm 4{,}657$ | $99{,}19 \pm 3{,}017$     | $8{,}126 \pm 0{,}04591$ | $8{,}186 \pm 0{,}04292$  |
| Transient   | $99{,}89 \pm 2{,}355$ | $100{,}0 \pm 0{,}0003162$ | $1{,}673 \pm 0{,}01000$ | $1{,}799 \pm 0{,}008132$ |

: Die ermittelten Metriken für jede Store-to-Leak-Variante, angegeben als $\mu\pm\sigma$ auf 4 gültige Ziffern. {#tbl:eval-stl}

<!-- - tabelle zeigt die ermittelten metriken -->

<!-- - phänomene:
  - transient wesentlich besser und schneller als signal handler
    - kein rauschen im signal und kein zeitverlust durch fault handler des kernels
    - wie schon bei zombieload beobachtet
  - unmapped braucht bisschen mehr zeit als mapped
    - potenzielle erklärung: mapped hitted den tlb immer nach dem ersten versuch, braucht deswegen kürzer (unmapped trifft den tlb nie)
  - f+f wie erwartet ein bisschen schneller und wesentlich weniger zuverlässig als f+r
    - aber weniger als bei den anderen angriffen, da hier nur eine cachezeile beobachtet wird -->

[@tbl:eval-stl] zeigt die ermittelten Metriken für jede der untersuchten Varianten von Store-to-Leak. *Mapped* und *Unmapped* bezeichnen dabei die zur Klassifikation einer vorhandenen und einer nicht vorhandenen Speicherzuordnung benötigte Zeit. Dabei ist zu sehen, dass die Unterdrückung von Exceptions durch Transient Execution zu einer wesentlich zuverlässigeren und wesentlich schnelleren Klassifikation führt. Dies ist durch die Abwesenheit einer architekturell auftretenden Prozessor-Exception zu erklären. Das Betriebssystem muss die Exception nicht behandeln, wodurch der Ablauf beschleunigt und potenzielle Quellen der Interferenz reduziert werden. Der gleiche Effekt wurde bereits bei ZombieLoad in [@sec:eval-zombieload] beobachtet.

Außerdem erfolgt die Klassifikation von vorhandenen Speicherzuordnungen etwas schneller als die Klassifikation nicht vorhandener Zuordnungen. Dies lässt sich dadurch erklären, dass bei vorhandenen Zuordnungen die wiederholten Zugriffe durch den TLB beschleunigt werden. Im Falle einer nicht vorhandenen Zuordnung wird hingegen kein Eintrag im TLB erstellt.

Des Weiteren wird die Klassifizierung durch die Verwendung von Flush+Flush wesentlich weniger zuverlässig und etwas schneller. Dies stimmt mit den Ergebnissen aus [@sec:eval-sidechannel] und ähnlichen Beobachtungen bei den in [@sec:eval-angriffe] betrachteten Angriffen überein. Die Auswirkung auf die Ausführungszeit ist bei Store-to-Leak wesentlich geringer als bei den in [@sec:eval-angriffe] betrachteten Angriffen, da Store-to-Leak nur eine einzelne Cachezeile beobachtet.

<!-- - kernel base lässt sich im schnitt in 0,5ms ermitteln, dadurch kaslr brechen
  - kaslr entropie 9 bit, wie in sec:grundlagen-speicher-adressbereich beschrieben
    - fußnote: kernel start 0xffffffff80000000, end 0xffffffffc0000000, align 0x200000 / 21 bit / 2 MB
  - 512 mögliche basisadressen des kernels, 256 müssen im schnitt geprüft werden
  - 256 prüfungen brauchen $256 \cdot 1{,}799 = 460{,}544$ mikrosekunden, ca. 0,5ms -->

Wie in [@sec:grundlagen-speicher-adressbereich] beschrieben, besitzt die Basisadresse des Linux Kernels eine Entropie von $9$\ Bits[^kaslr-range]. Um diese Basisadresse zu bestimmen, müssen von den $512$ möglichen Adressen im Schnitt $256$ geprüft werden. Diese Klassifizierungen benötigen mit der Transient-Variante von Store-to-Leak ungefähr $256 \cdot 1{,}799 = 460{,}544$ Mikrosekunden. Store-to-Leak kann folglich genutzt werden, um in einer halben Millisekunde die Basisadresse des Linux Kernels zu bestimmen und dadurch KASLR zu umgehen.

[^kaslr-range]: Die Basisadresse des Linux Kernels wird zwischen `0xffffffff80000000` und `0xffffffffc0000000` gewählt, als ein Vielfaches von `0x200000`.

<!-- - vergleich mit anderen arbeiten
  - fallout 6.1: data bounce has f1-score of 0.9996
  - f1-score ist harmonisches mittel zwischen genauigkeit und sensitivität
  - genauigkeit: tp/(tp+fp)
  - für variante transient:
    - genauigkeit 1.000
    - f1-score 0.9994
  - es wird also ein vergleichbarer f1-score erreicht -->

In der Literatur wird für die mithilfe von Store-to-Leak durchgeführte Klassifikation ein *F1-Score* von $0{,}9996$ angegeben [@fallout, Kap. 6.1]. Der F1-Score ist das harmonische Mittel von der *Genauigkeit* und der Sensitivität der Klassifikation [@classification, Kap. 2.8]. Die Genauigkeit, auch *positiver Vorhersagewert*, ist der Anteil der als vorhanden klassifizierten Speicherzuordnungen, die tatsächlich vorhanden sind [@classification, Kap. 2.5]. Für die Transient-Variante von Store-to-Leak beträgt die Genauigkeit hier $1{,}000$ und der F1-Score $0{,}9994$. Die Ergebnisse der Literatur können hier also reproduziert werden.

<!-- - genauigkeit hier: basis 1.000, f+f 0.7299, transient 1.000
- f1-score hier: basis 0.8181, f+f 0.04243, transient 0.9994 -->

## Zusammenfassung

In [@sec:eval-angriffe] und [@sec:eval-stl] wurden RIDL, Write Transient Forwarding, ZombieLoad und Store-to-Leak in verschiedenen Varianten evaluiert. Bei dieser Evaluation zeigten drei von den vier betrachteten Angriffen die erwarteten Ergebnisse in den meisten ihrer Varianten. RIDL, ZombieLoad und Store-to-Leak extrahierten erfolgreich anvisierte Daten. Durch Write Transient Forwarding hingegen konnten in keiner der evaluierten Varianten die anvisierten Daten erfolgreich extrahiert werden.
