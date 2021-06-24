# Spectre-Angriffe und Details ihrer Implementierung {#sec:impl}

<!-- - Intro
  - Spectre-Angriffe aka Transient-Execution Attacks
  - subtype of Microarchitectural Attacks / nutzen fehler in der mikroarchitektur aus, um unberechtigt auf daten zuzugreifen
  - angriffe nutzen transient execution um auf geheime daten zuzugreifen und übertragen diese anschließend aus der transient execution in den architekturellen zustand
  - zur übertragung werden bei allen hier betrachteten angriffen seitenkanalangriffe auf den cache benutzt (siehe kapitel xxx) -->

<!-- Spectre-Angriffe nutzen Fehler in der Mikroarchitektur aus, um während einer Transient Execution unberechtigt auf Daten zuzugreifen. Diese Daten übertragen sie anschließend aus dem Kontext der Transient Execution in den architekturellen Zustand. Bei allen in dieser Arbeit betrachteten Angriffen werden für diese Übertragung Cache-basierte Seitenkanalangriffe benutzt (siehe [@sec:grundlagen-sidechannel]). Spectre-Angriffe werden auch als *Transient Execution Attacks* bezeichnet. [@transient-fail, Kap. 1] -->

Spectre-Angriffe nutzen Fehler in der Mikroarchitektur aus, um während einer Transient Execution unberechtigt auf Daten zuzugreifen. Diese Daten übertragen sie anschließend aus dem Kontext der Transient Execution in den architekturellen Zustand. Bei allen in dieser Arbeit betrachteten Angriffen werden für diese Übertragung *Cache-basierte Seitenkanalangriffe* benutzt. Zwei wichtige Seitenkanalangriffe dieser Art werden zunächst in [@sec:grundlagen-sidechannel] erklärt. Diese ermöglichen einem Angreifer, Zeitpunkt und Adresse von Speicherzugriffen zu ermitteln. Spectre-Angriffe werden auch als *Transient Execution Attacks* bezeichnet. [@transient-fail, Kap. 1]

<!-- - in diesem kapitel:
  - eigenschaften und vorgehensweise der angriffe allgemein
  - zwei grundlegende kategorisierungen der angriffe: basierend auf branch prediction oder auf prozessor-exceptions
  - Einzelne Teile der Angriffe und wie diese umgesetzt werden
  - konkrete prozessor-exception-basierte angriffe und stellenweise details der implementierung:
    - ridl, fallout, zombieload -->

Neben Cache-basierten Seitenkanalangriffen behandelt dieses Kapitel Spectre-Angriffe allgemein sowie ausgewählte konkrete Spectre-Angriffe. In [@sec:impl-allgemein] werden die Eigenschaften und die Funktionsweise von Spectre-Angriffen im Allgemeinen beschrieben. Spectre-Angriffe werden grundlegend unterteilt in Angriffe basierend auf Branch Prediction und Angriffe basierend auf Prozessor-Exceptions [@transient-fail, Kap. 2]. Die Unterschiede zwischen diesen Arten werden in [@sec:impl-spectre-prediction] und [@sec:impl-spectre-exception] detailliert dargestellt. In [@sec:impl-szenario] werden Angriffsszenarien für Spectre-Angriffe allgemein sowie die in dieser Arbeit betrachteten Szenarien beschrieben. Anschließend werden in [@sec:impl-teile] wiederverwendbare Elemente der Implementierung von Spectre-Angriffen thematisiert. Diese Elemente werden in den konkreten Spectre-Angriffen verwendet, die schließlich in [@sec:impl-angriffe] erklärt werden. Dabei handelt es sich um vier Angriffe basierend auf Prozessor-Exceptions. Es werden sowohl die Funktionsweise als auch Details der Implementierung dieser Angriffe beschrieben.

<!-- - um zu leaken aus line-fill buffer, store buffer, load ports und position von speicher mappings festzustellen
- ridl: leak from line-fill buffer, load ports
- fallout: leak from store buffer, leak position of memory mappings
- zombieload: leak from line-fill buffer -->

## Cache-basierte Seitenkanalangriffe {#sec:grundlagen-sidechannel}

<!-- - Cache-basierte Seitenkanalangriffe: Unterschiede bei Zahl der benötigten Taktzyklen bei Speicherzugriffen ausnutzen, um Informationen zu gewinnen
  - Grundlage: Speicher im Cache braucht deutlich weniger Zeit -> rausfinden, welcher Speicher im Cache ist
  - TLB- oder Nutzdaten-Cache
  - Grund der Caches führt direkt zu diesen Side Channels -->

*Cache-basierte Seitenkanalangriffe* nutzen durch einen Cache bedingte Unterschiede in der Zugriffszeit, um vertrauliche Informationen abzuleiten [@flush-reload, Kap. 3]. Grundlage dafür ist, dass die Latenz des Zugriffs kürzer ist, wenn die angefragten Daten bereits im Cache vorhanden sind. Auf diese Weise kann ein Angreifer herausfinden, ob von ihm angefragte Daten bereits im Cache vorhanden sind oder nicht. Da die Unterschiede in der Zugriffszeit direkt aus der allgemeinen Funktionsweise von Caches folgen (siehe [@sec:grundlagen-caches]), weisen alle Caches einen solchen Seitenkanal auf. Diese Angriffe können folglich sowohl auf Caches der Adressübersetzung [@sidechannel-tlb-1] [@sidechannel-tlb-2] als auch auf Caches für Nutzdaten angewandt werden. Im Folgenden werden zwei Cache-basierte Seitenkanalangriffe auf Caches für Nutzdaten beschrieben.

<!-- - alle in dieser arbeit betrachteten konkreten angriffe [@sec:impl-angriffe] nutzen ausschließlich cache-basierte seitenkanalangriffe auf den l1d cache -->

Alle konkreten Angriffe, die in dieser Arbeit betrachtet werden (siehe [@sec:impl-angriffe]), führen Cache-basierte Seitenkanalangriffe ausschließlich auf den L1d Cache aus. Siehe auch [@sec:impl-teile-cache] für eine genaue Betrachtung dieses Umstandes.

### Flush+Reload

<!-- - Flush+Reload:
  - Angreifer und Opfer haben gemeinsamen (physischen) Speicher und Cache
  - Angreifer flusht Cacheline regelmäßig ("Flush")
  - Angreifer lädt und misst Zeit ("Reload")
  - Kurze Zeit -> Cacheline wurde in der Zwischenzeit geladen
  - Genaue Latenzen hängen von Setup ab
  - False negative: cacheline wurde aus cache verdrängt
  - False positive: prefetching
  - Angreifer muss flushen können, geht auf x86 unprivilegiert
  - Angreifer muss Zeit messen können, geht auf x86 unprivilegiert -->

*Flush+Reload* ist ein Cache-basierter Seitenkanalangriff. Um diesen Angriff auf ein bestimmtes Opfer und eine bestimmte Cachezeile durchführen zu können, muss der Angreifer sowohl den Speicherbereich der Cachezeile als auch einen zugehörigen Cache mit dem Opfer gemeinsam haben. Dies ermöglicht dem Angreifer, einen Zugriff des Opfers auf die Cachezeile zu detektieren. [@flush-reload, Kap.\ 3]

![Zeitlicher Verlauf von Flush+Reload.](figs/flush_reload.pdf){#fig:grundlagen-flushreload}

Der Angriff besteht aus drei Phasen, die in [@fig:grundlagen-flushreload] illustriert werden. In der ersten, sogenannten *Flush* Phase invalidiert der Angreifer die gewählte Cachezeile. In der zweiten Phase wartet der Angreifer. Das Opfer wird in dieser Zeit entweder auf die Cachezeile zugreifen oder nicht. In der letzten Phase, genannt *Reload*, lädt der Angreifer die Cachezeile ein und misst die Latenz dieses Zugriffs. Misst der Angreifer eine kurze Latenz, war die Cachezeile bereits im Cache vorhanden. In diesem Fall hat das Opfer in der Zwischenzeit auf die Cachezeile zugegriffen. Misst der Angreifer jedoch eine große Latenz, war die Cachezeile nicht im Cache vorhanden und das Opfer hat in der Zwischenzeit nicht auf diese zugegriffen. Welche konkreten Latenzen der Angreifer als Cache Hit oder Cache Miss klassifiziert, hängt von dem angegriffenen System ab. Wird die Cachezeile nach Zugriff des Opfers von anderen Cachezeilen aus dem Cache verdrängt, so kommt es zu falsch-negativen Ergebnissen. Falsch-positive Ergebnisse treten auf, wenn der Prozessor unabhängig von tatsächlichem Zugriff die Cachezeile einlädt (sogenanntes *Prefetching*). [@flush-reload, Kap. 3]

Um den Flush+Reload Angriff durchzuführen, muss der Angreifer die gewählte Cachezeile invalidieren können. Auf x86 Prozessoren ist dies mit der `clflush` Instruktion möglich, die keine Privilegien benötigt [@intel-sdm-2, S. 3-145]. Außerdem muss der Angreifer in der Lage sein die Zugriffslatenz zu bestimmen. Auf x86 Prozessoren lässt sich dies mithilfe der `rdtsc` Instruktion umsetzen [@intel-sdm-2, Kap. 4, S. 547]. Diese erfordert ebenfalls keine Privilegien. [@flush-reload, Kap. 3]

<!-- - Flush+Reload:
  - Angreifer und Opfer haben geteilten (physischen) Speicher und Cache
  - Angreifer muss flushen können, geht auf x86 unprivilegiert
  - Angreifer flusht, Opfer greift zu oder nicht, Angreifer lädt und misst Zeit
  - Pause des Angreifers kann variiert werden. Lange Pause -> mehr Zeit in der Opfer beobachtet wird, aber höhere Wahrscheinlichkeit dass Cache evictet wird
- Flush+Reload:
  - Angreifer flusht Cacheline regelmäßig
  - Angreifer lädt und misst Zeit
  - Kurze Zeit -> Cacheline wurde in der Zwischenzeit geladen
  - Braucht shared memory -->

<!-- - Pause des Angreifers kann variiert werden. Lange Pause -> mehr Zeit in der Opfer beobachtet wird, aber höhere Wahrscheinlichkeit dass Cache evictet wird
- Kann wiederholt werden um Verhalten über Zeit zu beobachten
- Mehrere Cachelines können gleichzeitig beobachtet werden
- Insgesamt: Leakt Zugriffsmuster (innerhalb des shared memory) des Opfers mit Cacheline-Granularität über Zeit -->
<!-- Die Wartezeit des Angreifers in der zweiten Phase kann variiert werden. Eine längere Wartezeit ermöglichtDies erhöht die Wahrscheinlichkeit für falsch-negative Ergebnisse -->

Der Flush+Reload Angriff kann wiederholt ausgeführt werden, um Zugriffe des Opfers über einen längeren Zeitraum zu beobachten. Außerdem kann der Angriff gleichzeitig für unterschiedliche Cachezeilen ausgeführt werden, um Zugriffe auf einem größeren Speicherbereich zu beobachten. Insgesamt kann so das Zugriffsverhalten des Opfers über den gesamten geteilten Speicherbereich und über einen beliebigen Zeitraum ermittelt werden. Die Speicherzugriffe lassen sich dabei räumlich auf die Größe einer Cachezeile und zeitlich auf die Dauer eines einzelnen Flush+Reload Angriffs auflösen. [@flush-reload, Kap. 3]

<!-- - konkreter angriff aus paper
- angriff auf rsa implementierung
- privater schlüssel rekonstruiert aus beobachteten signierungen
- [@flush-reload, Kap. 4] -->

In der Praxis wurde Flush+Reload beispielsweise eingesetzt, um die RSA-Implementierung von GnuPG anzugreifen. Dabei wurde mittels Flush+Reload die Aktivität eines `gpg` Prozesses beobachtet, während dieser eine Signatur berechnet. Aus den dabei erhobenen Messungen wurde anschließend eine Komponente des verwendeten privaten Schlüssels rekonstruiert. [@flush-reload, Kap. 4]

### Flush+Flush

<!-- - flush+flush: variante von flush+reload
- basiert nicht auf zeitunterschied im zugriff, sondern auf zeitunterschied in der invalidierung der cachezeile
- im cache -> invalidierung muss cacheeintrag tatsächlich invalidieren, nicht im cache -> invalidierung kann früher beendet werden
- einordnung des ergebnisses andersrum als bei flush+reload
- rest gleich, gleiche drei phasen

- flush löst kein prefetching aus -> nur noch opfer kann prefechting auslösen, weniger false positives
- bei wiederholter ausführung des f+f angriffes kann die erste phase weggelassen werden, da die dritte phase den cache schon invalidiert -->

Der *Flush+Flush* Angriff ist eine Variante von Flush+Reload. Er basiert nicht auf Unterschieden in der Latenz von Speicherzugriffen, sondern auf Zeitunterschieden bei der Invalidierung einer Cachezeile. Befindet sich eine Cachezeile im Cache, so muss die Invalidierung des Cacheeintrages tatsächlich erfolgen. Befindet sich die Cachezeile hingegen nicht im Cache, so kann die Invalidierung vorzeitig beendet werden. Aufgrund dieses Unterschiedes weist die Invalidierung einer Cachezeile eine größere Latenz auf, wenn diese im Cache vorhanden ist. Die Einordnung der Messergebnisse ist damit umgekehrt zu der im Flush+Reload Angriff: Eine lange Latenz signalisiert einen Zugriff des Opfers. Der Flush+Flush Angriff besteht aus den gleichen drei Phasen wie der Flush+Reload Angriff. Der einzige Unterschied liegt entsprechend in der dritten Phase, in der der Angreifer die anvisierte Cachezeile invalidiert, anstatt sie einzuladen. [@flush-flush, Kap. 3]

Die Invalidierung einer Cachezeile löst kein Prefetching aus [@flush-flush, Kap. 3]. Daher kann Prefetching beim Flush+Flush Angriff nicht mehr durch den Angreifer, sondern nur noch durch das Opfer ausgelöst werden. Dies reduziert die Zahl von falsch-positiven Ergebnissen. Bei wiederholter Ausführung des Flush+Flush Angriffs kann außerdem die erste Phase übersprungen werden, da die dritte Phase bereits für eine Invalidierung der Cachezeile sorgt. Dies wird in [@fig:grundlagen-flushflush] illustriert.

![Zeitlicher Verlauf von Flush+Flush.](figs/flush_flush.pdf){#fig:grundlagen-flushflush}

<!-- - konkreter angriff aus paper
- zeitpunkte von tastendrücken/tastaturanschlägen bestimmen [@flush-flush, Kap. 6]
- aes schlüssel rekonstruieren aus beobachteten verschlüsselungen, mit bekanntem plaintext [@flush-flush, Kap. 7] -->

In der Praxis wurde Flush+Flush beispielsweise eingesetzt, um eine AES-Implementierung von OpenSSL anzugreifen. Dabei wurde mittels Flush+Flush die Aktivität eines Prozesses beobachtet, während dieser wiederholt einen bekannten Klartext verschlüsselt. Aus den dabei erhobenen Messungen wurde anschließend ein Teil des verwendeten Schlüssels rekonstruiert. [@flush-flush, Kap.\ 7]

## Allgemeine Funktionsweise von Spectre-Angriffen {#sec:impl-allgemein}

<!-- - grobe vorgehensweise der angriffe stimmt überein (habil 43--45):
  - abbildung hierzu, einfach die aus habil kopieren?
  - 1: prepare microarchitecture and covert channel / vorbereitung der mikroarchitektur (transient execution muss auf anvisierte daten zugreifen können und lang genug sein um diese zu übertragen) und des übertragungsweges (zwischen transient und normaler execution), immer im kontext des angreifers
  - 2: enter transient execution / eintritt in transient execution, kann im kontext des angreifers oder des opfers geschehen
  - 3: trigger leak / führe transient instructions aus, die auf anvisierte daten zugreifen und diese auf übertragung vorbereiten, weiterhin im gleichen kontext wie phase 2
  - 4: encode in covert channel / übertrage daten sodass diese nach der transient execution empfangen werden können, weiterhin im gleichen kontext wie phase 2 und 3
  - 5: exit transient execution / transient execution endet, ergebnisse der execution werden verworfen, übertragungsweg wurde so gewählt dass daten in ihm jedoch erhalten bleiben
  - 6: decode covert channel / empfange die in phase 4 übertragenen daten durch den gewählten übertragungsweg, immer im kontext des angreifers -->

![Methodik von Spectre-Angriffen, bestehend aus 6 Phasen. [@gruss-habil, Abb. 3.1]](figs/spectre.pdf){#fig:impl-spectre}

Die Methodik von Spectre-Angriffen stimmt bei allen Angriffen überein. [@fig:impl-spectre] illustriert die 6 Phasen, in die sich ein solcher Angriff unterteilen lässt:

- In **Phase 1** werden die Mikroarchitektur und der Übertragungskanal vorbereitet. Es wird dafür gesorgt, dass die später ausgelöste Transient Execution auf die anvisierten Daten zugreifen kann. Außerdem wird sichergestellt, dass die Transient Execution lange genug anhält, um diese Daten in den Übertragungskanal zu kodieren. Diese Phase wird immer in dem Kontext des Angreifers ausgeführt. [@gruss-habil, S. 43--45]

- **Phase 2** bezeichnet den Eintritt in die Transient Execution. Wie in [@sec:grundlagen-mikroarch-transient] erläutert, kann diese durch eine falsche Branch Prediction oder durch eine Prozessor-Exception ausgelöst werden. Des Weiteren kann die Transient Execution sowohl im Kontext des Angreifers als auch im Kontext des Opfers erfolgen. Beides ist abhängig von der Art des Spectre-Angriffs und wird in [@sec:impl-spectre-prediction] und [@sec:impl-spectre-exception] differenziert. [@gruss-habil, S.\ 43--45]

- In **Phase 3** werden Transient Instructions ausgeführt, die auf die anvisierten Daten zugreifen. Anschließend werden die Daten auf Kodierung in den Übertragungskanal vorbereitet. Dies findet in dem gleichen Kontext wie Phase 2 statt. [@gruss-habil, S. 43--45]

- **Phase 4** findet weiterhin innerhalb der Transient Execution statt. In dieser Phase werden die Daten in den Übertragungskanal kodiert, sodass diese nach Abschluss der Transient Execution extrahiert werden können. Dies findet in dem gleichen Kontext wie die Phasen 2 und 3 statt. [@gruss-habil, S. 43--45]

- In **Phase 5** wird die Transient Execution beendet und damit die Ergebnisse aller Transient Instructions verworfen. Der Übertragungskanal wurde jedoch so gewählt, dass die in ihm kodierten Daten erhalten bleiben und anschließend dekodiert werden können. [@gruss-habil, S.\ 43--45]

- In **Phase 6** wird diese Dekodierung durchgeführt: Die im Übertragungskanal enthaltenen Daten werden extrahiert. Dadurch werden diese Daten von der Domäne der Transient Execution in den architekturellen Zustand übertragen. Diese Phase wird immer in dem Kontext des Angreifers ausgeführt. [@gruss-habil, S. 43--45]

<!-- - covert channel ist üblicherweise der cache, z.b. bei allen hier betrachteten angriffen
- kodierung in zustand des caches, also welche cachezeilen im cache enthalten sind
- dieser zustand kann per cache side-channel angriff beobachtet werden, dadurch werden die daten dann extrahiert, siehe kapitel grundlagen-sidechannel
- in phase 1 werden also die beobachteten cachezeilen invalidiert (siehe grundlagen-sidechannel)
- in phase 6 wird also auf die beobachteten cachezeilen zugegriffen (entweder load oder flush) und die latenz des zugriffes gemessen (siehe grundlagen-sidechannel)
- (bei Flush+Reload über Einladen ihres Inhalts, bei Flush+Flush durch erneute Invalidierung, siehe [@sec:grundlagen-sidechannel]) -->

Üblicherweise wird ein Cache-basierter Übertragungskanal verwendet. Dies ist insbesondere bei allen in [@sec:impl-angriffe] betrachteten Angriffen der Fall. Bei diesem Übertragungskanal werden die Daten in den Zustand des Caches kodiert. Dabei bezeichnet der Zustand des Caches, welche Cachezeilen im Cache enthalten sind. Dieser Zustand kann mit einem Cache-basierten Seitenkanalangriff (siehe [@sec:grundlagen-sidechannel]) beobachtet werden. Auf diese Weise werden die kodierten Daten extrahiert. Die Vorbereitung eines Cache-basierten Übertragungskanals in Phase 1 des Spectre-Angriffs besteht darin, alle beobachteten Cachezeilen zu invalidieren. Dies entspricht der ersten Phase des Cache-basierten Seitenkanalangriffs. In Phase 6 des Spectre-Angriffs wird auf die beobachteten Cachezeilen zugegriffen und die Latenz dieses Zugriffs gemessen. Dies entspricht der dritten Phase des Cache-basierten Seitenkanalangriffs. [@gruss-habil, S. 43--45]

<!-- - Unterteilung in zwei Bereiche: basierend auf branch prediction oder auf prozessor-exceptions
- die beiden arten werden im folgenden detailliert beschrieben -->

Spectre-Angriffe werden unterteilt in zwei verschiedene Arten: Angriffe basierend auf Branch Prediction und Angriffe basierend auf Prozessor-Exceptions [@transient-fail, Kap. 2]. Diese beiden Arten werden im Folgenden detailliert beschrieben.

## Spectre-Angriffe basierend auf Branch Prediction {#sec:impl-spectre-prediction}

<!-- - auch Spectre-type genannt, nach ersten Spectre Attacken
- transient execution ab phase 2 wird durch falsche branch prediction ausgelöst, siehe sec:grundlagen-mikroarch-transient
- phasen 2--5 werden im kontext des opfers ausgeführt -->

Spectre-Angriffe basierend auf Branch Prediction werden auch *Spectre-type Attacks* genannt nach dem Paper, das die ersten Angriffe dieser Art beschrieben hat [@spectre]. Bei Angriffen basierend auf Branch Prediction wird die Transient Execution ab Phase 2 durch eine falsche Branch Prediction ausgelöst (siehe [@sec:grundlagen-mikroarch-transient]). Außerdem werden die Phasen 2 bis 5 im Kontext des Opfers ausgeführt. [@transient-fail, Kap. 2]

<!-- - genereller ablauf der angriffe:
  - training des branch predictors, damit ausgang des anvisierten bedingten sprunges falsch vorhergesagt wird
  - training heißt: branch oft ausführen mit dem gewünschten ergebnis, damit dieses während des angriffes vorhergesagt wird
  - beim angriff dann dafür sorgen dass ein anderes ergebnis auftritt
  - induziert also inkorrekte branch prediction im ziel
  - dadurch transient execution
  - diese übermittelt daten via cache covert channel
  - angreifer liest diese aus -->

Um in Phase 2 eine falsche Branch Prediction zu induzieren, wird in Phase 1 ein *Training* des Branch Predictors durchgeführt. Im Allgemeinen beeinflusst dieses Training den mikroarchitekturellen Zustand des Branch Predictors, um für einen anvisierten Sprung in bestimmter Weise eine falsche Vorhersage zu verursachen. Ein konkretes Training kann beispielsweise darin bestehen, einen anvisierten bedingten Sprung wiederholt mit dem gewünschten Ausgang auszuführen. In diesem Fall wird der Sprung in Phase 2 mit einem anderen Ausgang ausgeführt, während der Branch Predictor weiterhin den trainierten Ausgang voraussagt. Es kommt also zu einer falschen Branch Prediction. [@transient-fail, Kap. 3]

<!-- - weiter unterteilt nach art und weise auf die der branch predictor trainiert wird: -->

Spectre-Angriffe basierend auf Branch Prediction werden weiter unterteilt nach der Art und Weise, auf die der Branch Predictor trainiert wird:

<!-- - teil des branch predictors, der angegriffen wird / art der trainierten sprünge
  - bedingte sprünge, indirekte sprünge, rücksprung aus funktionen können branch prediction auslösen
  - jede art der prediction kann damit ziel des trainings werden
  - (predictor verwendet dafür verschiedene buffer, z.B. pattern history table, branch target buffer, return stack buffer) -->

- Dies umfasst einerseits die Art der verwendeten Sprünge. Eine Branch Prediction kann ausgelöst werden durch **bedingte Sprünge**, **indirekte Sprünge** oder **Rücksprünge aus Funktionen**. Jede Art der Branch Prediction kann damit Ziel des Trainings werden. [@transient-fail, Kap. 3]

<!-- - same-address-space oder cross-address-space
  - same: training erfolgt in dem gleichen adressraum wie die inkorrekte branch prediction während des angriffes, typischerweise im kontext des opfers
  - cross: training erfolgt in einem anderen adressraum als die inkorrekte branch prediction während des angriffes, typischerweise im kontext des angreifers -->

- Da der Branch Predictor virtuelle Adressen verwendet [@spectre, Kap. V], kann das Training andererseits auch in unterschiedlichen Adressräumen stattfinden, sofern die virtuelle Adresse der zum Training und zum Angriff verwendeten Sprünge übereinstimmt. Erfolgt das Training in dem **gleichen Adressraum** wie die inkorrekte Branch Prediction während des Angriffs, so spricht man von einem *Same-Address-Space* Training. In diesem Fall geschieht das Training typischerweise in dem Kontext des Opfers. Erfolgt das Training in einem **anderen Adressraum** als die inkorrekte Branch Prediction während des Angriffs, so spricht man von einem *Cross-Address-Space* Training. In diesem Fall geschieht das Training typischerweise in dem Kontext des Angreifers. [@transient-fail, Kap. 3]

<!-- - in-place oder out-of-place
  - in: training erfolgt mit einem branch an der gleichen virtuellen adresse wie der anvisierte branch
  - out: training erfolgt mit einem branch mit einer anderen virtuellen adresse, die kongruent zu der virtuellen adresse des anvisierten branches ist
    - "kongruent" analog zur definition bei caches: nur ein teil der virtuellen adresse wird benutzt um die branches ggü dem predictor zu identifizieren -->

- Der Branch Predictor verwendet außerdem nicht die gesamte virtuelle Adresse eines Sprunges, sondern nur eine feste Anzahl der niederwertigsten Bits [@transient-fail, Kap. 3]. Dies ermöglicht das Training mit einem Sprung an einer **anderen virtuellen Adresse**, die kongruent[^branch-kongruent] zu der Adresse des anvisierten Sprunges ist. In diesem Fall spricht man von einem *Out-Of-Place* Training. Erfolgt das Training hingegen an der **gleichen virtuellen Adresse** wie der Angriff, spricht man von einem *In-Place* Training. [@transient-fail, Kap. 3]

[^branch-kongruent]: Das Konzept kongruenter Adressen aus [@sec:grundlagen-caches] lässt sich hier analog anwenden: Zwei Adressen sind kongruent, wenn sie in den vom Branch Predictor verwendeten Bits übereinstimmen.

<!-- - beispiele: spectre varianten
- spectre variant 1: conditional branches [@spectre, Kap. IV]
- spectre variant 2: indirect branches [@spectre, Kap. V]
- ret2spec, SpectreRSB: returns from functions
- hier nicht weiter betrachtet -->

Zu den Spectre-Angriffen basierend auf Branch Prediction gehören beispielsweise *Spectre Variant 1* [@spectre, Kap. IV], der bedingte Sprünge verwendet, und *Spectre Variant 2* [@spectre, Kap. V], der indirekte Sprünge verwendet. Weitere Beispiele sind durch *SpectreRSB* [@spectre-spectrersb] und *ret2spec* [@spectre-ret2spec] gegeben, die Rücksprünge aus Funktionen verwenden. Alle in [@sec:impl-angriffe] beschriebenen Spectre-Angriffe basieren auf Prozessor-Exceptions. Die Untersuchung von zusätzlichen Angriffen basierend auf Branch Prediction würde den Rahmen dieser Bachelorarbeit überschreiten. Aufgrund dessen werden in dieser Arbeit keine konkreten Spectre-Angriffe basierend auf Branch Prediction betrachtet.

## Spectre-Angriffe basierend auf Prozessor-Exceptions {#sec:impl-spectre-exception}

<!-- - auch Meltdown-type genannt, nach ersten Meltdown attacken
- transient execution ab phase 2 wird durch prozessor-exception ausgelöst, siehe sec:grundlagen-mikroarch-transient
- anders als spectre-type: phasen 2--5 werden im kontext des angreifers ausgeführt -->

Spectre-Angriffe basierend auf Prozessor-Exceptions werden auch *Meltdown-type Attacks* genannt nach dem Paper, das den ersten Angriff dieser Art beschrieben hat [@meltdown]. Bei Angriffen basierend auf Prozessor-Exceptions wird die Transient Execution ab Phase 2 durch eine Prozessor-Exception ausgelöst (siehe [@sec:grundlagen-mikroarch-transient]). Anders als bei Spectre-Angriffen, die auf Branch Prediction basieren, werden die Phasen 2 bis 5 im Kontext des Angreifers ausgeführt. [@transient-fail, Kap. 2]

<!-- - genereller ablauf der angriffe:
  - lesender speicherzugriff, der prozessor-exception auslöst (phase 2 und 3)
  - exception wird erst bei retirement des faulting load ausgelöst
  - daten des load sind schon vor retirement verfügbar
  - innerhalb der transient execution können folgende instruktionen diese daten benutzen und in den cache kodieren (phase 4)
  - nach ende der transient execution können die daten aus dem cache extrahiert werden (phase 6)
- bei diesen angriffen wird der initiale speicherzugriff so gewählt, dass potentiell daten des opfers geladen werden
  - details sind spezifisch für einen bestimmten angriff -->

Phasen 2 und 3 bestehen bei diesen Angriffen aus einem lesenden Speicherzugriff, der eine Prozessor-Exception und damit Transient Execution auslöst. Dieser initiale Speicherzugriff wird dabei so gewählt, dass potentiell Daten des Opfers geladen werden. Auf diese können folgende Instruktionen innerhalb der Transient Execution anschließend zugreifen. Die Details des initialen Speicherzugriffs sind abhängig von dem konkreten betrachteten Angriff. [@transient-fail, Kap. 2]

<!-- - weiter unterteilt nach: -->

Spectre-Angriffe basierend auf Prozessor-Exceptions werden nach zwei Gesichtspunkten weiter unterteilt:

<!-- - art der ausgelösten exception
  - alle exceptions die ausgelöst werden können, z.B.:
    - page fault: z.B. auf virtuelle adresse zugreifen der keine physische adresse zugeordnet ist
    - general protection fault: z.B. auf eine non-canonical virtuelle adresse zugreifen (adresse deren oberste 16-Bit nicht dem 48. Bit entsprechen und damit nicht teil des 48-bit adressraumes sind)
  - alternativ zu exceptions: microcode assists
    - werden durch bestimmte seltene ereignisse/ausnahmen in der mikroarchitektur ausgelöst
    - effekt auf mikroarchitektur ist ähnlich wie der von exceptions
    - assists sind aber nicht architekturell sichtbar, d.h. sie ändern nicht den architekturellen zustand -->

- Ein Aspekt ist die **Art der ausgelösten Prozessor-Exception**. Dazu zählen alle architekturellen Exceptions, die im Prozessor ausgelöst werden können [@transient-fail, Kap. 4]. *Page-Fault* Exceptions treten beispielsweise auf, wenn ein Zugriff auf eine virtuelle Adresse erfolgt, der kein physischer Speicher zugeordnet ist [@intel-sdm-3, S. 6-44]. *General Protection* Exceptions treten unter anderem auf, wenn auf eine virtuelle Adresse zugegriffen wird, die nicht Teil des 48-Bit Adressraumes ist [@intel-sdm-3, S. 6-42]. Eine solche virtuelle Adresse wird *nicht-kanonisch* genannt [@intel-sdm-3, Kap. 6, S. 42]. Neben den architekturellen Exceptions können auch *Microcode Assists* einen Spectre-Angriff ermöglichen [@zombieload, Kap. 3.2]. Diese werden auch *mikroarchitekturelle Exceptions* genannt [@zombieload, Kap. 5.1]. Microcode Assists werden durch bestimme Ausnahmezustände in der Mikroarchitektur ausgelöst. Ihr Effekt auf die Mikroarchitektur ist ähnlich zu dem von Prozessor-Exceptions und sie können ebenfalls Transient Execution auslösen. Anders als Prozessor-Exceptions sind Assists jedoch nicht architekturell sichtbar, d.h. sie ändern den architekturellen Zustand nicht. [@fallout, Kap. 5.1]

<!-- - buffer aus dem geleakt wird
  - übliche ziele sind L1 Data Cache, line-fill buffer, store buffer, load ports (siehe [@sec:grundlagen-mikroarch-memory]) -->

- Der andere Aspekt ist das **Element der Architektur oder Mikroarchitektur**, aus dem unberechtigt Daten extrahiert werden. Übliche Ziele sind hierbei der L1 Data Cache, der Line-Fill Buffer, der Store Buffer und die Load Ports (siehe [@sec:grundlagen-mikroarch-memory]). [@gruss-habil, S. 73--74] [@transient-fail, Erweiterter Klassifizierungsbaum]

<!-- - ausgelöste exceptions müssen vom angreifer behandelt oder unterdrückt werden
- dafür gibt es verschiedene techniken: -->

Sofern der Angriff Prozessor-Exceptions und nicht Microcode Assists verwendet, müssen die ausgelösten Exceptions behandelt oder unterdrückt werden. Dafür finden verschiedene Techniken Verwendung, die im Folgenden skizziert werden.

<!-- - Exception handling:
  - behandle das signal, das das betriebssystem an den angreifenden prozess sendet
  - da die exception vom betriebssystem verarbeitet wird, langsamer als exception suppression, mehr interferenz durch das OS oder dritte prozesse -->

Wird eine Prozessor-Exception ausgelöst, senden alle gängigen, modernen Betriebssysteme ein Signal an den aktuellen Prozess. Dieses Signal kann von dem Prozess abgefangen und behandelt werden. Diese Möglichkeit, mit Exceptions umzugehen, wird als *Exception Handling* bezeichnet. Da in diesem Fall zunächst das Betriebssystem die Prozessor-Exception bearbeitet, ist diese Methode langsamer als ihre Alternativen. Dies führt zu zusätzlicher Interferenz durch das Betriebssystem oder dritte Prozesse. [@meltdown, Kap. 4.1]

<!-- - Exception suppression: unterdrücke die exception, bevor sie das betriebssystem erreicht
  - löse die exception innerhalb einer Intel TSX Transaction aus
    - sehr schnell und daher auch sehr wenig interferenz, benötigt aber die TSX Prozessorerweiterung, daher hier nicht weiter betrachtet
  - oder: verursache transient execution durch falsche vorhersage eines sprunges, löse exception innerhalb dieser transient execution aus
    - dafür muss der branch predictor vorher entsprechend trainiert werden -->

Eine Alternative zur Behandlung von Prozessor-Exceptions ist die *Exception Suppression*, die entstehende Exceptions unterdrückt. Dadurch erreichen diese nie das Betriebssystem und mögliche Interferenzquellen werden reduziert. Wird die Exception innerhalb einer Intel TSX[^intel-tsx] Transaktion ausgelöst, so wird die Exception unterdrückt und stattdessen die Transaktion abgebrochen. Diese Art der Exception Suppression benötigt jedoch die Intel TSX Prozessorerweiterung. Eine andere Art der Exception Suppression nutzt Transient Execution durch eine falsche Branch Prediction. Wird eine Prozessor-Exception von einer Transient Instruction ausgelöst, so wird diese Exception ebenfalls unterdrückt. Für Exception Suppression durch Transient Execution muss der Branch Predictor entsprechend trainiert werden. Dies erfolgt auf die gleiche Art und Weise wie bei Spectre-Angriffen basierend auf Branch-Prediction, die in [@sec:impl-spectre-prediction] beschrieben wurde. [@meltdown, Kap. 4.1]

[^intel-tsx]: *Intel Transactional Synchronization Extensions*, kurz *TSX*, sind Erweiterungen für Intel Prozessoren. Diese erlauben es, mehrere Speicheroperationen innerhalb einer *Transaktion* auszuführen, sodass diese anderen Ausführungskernen als eine einzige atomare Operation erscheinen. [@intel-sdm-1, Kap. 16, S. 1]

<!-- - später werden drei konkrete angriffe dieses typs dargestellt (siehe xxx) -->

In [@sec:impl-angriffe] werden vier konkrete Spectre-Angriffe basierend auf Prozessor-Exceptions detailliert beschrieben. Details der Implementierung von Techniken zur Vermeidung von Prozessor-Exceptions werden in [@sec:impl-teile-exception] erläutert.

## Angriffsszenarien {#sec:impl-szenario}

<!-- - verschiedene angriffsszenarien werden unterschieden
- einerseits fähigkeiten und privilegien des angreifers, andererseits ziele des angreifers
- privilegien:
  - unprivilegierter nutzerprozess [@meltdown, Kap. 6.1] [@ridl, Kap. VI] [@zombieload, Kap. 4] [@fallout, Kap. 4] [@spectre, Kap. I] [@spectre-spectrersb, Kap. 5.3] [@spectre-ret2spec, Kap. 4]
  - betriebssystem [@zombieload, Kap. 4] [@spectre, Kap. V]
  - innerhalb eines TEE
  - in sandbox wie JavaScript im Browser [@ridl, Kap. VI] [@fallout, Kap. 6.2] [@spectre, Kap. I] [@spectre-ret2spec, Kap. 5] oder eBPF im Kernel [@spectre, Kap. I]
- ziele:
  - nutzerprozesse [@meltdown, Kap. 6.1] [@ridl, Kap. VI] [@zombieload, Kap. 4] [@fallout, Kap. 6.2] [@spectre, Kap. I] [@spectre-spectrersb, Kap. 5.3] [@spectre-ret2spec, Kap. 4] [@spectre-ret2spec, Kap. 5]
  - kernel [@meltdown, Kap. 6.1] [@ridl, Kap. VI] [@zombieload, Kap. 4] [@fallout, Kap. 4] [@spectre-spectrersb, Kap. 7]
  - andere container [@meltdown, Kap. 6.1]
  - andere VMs [@ridl, Kap. VI] [@zombieload, Kap. 4] [@spectre, Kap. V]
  - hypervisor [@zombieload, Kap. 4]
  - TEEs wie SGX [@ridl, Kap. VI] [@zombieload, Kap. 4] [@spectre-spectrersb, Kap. 6] -->

Im Kontext von Spectre-Angriffen werden verschiedene Angriffsszenarien unterschieden. Dies umfasst einerseits die vorhandenen Privilegien, die betrachteten Angreifern zugeschrieben werden, sowie andererseits die von diesen Angreifern anvisierten Strukturen.

In der Literatur wurden unter anderem konkrete Angriffe demonstriert, die von Angreifern in den folgenden Kontexten ausgehen:

- Unprivilegierte Nutzerprozesse [@meltdown, Kap. 6.1]

- Betriebssysteme [@zombieload, Kap. 4]

- JavaScript-Programme in Browsern [@ridl, Kap. VI]

- eBPF-Programme im Linux-Kern [@spectre, Kap. IV.D]

Die betrachteten Angreifer visierten dabei folgende Ziele an:

- Andere Nutzerprozesse [@meltdown, Kap. 6.1]

- Den Betriebssystem-Kern [@zombieload, Kap. 4]

- Andere Container [@meltdown, Kap. 6.1]

- Andere Virtuelle Maschinen [@ridl, Kap. VI]

- Hypervisors [@zombieload, Kap. 4]

- Trusted Execution Environments wie Intel SGX [@ridl, Kap. VI]

<!-- Im Kontext von Spectre-Angriffen werden verschiedene Angriffsszenarien unterschieden. Dies umfasst einerseits die vorhandenen Privilegien, die betrachteten Angreifern zugeschrieben werden, sowie andererseits die von diesen Angreifern anvisierten Strukturen. In der Literatur werden unter anderem Angreifer betrachtet, die Privilegien von Nutzerprozessen [@meltdown, Kap. 6.1], Betriebssystemen [@zombieload, Kap. 4] oder eingeschränkten Umgebungen wie JavaScript in Browsern [@ridl, Kap. VI] oder eBPF-Programmen im Linux-Kern [@spectre, Kap. IV.D] besitzen. Die betrachteten Angreifer visierten dabei Ziele wie andere Nutzerprozesse [@meltdown, Kap. 6.1], den Betriebssystem-Kern [@zombieload, Kap. 4], Container [@meltdown, Kap. 6.1], Virtuelle Maschinen [@ridl, Kap. VI], Hypervisor [@zombieload, Kap. 4] oder Trusted Execution Environments wie Intel SGX [@ridl, Kap. VI] an. -->

Außerdem wird die Relation der Ausführungskerne, auf denen Angreifer und Opfer ausgeführt werden, betrachtet. Dabei wird zwischen drei Konstellationen unterschieden:

<!-- - um angriffe möglichst allgemeingültig zu halten, hier alles nur von unprivilegierter linux userspace aus, ohne prozessorerweiterungen wie tsx
- verschiedene angriffsszenarien werden unterschieden:
  - same-thread: angreifer und opfer werden auf dem gleichen logischen kern ausgeführt
  - cross-thread: gleicher physischer kern, anderer logischer kern
  - cross-core: anderer physischer kern
- alle angriffe hier sind cross-thread, same-thread??? -->
<!-- Im Kontext von Spectre-Angriffen werden drei verschiedene Angriffsszenarien unterschieden: -->

- *Same-Thread*: Angreifer und Opfer werden auf dem gleichen logischen Kern ausgeführt. [@ridl, Kap. 1]

- *Cross-Thread*: Angreifer und Opfer werden auf dem gleichen physischen Kern, aber auf unterschiedlichen logischen Kernen ausgeführt. [@ridl, Kap. 1]

- *Cross-Core*: Angreifer und Opfer werden auf unterschiedlichen physischen Kernen ausgeführt. [@ridl, Kap. 1]

Alle in dieser Arbeit implementierten Angriffe setzen voraus, dass Angreifer und Opfer bestimmte Elemente physischer Kerne gemeinsam verwenden. Damit handelt es sich bei diesen um Cross-Thread Angriffe. Um die hier implementierten Angriffe möglichst allgemein anwendbar zu halten, werden außerdem nur Angriffe betrachtet, die von unprivilegierten Nutzerprozessen auf Linux-Systemen durchgeführt werden können. Anvisiert werden dabei andere Nutzerprozesse oder der Betriebssystem-Kern. Zusätzlich werden aus demselben Grund keine Angriffe betrachtet, die Unterstützung für Prozessorerweiterungen wie Intel TSX voraussetzen.

<!-- - Ziele: andere nutzerprozesse oder kernel -->

<!-- ## Teile der Implementierung {#sec:impl-teile} -->

## Wiederverwendbare Elemente der Implementierung von Spectre-Angriffen {#sec:impl-teile}

<!-- - mehrere eigenständige teile der implementierung, die in verschiedenen angriffen wiederverwendet werden -->

<!-- Dieses Kapitel beschreibt mehrere eigenständige Teile der Implementierung von Spectre-Angriffen. Diese werden in den in [@sec:impl-angriffe] betrachteten Angriffen wiederverwendet. -->

Dieses Kapitel beschreibt Elemente der Implementierung von Spectre-Angriffen, die unabhängig von den konkreten Angriffen sind und in mehreren der in [@sec:impl-angriffe] betrachteten Angriffen wiederwerwendet werden. Zunächst wird in [@sec:impl-teile-interferenz] beschrieben, wie eine häufige Quelle der Interferenz vermieden wird. Danach wird in [@sec:impl-teile-latenz] eine Technik zur Messung der Latenz einzelner Instruktionen erläutert. Anschließend wird in [@sec:impl-teile-cache] dargestellt, wie Daten in den Cache kodiert und aus diesem dekodiert werden. Schließlich werden in [@sec:impl-teile-exception] zwei Methoden zum Umgang mit auftretenden Prozessor-Exceptions erklärt.

<!-- - aller code ist der bachelorarbeit beigelegt
- wiederverwendbare elemente sind in den dateien common.h und common.c implementiert -->

Alle hier beschriebenen Elemente wurden im Rahmen dieser Arbeit implementiert. Der Programmcode der Implementierung ist in den Dateien `common.c` und `common.h` beigelegt.

<!-- - das sind möglichst viele aspekte, die von den konkreten angriffen unabhängig sind und wiederverwendet werden können
- insbesondere folgende teile
  - wie wird interferenz durch das betriebssystem vermieden
  - wie wird die latenz einzelner instruktionen gemessen
  - wie werden daten in den cache kodiert und aus ihm dekodiert
  - wie werden exceptions behandelt -->

### Reduzierung der Interferenz durch Wechsel zwischen Ausführungskernen {#sec:impl-teile-interferenz}

<!-- - viele betriebssysteme erlauben es auch unprivilegierten nutzern ihre prozesse an einzelne Ausführungskerne zu binden. Dadurch werden diese prozesse nur auf den gewählten logischen Kernen ausgeführt.
- um interferenz durch Wechsel zwischen Ausführungskernen zu verringern, angreifer und opfer an einzelne logische kerne binden -->

Werden während eines Spectre-Angriffs der Prozess des Angreifers oder der Prozess des Opfers von dem Betriebssystem auf einen anderen Ausführungskern migriert, so kann dies mit dem laufenden Angriff interferieren. Viele Betriebssysteme erlauben auch unprivilegierten Nutzern, ihre Prozesse an einzelne logische Ausführungskerne zu binden. Dadurch werden diese Prozesse nur auf den gewählten logischen Kernen ausgeführt. Um Interferenzen durch den Wechsel zwischen Ausführungskernen zu vermeiden, werden daher sowohl der angreifende Prozess als auch der Prozess des Opfers an einzelne logische Kerne gebunden.

<!-- - angreifer- und opfer-prozess an den gleichen logischen kern zu binden ermöglicht es same-thread angriffen zuverlässig zu funktionieren
- bei cross-thread angriffen hingegen sollten angreifer und opfer an unterschiedliche logische kernen des gleichen physischen kerns gebunden werden
- außerdem kann die stabilität von cross-core angriffen erhöht werden, wenn angreifer- und opfer-prozess an einzelne logische kerne von unterschiedlichen physischen kernen gebunden werden -->

Den Prozess des Angreifers und den Prozess des Opfers an den gleichen logischen Ausführungskern zu binden, ermöglicht es Same-Thread Angriffen zuverlässig zu funktionieren. Bei Cross-Thread Angriffen hingegen sollten der Prozess des Angreifers und der des Opfers an unterschiedliche logische Kerne des gleichen physischen Kerns gebunden werden, um zuverlässige Funktion zu ermöglichen. Außerdem kann die Stabilität von Cross-Core Angriffen erhöht werden, indem der Prozess des Angreifers und der Prozess des Opfers an einzelne logische Kerne von unterschiedlichen physischen Kernen gebunden werden.

### Messung der Latenz einzelner Instruktionen {#sec:impl-teile-latenz}

<!-- - für cache-basierte seitenkanalangriffe muss der angreifer die latenz einzelner instruktionen messen können
- wie in kap sidechannel erwähnt geht das auf x86 prozessoren mit der rdtsc (Read Time-Stamp Counter) instruktion
- diese instruktion lädt ein prozessorinternes register, das mit jedem prozessorzyklus hochgezählt wird [@intel-sdm-2, Kap. 4, S. 547] -->

Um Cache-basierte Seitenkanalangriffe durchzuführen, muss ein Angreifer die Ausführungslatenz einzelner Instruktionen messen können. Wie in [@sec:grundlagen-sidechannel] erwähnt, geht dies auf x86 Prozessoren mithilfe der Instruktion `rdtsc` (Read Time-Stamp Counter). Diese Instruktion lädt ein prozessorinternes Register, das mit jedem Prozessorzyklus hochgezählt wird [@intel-sdm-2, Kap. 4, S. 547].

<!-- - die einzelne instruktion, deren latenz gemessen werden soll, kann nicht einfach mit rdtsc umgeben werden, da durch out-of-order execution die ausführungsreihenfolge nicht festgelegt ist
- Erstmal Zeitmessung für load betrachten, flush wird später betrachtet -->

Um die Messung der Latenz vorzunehmen, genügt es jedoch nicht, die einzelne Instruktion mit `rdtsc` Instruktionen zu umgeben, da die Ausführungsreihenfolge dieser Instruktionen nicht festgelegt ist. Durch Out-Of-Order Execution kann die zu messende Instruktion beispielsweise vor oder hinter beide `rdtsc` Instruktionen geordnet werden, wodurch das Messergebnis bedeutungslos würde. Um dieses Problem zu umgehen, unterstützen x86 Prozessoren besondere Instruktionen. Diese ordnen die Ausführungsreihenfolge von Instruktionen oder von Speicherzugriffen auf festgelegte Weise. Im Folgenden wird zunächst die Messung der Latenz von lesenden Speicherzugriffen betrachtet, wie sie für den Flush+Reload Angriff benötigt wird. Die `clflush` Instruktion, die Cacheeinträge invalidiert, wird anschließend untersucht. Eine Latenzmessung dieser wird für den Flush+Flush Angriff benötigt.

<!-- - Ordnende Instruktionen:
  - cpuid (CPU Identification) 3-198: ruft informationen über den prozessor ab, außerdem serialisiert den instruktionsfluss, alle vorherigen instruktionen und alle speicherzugriffe werden abgeschlossen, bevor folgende instruktionen geladen werden
  - lfence (Load Fence) 3-559: wird erst ausgeführt wenn alle vorherigen instruktionen ausgeführt wurden und keine folgende instruktion wird ausgeführt, bevor lfence abgeschlossen wurde, außerdem unterbricht speculative execution, damit einzigartiger fence
  - sfence (Store Fence) 4-611: alle vorherigen schreibenden speicherzugriffe sind global sichtbar vor allen folgenden schreibenden speicherzugriffen
  - mfence (Memory Fence) 4-22: wie sfence aber auch für loads, also alle vorherigen speicherzugriffe sind global sichtbar vor allen folgenden speicherzugriffen, das heißt für lesende speicherzugriffe dass die geladenen daten feststehen und für stores dass sie von anderen ausführungskernen beobachtet werden können
  - rdtscp (Read Time-Stamp Counter and Processor ID) 4-549: wie rdtsc, nur wartet bis alle vorherigen instruktionen ausgeführt wurden und die adressen aller lesenden speicherzugriffe feststehen, also ähnlich zu lfence+rdtsc ohne spec barrier -->

x86 Prozessoren besitzen unter anderem folgende Instruktionen, die die Ausführung anderer Instruktionen ordnen:

- `cpuid` (CPU Identification) ruft Informationen über den Prozessor des Systems ab. Außerdem serialisiert `cpuid` den Instruktionsfluss: Alle ihr vorhergehenden Instruktionen werden ausgeführt und alle Speicherzugriffe abgeschlossen, bevor ihr nachfolgende Instruktionen geladen werden. [@intel-sdm-2, Kap. 3, S. 198]

- `lfence` (Load Fence) ordnet lesende Speicherzugriffe sowie teilweise auch den gesamten Instruktionsfluss. Bevor `lfence` ausgeführt wird, werden alle vorhergehenden Instruktionen ausgeführt. Außerdem wird keine nachfolgende Instruktion ausgeführt, bevor `lfence` abgeschlossen wurde. Dadurch unterbricht `lfence` auch spekulative Ausführung. Diese Eigenschaft besitzt nur `lfence`; andere Fence-Instruktionen ordnen den Instruktionsfluss nicht. Da die Ausführung schreibender Instruktionen beendet wird, bevor der von ihnen ausgelöste Speicherzugriff abgeschlossen ist, ordnet `lfence` keine schreibenden Speicherzugriffe. [@intel-sdm-2, Kap.\ 3, S.\ 559]

- `sfence` (Store Fence) ordnet schreibende Speicherzugriffe. Alle vorhergehenden schreibenden Speicherzugriffe werden abgeschlossen, bevor ein nachfolgender schreibender Speicherzugriff abgeschlossen wird. [@intel-sdm-2, Kap. 4, S. 611]

- `mfence` (Memory Fence) ist ähnlich zu `sfence`, ordnet aber zusätzlich auch lesende Speicherzugriffe. Alle vorhergehenden Speicherzugriffe werden abgeschlossen, bevor ein nachfolgender Speicherzugriff abgeschlossen wird. [@intel-sdm-2, Kap. 4, S. 22]

- `rdtscp` (Read Time-Stamp Counter and Processor ID) ist in ihrer Funktionsweise ähnlich zu der `rdtsc` Instruktion. Zusätzlich wird `rdtscp` erst ausgeführt, wenn alle vorhergehenden Instruktionen ausgeführt wurden. Damit ist `rdtscp` ähnlich zu `lfence` gefolgt von `rdtsc`. [@intel-sdm-2, Kap. 4, S. 549]

<!-- - Einfache Möglichkeit: umgeben mit cpuid
- c code, benutzt compiler built-in funktionen von gcc/clang
- cpuid rdtsc cpuid load cpuid rdtsc cpuid -->

Eine einfache Möglichkeit, einen lesenden Speicherzugriff gegenüber `rdtsc` Instruktionen zu ordnen, besteht also darin, sämtliche Instruktionen mit `cpuid` zu umgeben. Dies zeigt der Beispielcode in [@lst:time-cpuid]. Dabei handelt es sich um C Code, der die von Intel angegebenen Funktionen zur Ausführung der entsprechenden Prozessorinstruktionen verwendet [@intel-intrin]. Für `cpuid` gibt Intel keine Funktion an, daher wird für diese Instruktion eine fiktive Funktion `void cpuid(void)` verwendet. Außerdem wird eine Funktion `void mem_access(const void *)` verwendet, die auf die übergebene Speicheradresse lesend zugreift.

```{.c float=tbp #lst:time-cpuid}
cpuid();
start = _rdtsc();
cpuid();
mem_access(target);
cpuid();
end = _rdtsc();
cpuid();
latency = end - start;
```

: Latenzmessung eines lesenden Speicherzugriffs, durch cpuid geordnet.

<!-- - fences sind effizienter als cpuid [@intel-sdm-3, Kap. 8, S. 16]
- also besser: genau passende fences
- speculative execution der rdtsc oder des load soll verhindert werden, daher vor und nach jeder instruktion einen lfence
- diese fences ordnen die rdtsc und den load schon vollständig
- lfence rdtsc lfence load lfence rdtsc lfence -->
<!-- - alle instruktionen und speicherzugriffe vor dem ersten rdtsc sollen vor diesem ausgeführt werden, daher mfence lfence am anfang
- dann erstes rdtsc
- load soll nach dem ersten rdtsc ausgeführt werden, daher vor dem load ein lfence
- dann load
- load soll vor dem zweiten rdtsc ausgeführt werden, daher vor dem zweiten rdtsc ein lfence
- da auf diesen lfence unmittelbar das zweite rdtsc folgt, können beide instruktionen durch ein rdtscp ersetzt werden
- mfence lfence rdtsc lfence load rdtscp lfence -->
<!-- Dieser `lfence` unmittelbar vor dem zweiten `rdtsc` lässt sich nun mit demselben zu einem `rdtscp` kombinieren. Vor dem ersten `rdtsc` wird diese Kombination nicht durchgeführt, da -->

Da Fence-Instruktionen effizienter sind als `cpuid` [@intel-sdm-3, Kap. 8, S. 16], kann diese Technik der Latenzmessung optimiert werden. Alle Instruktionen und Speicherzugriffe vor dem ersten `rdtsc` sollen vor diesem abgeschlossen werden. Daher wird ein `mfence` gefolgt von einem `lfence` vor dem ersten `rdtsc` platziert. Der zu messende Speicherzugriff soll nach dem ersten `rdtsc` ausgeführt werden, daher wird zwischen diese ein `lfence` platziert. Andererseits soll der zu messende Speicherzugriff vor dem zweiten `rdtsc` ausgeführt werden, daher wird zwischen diese ein weiterer `lfence` platziert. Schließlich soll die zweite `rdtsc` Instruktion vor allen ihr folgenden Instruktionen ausgeführt werden. Aus diesem Grund wird hinter ihr erneut ein `lfence` platziert. Da beide `rdtsc` Instruktionen unmittelbar auf `lfence` Instruktionen folgen, können diese Paare jeweils zu einer `rdtscp` Instruktion kombiniert werden. Insgesamt ergibt sich dadurch der Code in [@lst:time-fence].

```{.c float=tbp #lst:time-fence}
_mm_mfence();
start = __rdtscp(&unused);
_mm_lfence();
mem_access(target);
end = __rdtscp(&unused);
_mm_lfence();
latency = end - start;
```

: Latenzmessung eines lesenden Speicherzugriffs, durch Fence-Instruktionen geordnet.

<!-- - latenzmessung: beschreiben wie viele zykel overhead die messungen haben (cpuid / fences)
  - overhead lässt sich messen, in dem der aufruf von mem_access entfernt wird
  - im schnitt / ungefähr cpuid: 135 cycles, fences: 24 cycles
  - die optimierung, cpuid durch fences zu ersetzen, lohnt sich also -->

Ohne den Aufruf von `mem_access` benötigt die durch `cpuid` geordnete Latenzmessung ([@lst:time-cpuid]) im Schnitt 135 Prozessorzyklen, während die durch Fence-Instruktionen geordnete Latenzmessung ([@lst:time-fence]) im Schnitt nur 24 Zyklen benötigt. Die optimierte Variante der Latenzmessung führt also zu einer wesentlichen Reduktion der Zeit, die für die Messung selber benötigt wird. Dadurch wird das Zeitfenster für potentielle Interferenz durch dritte Prozesse oder durch das Betriebssystem verringert. Außerdem wird Angriffen auf diese Weise eine höhere Übertragungsrate ermöglicht.

<!-- - Um flush zu messen: lfences sind wieder wegen speculative execution nötig, lfences ordnen clflush (3-145), daher die gleichen fences wie bei load -->
<!-- - Um flush zu messen: lfence ordnet clflush, rdtscp jedoch nicht, ersetze daher rdtscp wieder durch lfence+rdtsc
- mfence rdtscp lfence clflush lfence rdtsc lfence -->

<!-- - clflush nur durch mfence geordnet [@flush-flush, Kap. 3]
- daher lfence nach rdtscp durch mfence ersetzen, zweites rdtscp durch mfence;rdtsc -->

Um die Latenz von `clflush` Instruktionen zu messen, müssen an diesem Code zwei Änderungen vorgenommen werden. `clflush` Instruktionen werden nur durch `mfence` Instruktionen geordnet und nicht durch die vorhandenen `lfence` oder `rdtscp` Instruktionen [@flush-flush, Kap. 3]. Aus diesem Grund wird die zweite `rdtscp` Instruktion wieder durch eine `lfence` Instruktion, gefolgt von einer `rdtsc` Instruktion, ersetzt. Anschließend werden die den Speicherzugriff umgebenden `lfence` Instruktionen durch `mfence` Instruktionen ersetzt. Mit diesen Änderungen ergibt sich der in [@lst:time-flush] abgedruckte Code.

<!-- Um die Latenz von `clflush` Instruktionen zu messen, muss an diesem Code nur eine einzelne Änderung vorgenommen werden. Während `clflush` Instruktionen durch `lfence` Instruktionen geordnet werden, werden sie nicht durch `rdtscp` Instruktionen geordnet [@intel-sdm-2, Kap. 3, S. 145]. Daher wird die zweite `rdtscp` Instruktion wieder durch eine `lfence` Instruktion gefolgt von einer `rdtsc` Instruktion ersetzt. Mit dieser Änderung ergibt sich der in [@lst:time-flush] abgedruckte Code. -->

```{.c float=tbp #lst:time-flush}
_mm_mfence();
start = __rdtscp(&unused);
_mm_mfence();
_mm_clflush(target);
_mm_mfence();
end = _rdtsc();
_mm_lfence();
latency = end - start;
```

: Latenzmessung einer clflush Instruktion, durch Fence-Instruktionen geordnet.

<!-- - vergleich mit anderen arbeiten -->

### Cache als Übertragungskanal {#sec:impl-teile-cache}

<!-- - flush+reload oder flush+flush -->

Spectre-Angriffe erfordern einen Übertragungskanal aus dem Kontext der Transient Execution in den architekturellen Zustand. Wie in [@sec:impl-allgemein] beschrieben, wird in den hier implementierten Angriffen dafür der Cache verwendet. Um Daten in diesen Kanal zu kodieren, werden ausgewählte Cachezeilen eingeladen. Um die Daten anschließend aus dem Kanal zu extrahieren, wird der Zustand des Caches über einen Cache-basierten Seitenkanalangriff beobachtet.

Bei allen konkreten Angriffen, die in [@sec:impl-angriffe] betrachtet werden, wird ausschließlich der L1d Cache als Übertragungskanal verwendet. Die von diesen Angriffen anvisierten Puffer der Mikroarchitektur werden nicht zwischen mehreren physischen Prozessorkernen geteilt. Folglich können die Angriffe in einem Cross-Core Szenario nicht funktionieren. Da innerhalb eines physischen Kerns der L1d Cache der schnellste von logischen Kernen gemeinsam verwendete Cache ist, wird ausschließlich dieser als Übertragungskanal verwendet.

Zwei Aspekte der Implementierung eines solchen Cache-basierten Übertragungskanals werden im Folgenden dargestellt. In [@sec:eval-sidechannel] wird dieser Übertragungskanal unter Verwendung von Flush+Reload und Flush+Flush evaluiert.

#### Klassifizierung von Latenzen {#sec:impl-teile-cache-threshold}

<!-- - cache sidechannel muss latenzen der jeweiligen instruktion klassifizieren, nach cache-hit oder cache-miss
- wird üblicherweise mit festem grenzwert (*threshold*) gemacht, der die beiden fälle unterscheidet
- wie threshold bestimmen?
  - messen
  - ausreißer rausnehmen
  - normalverteilung fitten
  - schnittpunkt nehmen
- siehe evaluation für konkrete durchführung -->

Ein Cache-basierter Seitenkanalangriff muss die gemessenen Latenzen der jeweiligen Instruktion danach klassifizieren, ob sie auf einen Cache Hit oder einen Cache Miss hindeuten (siehe [@sec:grundlagen-sidechannel]). Um diese Klassifikation vorzunehmen, wird üblicherweise ein fester Grenzwert verwendet. Dieser wird auch *Threshold* genannt. [@flush-reload, Kap. 3]

<!-- - viele messungen -> beschriftete messergebnisse
- threshold manuell wählen, um korrekte klassifizierung zu maximieren
- ausreißer sollten dabei nicht beachtet werden
- gründe für ausreißer nennen -->

Der Threshold kann in Vorbereitung auf einen Angriff wie folgt bestimmt werden. Zunächst werden möglichst viele Latenzmessungen für beide möglichen Zustände des Caches durchgeführt, um beschriftete Messergebnisse zu erhalten. Aus diesen Ergebnissen werden anschließend Ausreißer gefiltert. Diese können beispielsweise auftreten, wenn der Prozess während einer Latenzmessung von dem Betriebssystem unterbrochen wird. Nun wird ein geeigneter Threshold gewählt, der den Anteil der korrekt klassifizierten Messungen maximiert. Die Auswahl dieses Thresholds findet üblicherweise manuell statt [@flush-reload, Kap. 3].

<!-- Der Threshold kann in Vorbereitung auf einen Angriff wie folgt bestimmt werden. Zunächst werden möglichst viele Latenzmessungen für beide möglichen Zustände des Caches durchgeführt, um beschriftete Messergebnisse zu erhalten. Aus diesen Ergebnissen werden anschließend Ausreißer gefiltert. Ausreißer können beispielsweise auftreten, wenn der Prozess während einer Latenzmessung von dem Betriebssystem unterbrochen wird. Basierend auf der Annahme, dass die Latenzen für Cache Hits und Cache Misses jeweils normalverteilt sind, werden die Messergebnisse nun durch Normalverteilungen ersetzt, die in Erwartungswert und Standardabweichung mit den gemessenen Verteilungen übereinstimmen[^sample-distribution]. Um den optimalen Threshold zu bestimmen, wird schließlich der Schnittpunkt dieser beiden Normalverteilungen berechnet.

[^sample-distribution]: Streng genommen werden Normalverteilungen gewählt, deren Erwartungswerte und Standardabweichungen übereinstimmen mit den Stichprobenmitteln und Stichprobenstandardabweichungen der Messergebnisse. -->

In [@sec:eval-sidechannel] wird eine solche Bestimmung des Thresholds anhand konkreter Messungen durchgeführt.

#### Kodierung von Daten im Cache

<!-- - angriffe werden immer 8 bit in den cache kodieren
- daher beobachtet der angreifer 256 cache lines auf einmal
- jede auf einer eigenen page um prefetching zu reduzieren
- transient execution lädt eine der cachezeilen (in phase 4), diese landet im cache
- welche in den cache geladen wurde durch sidechannel rausfinden
- index dieser cachezeile gibt die übertragenen 8 bit an
- dadurch daten von transient execution zu normaler execution übertragen -->

![Beobachtete Cachezeilen, jede auf einer eigenen Page.](figs/cache_code.pdf){#fig:impl-teile-cache}

Üblicherweise kodieren Spectre-Angriffe 8 Bit in den Zustand des Caches, indem eine von 256 Cachezeilen in den Cache geladen wird [@meltdown, Kap. 4.2] [@ridl, Kap. IV] [@fallout, Kap. 3.1]. Um Interferenz durch Optimierungen wie Prefetching zu reduzieren, wird jede der Cachezeilen auf einer eigenen Page platziert [@intel-opt, Kap. 3, S. 61--62]. Der verwendete Cache-basierte Seitenkanalangriff beobachtet diese 256 Cachezeilen. Wird eine Cachezeile als im Cache vorhanden erkannt, so gibt ihr Index die übertragenen 8 Bit an. [@fig:impl-teile-cache] illustriert die beobachteten Cachezeilen und zeigt beispielhaft die Cachezeile, die bei Kodierung des Wertes 3 eingeladen wird.

Auf analoge Art und Weise kann eine beliebige Anzahl an Möglichkeiten in diesem Übertragungskanal kodiert und aus ihm extrahiert werden.

### Behandlung und Unterdrückung von Prozessor-Exceptions {#sec:impl-teile-exception}

Wie in [@sec:impl-spectre-exception] beschrieben, müssen Spectre-Angriffe ausgelöste Prozessor-Exceptions entweder behandeln oder unterdrücken.

<!-- - Exception handling: wie oben beschrieben, behandle signal das das betriebssystem an den prozess des angreifers schickt
- registriere signal handler über sigaction
- speichere zustand der ausführung vor auslösen der exception, mittels setjmp
- signal handler stellt diesen zustand wieder her, mittels longjmp
- (über rückgabewert von setjmp kann unterschieden werden, ob zustand wiederhergestellt wurde oder zum ersten mal erreicht wird / kann der erste vom zweiten return unterschieden werden) -->

Um Prozessor-Exceptions zu behandeln, registriert der angreifende Prozess einen *Signal Handler* mittels der Funktion `sigaction` [@man-sigaction]. Vor Auslösen der Exception speichert der Prozess seinen aktuellen Ausführungszustand mithilfe der Funktion `setjmp` [@man-setjmp]. Nach Auslösen der Exception wird dieser Zustand durch die Funktion `longjmp` wiederhergestellt [@man-setjmp].

<!-- - Exception suppression: wie oben beschrieben, verursache transient execution durch falsche vorhersage eines sprunges, löse exception innerhalb dieser transient execution aus
- training erfolgt wie bei spectre-angriffen die auf branch prediction basieren, aber da hier die misprediction im kontext des angreifers passiert hat dieser volle kontrolle über den code und kann immer same-address-space in-place nehmen
- weiterhin verschiedene arten der misprediction:
  - function returns, conditional branches, indirect branches
- funktioniert nicht, siehe evaluation -->

Um Prozessor-Exceptions hingegen zu unterdrücken, führt der angreifende Prozess die auslösende Instruktion während einer Transient Execution aus. Diese Transient Execution folgt dabei auf die falsche Vorhersage eines Sprunges durch den Branch Predictor. Das dafür erforderliche Training des Branch Predictors erfolgt wie bei Spectre-Angriffen basierend auf Branch Prediction. Die Art und Weise dieses Trainings wurde bereits in [@sec:impl-spectre-prediction] behandelt. Der einzige Unterschied des hier vorliegenden Anwendungsfalles zu dem aus [@sec:impl-spectre-prediction] besteht darin, dass hier die falsche Vorhersage im Kontext des Angreifers ausgelöst wird. Da der Angreifer in diesem Kontext die Kontrolle über jeglichen ausgeführten Code hat, kann er stets ein Same-Address-Space In-Place Training durchführen. Die Art der verwendeten Sprünge kann weiterhin variiert werden. In [@sec:eval-transient] wird diese Technik zur Unterdrückung von Prozessor-Exceptions evaluiert unter der Verwendung von bedingten Sprüngen, indirekten Sprüngen und Rücksprüngen aus Funktionen.

Die andere in [@sec:impl-spectre-exception] beschriebene Technik zur Unterdrückung von Prozessor-Exceptions setzt voraus, dass der Prozessor die Intel TSX Prozessorerweiterung unterstützt. Wie in [@sec:impl-szenario] erwähnt, werden in dieser Arbeit keine Angriffe betrachtet, die eine Unterstützung für Intel TSX voraussetzen. Daher wird eine Implementierung dieser Technik hier nicht behandelt.

<!-- - vorgehensweise jeweils:
  - training: target branch mehrmals ausführen in richtung die den zugriff ausführt
  - branch history des trainings nachbilden
  - target branch ausführen in die richtung die nichts tut
  - per flush+reload feststellen, ob zugriff erfolgt ist -->

<!-- ### optimize bias towards 0 -->

<!-- ### prefetcher verwirren -->

## Funktionsweise und Details der Implementierung ausgewählter Spectre-Angriffe {#sec:impl-angriffe}

<!-- - verschiedene varianten werden beschrieben
- es wird auf die funktionsweise und stellenweise auf details der implementierung eingegangen
- alle sind spectre-angriffe basierend auf prozessor-exceptions
- jeweils gute abbildungen aus den papern? -->

In diesem Kapitel werden ausgewählte konkrete Spectre-Angriffe beschrieben. Dabei wird sowohl auf ihre Funktionsweise als auch auf Details ihrer Implementierung eingegangen. Alle behandelten Angriffe sind Spectre-Angriffe basierend auf Prozessor-Exceptions (siehe [@sec:impl-spectre-exception]).

<!-- - nochmal szenario wiederholen:
  - aktuelles linux-system
  - unprivilegierte userspace
  - ziele: andere prozesse oder kernel
  - kein tsx oder andere erweiterungen -->

Wie in [@sec:impl-szenario] bereits erläutert, werden alle hier behandelten Angriffe von unprivilegierten Nutzerprozessen auf einem aktuellen Linux-System durchgeführt. Anvisiert werden dabei entweder andere Nutzerprozesse oder der Betriebssystem-Kern. Außerdem werden keine Angriffe betrachtet, die Unterstützung für Prozessorerweiterungen wie Intel TSX voraussetzen.

<!-- - aller code ist der bachelorarbeit beigelegt
- dokumentation in README
- jeder angriff ist in einer einzelnen datei implementiert (ridl.c, wtf.c, storetoleak.c, zombieload.c) -->

Alle hier beschriebenen Angriffe wurden im Rahmen dieser Arbeit implementiert. Der Programmcode der Implementierungen ist in den Dateien `ridl.c`, `wtf.c`, `storetoleak.c` und `zombieload.c` beigelegt.

<!-- ## Spectre ??

- variante 1:
  - conditional branches für bounds checks werden inkorrekt trainiert
  - benötigt sehr spezifische gadgets um nützliche daten zu leaken
  - konkretes beispiel?? gadget code und angreifer beschreiben
- variante 2:
  - indirect branches werden trainiert
  - training kann im kontext des angreifers erfolgen -> transient execution von beliebigen gadgets
  - ähnlich zu ROP
  - braucht passende gadgets, weniger eingeschränkt als variante 1
  - konkretes beispiel?? gadget code und angreifer beschreiben

## Meltdown ??

- kernel ist in adressraum des prozesses gemappt
- übersetzung von kerneladressen liefert valide physische adresse
- bei zugriff werden daten von dieser adresse schon geladen
- berechtigung fehlt -> exception wird ausgelöst am ende des loads
- wie oben beschrieben:
  - faulting load und instruktionen danach werden zur ausführung eingereiht
  - sobald daten des loads verfügbar sind können folgende instruktionen ausgeführt werden
  - exception wird jedoch erst generiert wenn load retiret
  - folgende instruktionen werden zurückgerollt, zustand des caches jedoch nicht
  - über cache covert channel informationen extrahieren
- insgesamt: daten von kernel adressen lesen -->

### RIDL: Rogue In-Flight Data Load {#impl-angriffe-ridl}

<!-- - ausgelöste exception: page fault
- angegriffenes element: line-fill buffer, load port -->

*Rogue In-Flight Data Load*, kurz *RIDL*, ist ein Spectre-Angriff basierend auf Prozessor-Exceptions (siehe [@sec:impl-spectre-exception]). RIDL verwendet Page-Fault Exceptions, um unberechtigt auf Daten aus dem Line-Fill Buffer oder den Load Ports zuzugreifen. [@ridl, Kap. IV, VI]

<!-- - ablauf:
- zugriff auf speicher, bei dem ein page fault auftritt
- wie oben beschrieben, löst transient execution aus bevor die exception bei retirement des load behandelt wird
- transient instructions können trotz der exception die daten des initialen speicherzugriffes verwenden
- an dieser stelle wird zwischen zwei varianten unterschieden, die sich in der art des initialen speicherzugriffes unterscheiden
- speicherzugriff auf einzelne cachezeile -> daten des zugriffs werden vom line-fill buffer bereitgestellt
- speicherzugriff auf mehrere cachezeilen -> daten des zugriffs werden von einem load port bereitgestellt
- in beiden fällen werden die daten ohne jegliche prüfung bereitgestellt, unabhängig davon ob sie aus dem angreifenden prozess, anderen prozessen auf dem system oder dem betriebssystemkern stammen -->

Phase 2 dieses Spectre-Angriffs besteht aus einem lesenden Speicherzugriff, durch den eine Page-Fault Exception ausgelöst wird. Wie in [@sec:grundlagen-mikroarch-transient] beschrieben, löst dies eine Transient Execution aus, bevor die Exception bei Retirement des Speicherzugriffs behandelt wird. Trotz der Exception können folgende Transient Instructions die Daten des initialen Speicherzugriffs verwenden. An dieser Stelle wird zwischen zwei Varianten von RIDL unterschieden, die in der Art des initialen Speicherzugriffs voneinander abweichen. Lädt dieser Speicherzugriff eine einzelne Cachezeile ein, so werden die resultierenden Daten aus dem Line-Fill Buffer bereitgestellt. Lädt der Speicherzugriff jedoch von mehreren Cachezeilen, so werden die resultierenden Daten aus einem der Load Ports bereitgestellt. In beiden Fällen werden diese Daten ohne jegliche Prüfung bereitgestellt und unabhängig davon, ob sie aus dem angreifenden Prozess selber, aus anderen Prozessen des Systems oder sogar aus dem Betriebssystem-Kern stammen. [@ridl, Kap. VI]

<!-- - insgesamt: dadurch kann der angreifer über den line-fill buffer die daten von speicherzugriffen anderer prozesse auf dem gleichen physischen kern beobachten -->

Insgesamt erlaubt dies einem Angreifer, den gesamten Inhalt des Line-Fill Buffers und der Load Ports zu lesen. Da der Line-Fill Buffer und die Load Ports Teil eines physischen Kerns sind und von logischen Kernen gemeinsam verwendet werden, kann ein Angreifer folglich die Daten von Speicherzugriffen anderer Prozesse auf dem gleichen physischen Kern beobachten. [@ridl, Kap.\ VI]

<!-- - implementierung:
- nutze normal flush+reload, wie oben beschrieben (Cache als Übertragungsmedium)
- nutze unmapped page um page fault ohne segmentation fault signal auszulösen
  - alternative: signal handling oder suppression, wie oben beschrieben
- initialer speicherzugriff lädt ein byte aus dem angegriffenen puffer (lfb oder lp), überträgt in architekturellen zustand über den cache, wie oben beschrieben -->

Die im Rahmen dieser Arbeit erstellte Implementierung von RIDL nutzt den L1d Cache als Übertragungskanal, wie in [@sec:impl-teile-cache] beschrieben. Der initiale Speicherzugriff greift auf eine ausgelagerte Page zu. Auf diese Weise wird eine Page-Fault Exception ausgelöst, ohne dass das Betriebssystem dem angreifenden Prozess anschließend ein Signal schickt. Alternativ kann der initiale Speicherzugriff auf eine virtuelle Adresse ohne Zuordnung zu physischem Speicher erfolgen. In diesem Fall muss die auftretende Exception behandelt oder unterdrückt werden, wie in [@sec:impl-teile-exception] beschrieben.

<!-- - beispielcode in [@lst:impl-ridl] abgedruckt
- löst den fehler aus, extrahiert ein byte an daten aus dem lfb und kodiert dieses in den cache -->

[@lst:impl-ridl] zeigt eine stark vereinfachte Implementierung von RIDL. Diese bereitet den Angriff vor, extrahiert ein Byte aus dem Line-Fill Buffer und überträgt es durch den Cache in den architekturellen Zustand.

```{.c float=tbp #lst:impl-ridl}
// Vorbereitung
uint8_t *target = alloc_page();
evict_page(target);

// Angriff
uint8_t leak = *target;
encode_in_cache(leak);

// Rekonstruktion
uint8_t data = decode_from_cache();
```

: Vereinfachte Implementierung von RIDL.

<!-- - beide varianten evaluiert, um cross-thread daten aus einem anderen prozess zu leaken -->

In [@sec:eval-ridl] werden beide Varianten von RIDL evaluiert vor dem Hintergrund Cross-Thread Daten aus einem anderen Prozess zu laden.

### Fallout

<!-- - das fallout paper beschreibt zwei angriffe
- wtf, das aus es ermöglicht daten aus dem store buffer zu lesen
- fetch+bounce, das es ermöglicht die anwesenheit von speicherzuordnungen an bestimmten virtuellen adressen festzustellen -->

Unter dem Begriff *Fallout* werden zwei unterschiedliche Angriffe zusammengefasst. Der erste dieser Angriffe ist *Write Transient Forwarding*, kurz *WTF*, der es ermöglicht Daten aus dem Store Buffer zu lesen [@fallout, Kap. 3.1]. Bei dem zweiten Angriff handelt es sich um *Store-to-Leak*, der es ermöglicht, die Anwesenheit von Speicherzuordnungen an gewählten virtuellen Adressen festzustellen [@fallout, Kap. 3.2].

<!-- - beide basieren auf der optimierung *store-to-load forwarding*
- diese optimiert lesende speicherzugriffe auf adressen, in die kürzlich geschrieben wurde
- speicherzugriff kann daten direkt aus dem store buffer nehmen, anstatt tatsächlich auf den speicher zuzugreifen -->

Beide Angriffe basieren auf einer Optimierung, die *Store-To-Load Forwarding* genannt wird. Diese optimiert lesende Speicherzugriffe auf Adressen, in die kürzlich geschrieben wurde: Der lesende Speicherzugriff lädt die Daten direkt aus dem Store Buffer (siehe [@sec:grundlagen-mikroarch-memory]) Eintrag des vorhergehenden schreibenden Speicherzugriffs. Auf diese Weise muss der lesende Speicherzugriff nicht tatsächlich auf den Speicher zugreifen. [@fallout, Kap. 2.3]

#### Write Transient Forwarding {#impl-angriffe-wtf}

<!-- - ausgelöste exception: general protection fault oder page fault
- angegriffenes element: store buffer -->

Write Transient Forwarding ist ein Spectre-Angriff basierend auf Prozessor-Exceptions (siehe [@sec:impl-spectre-exception]). Er verwendet unter anderem General Protection oder Page-Fault Exceptions, um unberechtigt auf Daten aus dem Store Buffer zuzugreifen [@fallout, Kap. 3.1, 5.2].

<!-- - angriff basiert darauf, dass store-to-load forwarding fälschlicherweise passiert
- wenn einige niedrige bits der virtuellen adresse mit einem eintrag des store buffers übereinstimmen
- und die adressübersetzung des load fehlschlägt, z.B. wenn der load eine general protection fault durch non-canon adress oder page fault durch supervisor auslöst
- load wird also diese daten den folgenden transient instructions zur verfügung stellen
- dies erlaubt es einem angreifer, auf den gesamten inhalt des store buffers zuzugreifen
Das Auslösen des Store-to-Load Forwarding führt dann dazu, dass der lesende Speicherzugriff folgenden Transient Instructions Daten aus dem Store Buffer zur Verfügung stellt. -->

Write Transient Forwarding basiert darauf, dass Store-to-Load Forwarding fälschlicherweise ausgelöst wird, wenn die Adressübersetzung des lesenden Speicherzugriffs fehlschlägt. Dies passiert beispielsweise, wenn der Speicherzugriff eine General Protection Exception durch Verwendung einer nicht-kanonischen Adresse oder eine Page-Fault Exception durch Verwendung einer Adresse des Betriebssystem-Kerns verursacht. Wie in [@sec:grundlagen-mikroarch-transient] beschrieben, löst diese Exception außerdem eine Transient Execution aus. Aufgrund des Store-to-Load Forwarding nimmt der lesende Speicherzugriff Daten aus dem Store Buffer auf. Diese Daten werden folgenden Transient Instructions zur Verfügung gestellt. [@fallout, Kap. 5.2]

Insgesamt erlaubt dies einem Angreifer, den gesamten Inhalt des Store Buffers zu lesen. Da der Store Buffer Teil eines physischen Kerns ist und von logischen Kernen gemeinsam verwendet wird, kann ein Angreifer folglich die Daten von schreibenden Speicherzugriffen anderer Prozesse auf dem gleichen physischen Kern beobachten. [@fallout, Kap. 5.2]

<!-- Dies passiert, wenn zwei Bedingungen erfüllt sind.
Einerseits müssen die virtuellen Adressen des lesenden und des schreibenden Zugriffes in einer festen Anzahl niederwertigster Bits übereinstimmen.
Andererseits muss die Adressübersetzung
Dies passiert, wenn eine feste Anzahl der niederwertigsten Bits von der virtuellen Adresse -->

<!-- - implementierung:
- nutze normal flush+reload, wie oben beschrieben (Cache als Übertragungsmedium)
- initialer load (phase 2 und 3) dereferenziert non-canon address oder alternativ supervisor address
- die entstehende exception wird durch signal handling oder suppression behandelt, siehe oben
- erlaubt einem angreifer den inhalt des store-buffers zu lesen
Auf diese Weise kann ein Angreifer den gesamten Inhalt des Store Buffers lesen. -->

Die im Rahmen dieser Arbeit erstellte Implementierung des Write Transient Forwarding nutzt den L1d Cache als Übertragungskanal, wie in [@sec:impl-teile-cache] beschrieben. Der initiale lesende Speicherzugriff dereferenziert eine nicht-kanonische Adresse. Alternativ kann eine Adresse des Betriebssystem-Kerns verwendet werden. Die dabei auftretende Prozessor-Exception muss behandelt oder unterdrückt werden. Dies erfolgt mit einer der in [@sec:impl-teile-exception] beschriebenen Techniken.

[@lst:impl-wtf] zeigt eine stark vereinfachte Implementierung von Write Transient Forwarding. Diese extrahiert ein Byte aus dem Store Buffer und überträgt es durch den Cache in den architekturellen Zustand.

```{.c float=tbp #lst:impl-wtf}
// Angriff
uint8_t leak = *(uint8_t *)0x4141414141410000;
encode_in_cache(leak);

// Rekonstruktion
uint8_t data = decode_from_cache();
```

: Vereinfachte Implementierung von Write Transient Forwarding.

<!-- - evaluiert, um cross-thread daten aus einem anderen prozess zu leaken -->

In [@sec:eval-wtf] wird Write Transient Forwarding evaluiert und benutzt, um Cross-Thread Daten aus einem anderen Prozess zu laden.

#### Store-to-Leak {#impl-angriffe-stl}

<!-- - anders als andere hier beschriebene angriffe, da er keine daten aus einem der üblichen puffer der mikroarchitektur extrahiert
- benutzt stattdessen eine eigenschaft des store-to-load forwarding, um die anwesenheit von einträgen im TLB zu beobachten -->

Store-to-Leak unterscheidet sich von den bisher beschriebenen Angriffen, da er keine Daten aus einem der üblichen Puffer der Mikroarchitektur extrahiert. Stattdessen benutzt Store-to-Leak eine Eigenschaft des Store-to-Load Forwarding, um die Anwesenheit von Einträgen im TLB zu beobachten. [@fallout, Kap. 3.2]

<!-- - STL erfolgt nur, wenn es zu der virtuellen adresse einen validen eintrag im TLB gibt
- außerdem erfolgt STL auch bei zugriffen auf speicher des kernels
- in diesem fall löst der schreibende zugriff eine exception aus, dennoch kann in der folgenden transient execution erkannt werden, ob STL stattgefunden hat oder nicht
- angreifer kann folglich herausfinden, ob für eine gegebene kernel-adresse ein TLB eintrag besteht
- durch vorherigen speicherzugriff des angreifers kann sichergestellt werden dass ein TLB eintrag existiert, falls das mapping valide ist -->

Store-to-Load Forwarding erfolgt nur, wenn zu der virtuellen Adresse des lesenden Speicherzugriffs ein valider Eintrag im TLB existiert. Zusätzlich erfolgt Store-to-Load Forwarding auch bei Zugriffen auf den Speicherbereich den Betriebssystem-Kerns. In diesem Fall löst ein schreibender Speicherzugriff eine Prozessor-Exception aus. In der folgenden Transient Execution kann ein lesender Speicherzugriff durchgeführt und anschließend erkannt werden, ob zwischen den beiden Zugriffen Store-to-Load Forwarding stattgefunden hat. Ein Angreifer kann folglich herausfinden, ob für eine gewählte virtuelle Adresse des Betriebssystem-Kerns ein TLB Eintrag existiert. Durch einen vorhergehenden Speicherzugriff kann außerdem sichergestellt werden, dass ein solcher TLB Eintrag existiert, falls der gewählten virtuellen Adresse physischer Speicher zugeordnet ist. [@fallout, Kap.\ 3.2,\ 5.3]

<!-- - implementierung:
- nutze cache sidechannel, aber beobachte anders als die anderen angriffe nur eine cachezeile
- detektiere ob memory mapping an einer bestimmten adresse besteht:
  - schreibe 0 an diese adresse (löst zugriffsfehler aus, da kernel-adresse; rest in transient execution)
  - lade von dieser adresse, kodiere geladenes byte in den cache
  - valides mapping (TLB eintrag) -> load nimmt direkt die gespeicherten daten (store-to-load forwarding)
  - kein TLB eintrag -> kein forwarding, zustand des cache bleibt unverändert
  - exception per handling behandeln
  - cache sidechannel um festzustellen ob eintrag im cache liegt
- nutze dies um adressen von 0xffffffff80000000 bis 0xffffffffc0000000 in abständen von 0x100000 durchzuprobieren, bis kernel mapping gefunden wurde
- leak KASLR -->

Die im Rahmen dieser Arbeit erstellte Implementierung von Store-to-Leak nutzt den L1d Cache als Übertragungskanal, wie in [@sec:impl-teile-cache] beschrieben. Um zu die Anwesenheit einer Speicherzuordnung an einer gewählten virtuellen Adresse zu detektieren, wird wie folgt vorgegangen. Zunächst wird ein fest gewählter Wert an diese Adresse geschrieben. Da es sich um eine Adresse aus dem Bereich des Betriebssystem-Kerns handelt, löst dies eine Prozessor-Exception aus. Innerhalb der folgenden Transient Execution wird ein Wert von der gewählten Adresse geladen und in den Cache-basierten Übertragungskanal kodiert. Existiert ein valider TLB Eintrag für die gewählte Adresse, wird Store-to-Load Forwarding ausgelöst. In diesem Fall wird der zuvor geschriebene Wert in den Cache kodiert. Existiert hingegen kein valider TLB Eintrag, so geschieht kein Store-to-Load Forwarding und es wird kein Wert in den Cache kodiert. Die auftretende Prozessor-Exception wird mit einer der Techniken aus [@sec:impl-teile-exception] behandelt oder unterdrückt. Auf diese Weise lässt sich die Anwesenheit eines validen TLB Eintrages für die gewählte virtuelle Adresse detektieren. Um die Anwesenheit einer Speicherzuordnung zu beobachten, kann diese Technik zweimal hintereinander ausgeführt werden. [@fallout, Kap. 3.2, 5.3]

<!-- Die Implementierung von Store-to-Leak nutzt den Cache als Übertragungskanal. Anders als bei den übrigen Angriffe wird dabei nur eine Cachezeile verwendet.
Um zu die Anwesenheit einer Speicherzuordnung an einer gewählten virtuellen Adresse zu detektieren, wird wie folgt vorgegangen.
Zunächst wird ein fest gewählter Wert an diese Adresse geschrieben. Da es sich um eine Adresse aus dem Bereich des Betriebssystem-Kerns handelt, löst dies eine Prozessor-Exception aus. Die nächsten Schritte geschehen innerhalb einer Transient Execution.
Anschließend wird ein Wert von der gewählten Adresse geladen. Existiert ein valider TLB Eintrag für die gewählte Adresse, wird Store-to-Load Forwarding ausgelöst und der lesende Zugriff nimmt den zuvor geschriebenen Wert auf.
Existiert hingegen kein valider TLB Eintrag, so nimmt der lesende Zugriff einen unabhängigen Wert auf. -->

[@lst:impl-stl] zeigt eine stark vereinfachte Implementierung von Store-to-Leak. Diese schreibt ein festes Byte an die anvisierte Adresse, liest ein Byte von dieser Adresse und überträgt das gelesene Byte durch den Cache in den architekturellen Zustand. Stimmt das gelesene mit dem geschriebenen Byte überein, so deutet dies auf die Existenz eines TLB Eintrages für die anvisierte Adresse hin.

```{.c float=tbp #lst:impl-stl}
// Angriff
volatile uint8_t *target = TARGET_ADDR;
*target = 0x42;
uint8_t leak = *target;
encode_in_cache(leak);

// Rekonstruktion
uint8_t data = decode_from_cache();
```

: Vereinfachte Implementierung von Store-to-Leak.

<!-- - evaluiert, um kaslr zu brechen -->

In [@sec:eval-stl] wird Store-to-Leak evaluiert und verwendet, um die virtuelle Adresse des Betriebssystem-Kerns zu bestimmen und dadurch KASLR zu brechen.

### ZombieLoad {#impl-angriffe-zombieload}

<!-- - ausgelöste exception: microcode assist
- angegriffenes element: line-fill buffer -->

*ZombieLoad* ist ein Spectre-Angriff basierend auf Prozessor-Exceptions (siehe [@sec:impl-spectre-exception]). ZombieLoad verwendet Microcode Assists, um unberechtigt auf Daten aus dem Line-Fill Buffer zuzugreifen. [@zombieload, Kap. 3.1]

<!-- - wenn ein lesender speicherzugriff einen microcode assist erfordert, lädt dieser zunächst fälschlicherweise veraltete daten aus dem line-fill buffer
- nach dem die durch den assist ausgelöste transient execution beendet ist, wird der speicherzugriff erneut getätigt
- dabei löst er keinen assist mehr aus und lädt die korrekten daten ein
- wie bei anderen angriffen kann die transient execution genutzt werden, um die fälschlicherweise geladenen daten in den cache zu kodieren
- (da der microcode assist keine architekturell sichtbare exception auslöst, muss auch keine exception behandelt werden) -->

Wenn ein lesender Speicherzugriff einen Microcode Assist erfordert, lädt dieser zunächst fälschlicherweise veraltete Daten aus dem Line-Fill Buffer. Nachdem die durch den Microcode Assist ausgelöste Transient Execution beendet ist, wird der Speicherzugriff erneut getätigt. Dabei löst er keinen Microcode Assist mehr aus und lädt die korrekten Daten ein. Wie in [@sec:impl-spectre-exception] dargestellt, kann die Transient Execution genutzt werden, um die fälschlicherweise geladenen Daten in einen Cache-basierten Übertragungskanal zu kodieren. [@zombieload, Kap. 3.2]

Insgesamt erlaubt dies einem Angreifer, den gesamten Inhalt des Line-Fill Buffers zu lesen. Da der Line-Fill Buffer Teil eines physischen Kerns ist und von logischen Kernen gemeinsam verwendet wird, kann ein Angreifer folglich die Daten von Speicherzugriffen anderer Prozesse auf dem gleichen physischen Kern beobachten. [@zombieload, Kap. 3.2]

<!-- - "microarchitectural faults": faults that cause a memory request to be re-issued internally without becoming architecturally visible (actually assists)
- When Load requires microcode assist, it reads stale values before being re-issued
- stale-entry hypothesis:
  - assist triggers machine clear -> pipeline flush
  - instructions in flight still finish execution
  - load continues with wrong fill-buffer entry from a sibling core
  - load loads valid data from a previous load or store -->

<!-- - paper beschreibt drei varianten
- variante 1:
  - benötigt page, die in den adressraum des nutzer-prozesses und in den adressraum des kernels eingebunden ist
  - der angreifer invalidiert nun eine cachezeile der page über die virtuelle adresse des nutzer-prozesses
  - unmittelbar danach greift der angreifer lesend auf diese cachezeile zu, aber über die adresse des kernels
  - (da dieser speicherzugriff unberechtigt erfolgt, löst er eine exception aus)
  - dieser zugriff führt zu einem microcode assist und lädt wie oben beschrieben zunächst daten aus dem line-fill buffer, die vom angreifer extrahiert werden können -->

#### Varianten von ZombieLoad

![Situation und Vorgehen eines Angriffs auf die erste ZombieLoad-Variante.](figs/zombieload.pdf){#fig:impl-zombieload}

Es werden drei Varianten von ZombieLoad unterschieden. Um einen Angriff der ersten Variante durchzuführen, benötigt ein Angreifer eine Page, die in den Adressbereich des Nutzer-Prozesses und in den Adressbereich des Betriebssystem-Kerns eingebunden ist. Diese Situation ist in [@fig:impl-zombieload] dargestellt. Da Linux über eine Direct-Physical Map verfügt, ist diese Voraussetzung auf Linux für jede Page des Nutzer-Prozesses erfüllt (siehe [@sec:grundlagen-speicher-adressbereich]). Der Angreifer invalidiert nun eine Cachezeile der gewählten Page über die zugehörige virtuelle Adresse im Bereich des Nutzer-Prozesses. Unmittelbar danach greift der Angreifer lesend auf diese Cachezeile zu. Der Zugriff erfolgt dabei über die zugehörige virtuelle Adresse im Bereich des Betriebssystem-Kerns. Auf diese Weise durchgeführt, löst der lesende Zugriff einen Microcode Assist aus und lädt wie bereits beschrieben zunächst Daten aus dem Line-Fill Buffer. Diese Daten können über einen Cache-basierten Übertragungskanal extrahiert werden. [@zombieload, Kap. 5.1]

<!-- - variante 2:
  - führt einen ladenden speicherzugriff innerhalb einer Intel TSX Transaktion durch
  - gleichzeitig schreibt der angreifer von einem anderen logischen kern aus in die gleiche adresse
  - dadurch wird die transaktion abgebrochen, und der ladende speicherzugriff in der transaktion lädt fälschlicherweise aus dem line-fill buffer -->

Bei einem Angriff der zweiten Variante führt ein Angreifer einen ladenden Speicherzugriff innerhalb einer Intel TSX Transaktion durch. Gleichzeitig schreibt der Angreifer von einem anderen logischen Kern aus an die gleiche Adresse. Dies führt zu einem Konflikt in dem *Read Set* der Transaktion und damit zu einem Abbruch der Transaktion. Der Effekt dieses Abbruches ist ähnlich zu dem eines Microcode Assists. Insbesondere lädt der Speicherzugriff innerhalb der abbrechenden Transaktion fälschlicherweise aus dem Line-Fill Buffer. Die geladenen Daten können über einen Cache-basierten Übertragungskanal extrahiert werden. [@zombieload, Kap. 5.1]

<!-- - variante 3:
  - löst Adressübersetzung aus, die bestimmte Bits in einer der Zuordnungstabellen ändert
  - diese Änderung wird durch einen Microcode Assist durchgeführt
  - löst ein lesender Speicherzugriff solch eine Adressübersetzung aus, so lädt dieser wie oben beschrieben zunächst Daten aus dem Line-Fill Buffer -->

Ein Angriff der dritten Variante löst eine Adressübersetzung aus, die bestimmte Bits in einer der Zuordnungstabellen (siehe [@sec:grundlagen-speicher]) ändert. Diese Änderung wird durch einen Microcode Assist durchgeführt. Löst also ein lesender Speicherzugriff eine solche Adressübersetzung aus, so lädt dieser wie bereits beschrieben zunächst Daten aus dem Line-Fill Buffer. Diese Daten können über einen Cache-basierten Übertragungskanal extrahiert werden. [@zombieload, Kap. 5.1]

<!-- - implementierung variante 1:
- nutze normal flush+reload, wie oben beschrieben (Cache als Übertragungsmedium)
- allokiere eine page, ermittele adresse dieser page im direct mapping (siehe xxx), (in dieser implementierung über /proc/self/pagemap, auf das standardmäßig nur privilegierte nutzer zugriff haben, kann aber auch über anderen side channel ermittelt werden, z.B. fetch+bounce)
- haben damit virtuelle adressen im user- und im kernel-bereich, denen der gleiche physische speicher zugeordnet ist
- invalidiere die erste cachezeile durch die user-adresse, lade von der kernel-adresse
- folgende transient instructions haben zugriff auf daten, die aus dem line-fill buffer geladen wurden
- da zugriff auf kernel-adresse unberechtigt erfolgt, wird ein signal an den prozess gesendet. behandle das mit technik aus signal handling
- erlaubt dem angreifer inhalt des line-fill buffers zu lesen -->

#### Zur Implementierung von ZombieLoad

Die im Rahmen dieser Arbeit erstellte Implementierung der ersten ZombieLoad Variante nutzt den L1d Cache als Übertragungskanal, wie in [@sec:impl-teile-cache] beschrieben. Zunächst wird eine Page im Adressbereich des Nutzer-Prozesses allokiert. Anschließend wird die Adresse dieser Page in der Direct-Physical Map des Betriebssystem-Kerns ermittelt. Diese setzt sich aus der Basisadresse der Direct-Physical Map und der physischen Adresse der allokierten Page zusammen. In dieser Implementierung wird eine feste Adresse der Direct-Physical Map vorausgesetzt. Die physische Adresse der allokierten Page wird über die Datei `/proc/self/pagemap` ermittelt, auf die standardmäßig nur privilegierte Nutzer zugreifen können. Alternativ können die benötigten Adressen durch verschiedene Seitenkanäle ermittelt werden [@fallout, Kap. 6.1] [@attack-kaslr]. Damit ist eine virtuelle Adresse im Bereich des Nutzer-Prozesses und eine virtuelle Adresse im Bereich des Betriebssystem-Kerns bekannt, denen die gleiche physische Adresse zugeordnet ist. Die Voraussetzungen für die erste ZombieLoad Variante sind also gegeben und der Angriff kann wie bereits beschrieben durchgeführt werden. Da dabei ein unberechtigter Zugriff auf eine Adresse des Betriebssystem-Kerns erfolgt, tritt eine Prozessor-Exception auf. Diese wird wie in [@sec:impl-teile-exception] beschrieben behandelt oder unterdrückt.

[@lst:impl-zombieload] zeigt eine stark vereinfachte Implementierung von ZombieLoad. Diese bereitet den Angriff vor, extrahiert ein Byte aus dem Line-Fill Buffer und überträgt es durch den Cache in den architekturellen Zustand.

```{.c float=tbp #lst:impl-zombieload}
// Vorbereitung
uint8_t *target = alloc_page();
uint8_t *target_kernel = get_kernel_mapping(target);

// Angriff
_mm_clflush(target);
uint8_t leak = *target_kernel;
encode_in_cache(leak);

// Rekonstruktion
uint8_t data = decode_from_cache();
```

: Vereinfachte Implementierung von ZombieLoad.

<!-- - variante 2 braucht tsx
- variante 3 lässt sich nicht von linux userspace aus durchführen
- daher beide hier nicht weiter betrachtet -->

Eine Implementierung der zweiten ZombieLoad Variante setzt Unterstützung für die Intel TSX Prozessorerweiterungen voraus. Ein Angriff der dritten Variante lässt sich nicht von unprivilegierten Nutzerprozessen auf Linux-Systemen durchführen. Aus diesen Gründen werden beide Varianten in dieser Arbeit nicht implementiert (siehe [@sec:impl-szenario]).

<!-- - variante 1 evaluiert, um cross-thread aus anderem prozess zu leaken -->

In [@sec:eval-zombieload] wird die erste ZombieLoad Variante verwendet, um Cross-Thread Daten aus einem anderen Prozess zu laden, und daran evaluiert.

### Zusammenfassung

In diesem Kapitel wurden die Funktionsweise und Details der Implementierung ausgewählter konkreter Spectre-Angriffe beschrieben. Bei den Angriffen handelt es sich um RIDL, Write Transient Forwarding, Store-to-Leak und ZombieLoad. RIDL verwendet Page-Fault Exceptions, um unberechtigt auf Daten aus dem Line-Fill Buffer oder den Load Ports zuzugreifen. Write Transient Forwarding verwendet unter anderem General Protection oder Page-Fault Exceptions, um unberechtigt auf Daten aus dem Store Buffer zuzugreifen. Store-to-Leak benutzt eine Eigenschaft des Store-to-Load Forwarding, um die Anwesenheit von Einträgen im TLB zu beobachten. ZombieLoad verwendet Microcode Assists, um unberechtigt auf Daten aus dem Line-Fill Buffer zuzugreifen. Diese Spectre-Angriffe werden in [@sec:eval-angriffe] und [@sec:eval-stl] hinsichtlich einheitlicher Metriken evaluiert.
