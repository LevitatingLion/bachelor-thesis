# Grundlagen {#sec:grundlagen}

<!-- - Intro?
- Folgendes Kapitel beschreibt notwendige Grundlagen zur Funktionsweise moderner CPUs
- Jeweils erst allgemein, dann konkret für moderne Intel CPUs
- (jeweils: Bedeutung / warum wichtig, wann wird das beschrieben)
- Speicherorganisation / physischer und virtueller Speicher -> Prozessisolation, Grundlage separater Berechtigungen, dadurch Einfluss auf viele andere Elemente
- Caches -> verbergen Latenz des DRAM, sehr wichtig für Performance, Grundlage der meisten Side-Channel Angriffe
- Prozessororganisation -> verbindet die vorher beschriebenen Elemente, beschreibt wie Instruktionen Prozessorintern umgesetzt und ausgeführt werden, enthält Quelle der Spectre-Lücken -->

Das folgende Kapitel behandelt die Grundlagen der Funktionsweise moderner Prozessoren, die für ein Verständnis von Spectre-Angriffen benötigt werden. Zunächst wird in [@sec:grundlagen-speicher] das Konzept des virtuellen Speichers beschrieben. Virtueller Speicher ermöglicht unter anderem die Isolation von Prozessen, welche Voraussetzung für weitere Zugriffskontrollmechanismen ist. Anschließend wird in [@sec:grundlagen-caches] der Aufbau und die Funktionsweise von CPU-Caches dargestellt. CPU-Caches sind wichtig für die Performance moderner Prozessoren, da sie die Latenz des Hauptspeichers verbergen. Außerdem bilden sie die Basis der meisten Seitenkanalangriffe. Schließlich werden in [@sec:grundlagen-mikroarch] die wichtigsten Elemente der Prozessororganisation moderner Intel Prozessoren thematisiert. Dies beinhaltet, wie Instruktionen verarbeitet und ausgeführt werden und enthält die Quelle der Spectre-Lücken.

## Physischer und virtueller Speicher {#sec:grundlagen-speicher}

<!-- - Idee: virtueller Adressraum bildet physischen Adressraum in unterschiedlichen Prozessen unterschiedlich ab, ermöglicht Isolation von Prozessen
- Auf 64-bit Systemen normalerweise 48-bit Adressen
- Virtueller Adressraum (und physischer auch) unterteilt in Pages, verschiedene Pagegrößen (1-4KB für kleine Pages, bis zu ~GB, immer Zweierpotenz) -->

Moderne Prozessoren isolieren Prozesse voneinander dadurch, dass jeder Prozess seinen eigenen virtuellen Adressraum erhält. Einem Teil der virtuellen Adressen wird jeweils eine physische Adresse zugeordnet. Diese Zuordnung wird von der *Memory Management Unit* (MMU) durchgeführt. Sie erfolgt nicht byteweise, sondern nur in größeren Blöcken, genannt *Pages*. Viele Systeme unterstützen verschiedene Page-Größen, wobei kleine Pages von 4 KiB und große Pages von bis zu 1 GiB üblich sind [@intel-sdm-3, Kap. 4, Tab. 1]. 64-Bit Systeme adressieren den virtuellen Adressraum üblicherweise mit 48 Bit. [@gruss-thesis, S. 15]

<!-- - (Basis jeglicher Zugriffskontrollmechanismen) -->

<!-- - Da das Mapping von virtuellen zu physischen Adressen sehr partiell ist: mehrere Level an page-translation Tabellen
- Spezielles Register enthält Adresse der obersten Tabelle (auf x86: CR3), Tabellen sind im physischen Speicher enthalten -->

Eine vollständige Zuordnungstabelle zu speichern, welche jeder virtuellen Page eine physische Page zuordnet, würde bei 64 Bits pro Eintrag 512 GiB an Speicher benötigen. Da so eine Zuordnungstabelle in allen praktischen Fällen nur sehr spärlich besetzt ist, lassen sich diese Platzkosten jedoch wesentlich reduzieren. Auf modernen Prozessoren wird dies mithilfe mehrerer Ebenen an Zuordnungstabellen erreicht. Jeder Eintrag einer Zuordnungstabelle verweist dabei entweder auf eine Tabelle der folgenden Ebene oder zeigt an, dass diese Tabelle vollständig leer ist [@intel-sdm-3, S. 4-7]. Die Zuordnungstabellen werden im physischen Speicher abgelegt. Ein spezielles Prozessorregister enthält die (physische) Adresse der obersten Tabelle. In x86 Prozessoren ist dies das `CR3`-Register [@intel-sdm-3, Kap. 4, S. 7].

<!-- - Da Adressübersetzung sehr häufig passiert (bei jedem Speicherzugriff): cache Ergebnisse in TLB -->

Die Übersetzung von virtuellen zu physischen Adressen muss bei jedem Speicherzugriff durchgeführt werden. Dass diese Übersetzung schnell erfolgt, ist daher relevant für die Performance des Prozessors. Um eine schnelle Übersetzung zu gewährleisten, werden kürzlich ermittelte Zuordnungen in einem dedizierten Cache (siehe [@sec:grundlagen-caches]) vorgehalten, der oft *Translation Lookaside Buffer* (TLB) genannt wird [@intel-sdm-3, Kap. 4, S. 39].

### Zuordnungstabellen in modernen Intel Prozessoren

<!-- - In modernen Intel CPUs:
  - 4 Level, jeweils 512 Einträge, Indices sind jeweils 9 bit der virtuellen Adresse
  - Unterschiedliche Page Größen durch unterschiedliche Tiefe der Translation
  - Normalerweise rekursiv??? ist egal
  - Bild von x86 page tables??? nein -->

In modernen x86-64 Prozessoren werden die Zuordnungstabellen in vier Ebenen angeordnet. Jede Tabelle hat 512 Einträge. Die obersten 36 Bits der zu übersetzenden virtuellen Adresse sind in vier Abschnitte von je 9 Bit unterteilt. Jeder dieser Abschnitte gibt den Index in eine der Zuordnungstabellen an. Mithilfe dieser Indices wird in der Tabelle der letzten Ebene eine 4 KiB Page ermittelt. Die verbleibenden, niederwertigsten 12 Bit der virtuellen Adresse verweisen auf ein Byte innerhalb dieser Page. Die vier Ebenen der Zuordnungstabellen und die Unterteilung virtueller Adressen sind in [@fig:grundlagen-pagetables] veranschaulicht. [@intel-sdm-3, Kap. 4, S. 7]

![Zuordnungstabellen und Adressübersetzung in modernen x86-64 Prozessoren. [@gruss-habil, Abb. 2.4]](figs/page_tables.pdf){#fig:grundlagen-pagetables}

Anstelle eine folgende Zuordnungstabelle anzugeben, kann ein Tabelleneintrag auch direkt die physische Adresse einer Page enthalten. In diesem Fall verweisen die verbleibenden Bits der virtuellen Adresse auf ein Byte innerhalb der ermittelten großen Page. Auf diese Weise werden unterschiedliche Page-Größen implementiert. [@intel-sdm-3, Kap. 4, S. 7]

### Adressräume in modernen Betriebssystemen {#sec:grundlagen-speicher-adressbereich}

<!-- - adressraum fest aufgeteilt in teil für kernel des betriebssystems und nutzer-prozesse
- konkret: user 0x0000000000000000 bis 0x00007fffffffffff, kernel 0xffff800000000000 bis 0xffffffffffffffff
- zusätzlich wird ein teil des adressraums des kernel auf den gesamten physischen speicher des systems abgebildet
- wird "direct mapping" oder "linear mapping" genannt
- z.B. auf linux und osx -->

Auf modernen Betriebssystemen in x86-64 Prozessoren ist der virtuelle Adressraum üblicherweise fest in zwei Teile unterteilt. Die höheren Adressen sind dabei für den Kern des Betriebssystems reserviert und die niedrigeren Adressen für Nutzer-Prozesse. Zusätzlich wird auf manchen Betriebssystemen ein Teil des Adressraums des Kerns auf den gesamten physischen Speicher des Systems abgebildet. Diese Speicherzuordnung wird *Direct-Physical Map* genannt. Sie vereinfacht den Zugriff des Betriebssystem-Kerns auf den physischen Speicher. Eine Direct-Physical Map wird beispielsweise in den Kernen von Linux und OS X eingesetzt. [@meltdown, Kap. 2.2]

<!-- - ASLR/KASLR
- um das ausnutzen von sicherheitslücken zu erschweren, werden die virtuellen adressen der meisten zuordnungen randomisiert
- sowohl bei nutzer-prozessen als auch im kernel
- nutzer: aslr, kernel: kaslr -->

Um das Ausnutzen von Sicherheitslücken in Programmen zu erschweren, werden die virtuellen Adressen der meisten Zuordnungen randomisiert. Diese Technik wird *Address Space Layout Randomization*, kurz *ASLR* genannt. Sie kommt sowohl bei Nutzer-Prozessen als auch im Betriebssystem-Kern zum Einsatz. Im Betriebssystem-Kern wird sie auch als *Kernel Address Space Layout Randomization* oder *KASLR* bezeichnet. Aktuelle Linux-Systeme auf x86-64 Prozessoren verwenden ASLR in Nutzer-Prozessen mit einer Entropie von 28 Bits [@linux-doc-vm, mmap_rnd_bits] [@linux-x86config, Z. 265] und KASLR mit einer Entropie von 9 Bits [@attack-kaslr, Tab. 1]. [@fallout, Kap. 2.5]

<!-- - für aktuelles linux
- user ASLR: 28 Bits [@linux-doc-vm, mmap_rnd_bits] [@linux-x86config, Z. 265]
- KASLR: 9 Bits [@attack-kaslr, Tabelle 1] -->

## Caches {#sec:grundlagen-caches}

<!-- - Allgemein: sollen Latenz des DRAM verbergen, alle Zugriffe gehen durch den Cache -> Cache hit oder cache miss
- Überblick über Abschnitt -->

Die Latenz von Zugriffen auf den Hauptspeicher ist sehr hoch im Vergleich zu der Ausführungszeit von Instruktionen in modernen Prozessoren. Ein Zugriff auf den Hauptspeicher hat üblicherweise eine Latenz von mehreren hundert Prozessorzyklen [@gruss-habil, S. 23]. Um diese Latenz zu verbergen, existieren zusätzliche Speicher, genannt *Caches*. Caches haben eine wesentlich geringere Speicherkapazität, sind aber auch wesentlich schneller als der Hauptspeicher. Üblich sind Unterschiede zwischen Caches und Hauptspeicher von bis zu 6 Größenordnungen in der Kapazität [@intel-opt, Kap. 2, S. 20] und 2 Größenordnungen in der Latenz [@gruss-habil, S. 23]. Jeder Speicherzugriff des Prozessors erfolgt zunächst auf den Cache. Wenn die angeforderten Daten im Cache vorhanden sind, können sie direkt aus dem Cache geladen werden. In diesem Fall wird von einem *Cache Hit* gesprochen. Sind die angeforderten Daten jedoch nicht im Cache vorhanden, müssen sie aus dem Hauptspeicher geladen werden und, es wird von einem *Cache Miss* gesprochen. [@gruss-thesis, S. 17]

<!-- - Annahme der Locality? -->

Die bei einem Cache Miss aus dem Hauptspeicher geladenen Daten werden anschließend im Cache platziert [@intel-sdm-3, Kap. 11, S. 5]. Dies basiert auf der Annahme, dass diese Daten zukünftig mit einer höheren Wahrscheinlichkeit angefragt werden als andere Daten. In welchem Cacheeintrag die Daten platziert werden, hängt vom Typ des Caches ab und wird im folgenden Abschnitt beschrieben. Anschließend wird dargestellt, auf welche Art und Weise Daten ersetzt werden, die bereits im Cache vorhanden sind.

<!-- - Einerseits Speicherinhalt/Nutzdaten (Inhalt des Hauptspeichers)
- Andererseits Adressübersetzung/TLBs
- Andere Buffer wie LB SB LFB BTB RSB PHT nicht beschrieben -->
<!-- Neben der Verwendung von Caches zur Speicherung von Nutzdaten aus dem Hauptspeicher -->

Caches in der hier beschriebenen Form werden in modernen Prozessoren üblicherweise für zwei Zwecke eingesetzt. Einerseits werden Caches verwendet, um Nutzdaten aus dem Hauptspeicher vorzuhalten. Andererseits werden Caches in TLBs eingesetzt, um die Adressübersetzung zu beschleunigen. Anderen Puffern, wie beispielsweise dem *Load Buffer*, *Line-Fill Buffer* oder *Branch Target Buffer*, liegt eine eigene Funktionsweise zugrunde. Solche Puffer werden daher in diesem Kapitel nicht weiter beschrieben.

Der Inhalt von Caches für Nutzdaten wird immer in Einheiten einer festen Größe verwaltet, sogenannten *Cachezeilen*. Auf modernen x86 Prozessoren beträgt die Größe einer Cachezeile 64 Byte [@intel-sdm-3, Kap. 11, S. 4]. TLBs speichern eine Zuordnung von virtuellen zu physischen Adressen auf Page-Ebene. In diesem Fall würde eine einzige derartige Zuordnung als Cachezeile bezeichnet, jedoch wird der Begriff der Cachezeile im Kontext von TLBs üblicherweise nicht verwendet.

### Typen von Caches

<!-- - verschiedene Möglichkeiten, Adressen auf Einträge des Caches zu verteilen
- kongruenz
- immer tag überprüfen, um sicher zu sein -->

Es gibt verschiedene Möglichkeiten, Speicheradressen auf die Einträge des Caches zu verteilen. Zwei Speicheradressen, denen der gleiche Cacheeintrag zugeordnet wird, werden *kongruent* genannt [@gruss-thesis, S. 18]. Die Daten in einem Cacheeintrag können aus jeder der kongruenten Cachezeilen stammen. Um diese Daten einer eindeutigen Cachezeile zuordnen zu können, wird daher im Cacheeintrag zusätzlich ein sogenannter *Tag* gespeichert [@gruss-thesis, S. 18--19]. Der Tag muss bei jedem lesenden Zugriff auf den Cache verglichen werden. Üblicherweise wird der Tag aus den höherwertigsten Bits der Adresse gebildet [@gruss-thesis, S. 18--19].

<!-- #### Set-Associative Caches -->

<!-- - set-associative: aufgeteilt in 2^n Cache Sets, n mittlere Bits der Adresse sind Index des Cache Sets, (Adresse aufgeteilt in niedrige Bits um innerhalb Cacheline zu adressieren, mittlere Bits als Index in Cache Set, hohe Bits für Berechnung des Tag) jedes Set hat m Ways die Daten aus m kongruenten Adressen gleichzeitig speichern können -->

![Ein 2-Way Set-Associative Cache. [@gruss-thesis, Abb. 2.4]](figs/cache-setassoc.pdf){#fig:grundlagen-cache-setassoc}

Ein häufig verwendeter Typ von Caches ist der *Set-Associative Cache*. [@fig:grundlagen-cache-setassoc] zeigt ein Beispiel eines solchen Caches. Set-Associative Caches werden in modernen Intel Prozessoren hauptsächlich verwendet (siehe [@sec:grundlagen-caches-modern]). Sie bestehen aus $2^n$ Cacheeinträgen, sogenannten *Cache Sets*. Bei einer Cachezeilen-Größe von $2^b$ Bytes werden die niederwertigsten $b$ Adressbits benutzt, um ein Byte innerhalb einer Cachezeile zu adressieren. Die $n$ folgenden Bits der Adresse werden als Index in den Cache benutzt, um das entsprechende Cache Set zu ermitteln. Die verbleibenden, höherwertigsten Bits der Adresse werden zur Berechnung des Tags verwendet. Außerdem enthält jedes Cache Set mehrere *Cache Ways*. Jeder Cache Way kann eine Cachezeile an Daten speichern. Auf diese Weise kann ein Cache Set die Daten aus mehreren kongruenten Cachezeilen gleichzeitig speichern. Ein Set-Associative Cache mit $m$ Cache Ways wird auch *$m$-Way Set-Associative Cache* genannt. [@gruss-thesis, S.\ 19]

<!-- werden auch noch directly-mapped und fully-associative unterschieden, das sind aber Spezialfälle von set-associative -->

Außerdem werden zwei weitere Typen von Caches unterschieden. Diese sind jedoch Spezialfälle des Set-Associative Cache. Der *Directly-Mapped Cache* entspricht einem Set-Associative Cache mit nur einem Cache Way pro Cache Set, also einem 1-Way Set-Associative Cache. Folglich kann dieser Cache stets nur eine aus mehreren kongruenten Cachezeilen speichern. Der *Fully-Associative Cache* entspricht einem Set-Associative Cache mit nur einem Cache Set, welches alle Cache Ways enthält. Also sind in diesem Cache sämtliche Cachezeilen kongruent.

### Adressierung

<!-- - physically/virtually-indexed/tagged -->

Die Speicheradresse fließt ein in die Berechnung des Cache Index und in die Berechnung des Tags. In beiden Fällen kann entweder die physische oder die virtuelle Adresse verwendet werden. In Prozessoren üblich sind die drei Varianten *virtually-indexed virtually-tagged* (VIVT), *physically-indexed physically-tagged* (PIPT) und *virtually-indexed physically-tagged* (VIPT) [@gruss-thesis, S. 22]. Diese werden im Folgenden beschrieben.

#### Virtually-Indexed Virtually-Tagged

<!-- - VIVT: geringe Latenz, aber Shared Memory ist nicht im Cache geshared -> cache invalidation bei context switch -->

VIVT Caches nutzen die virtuelle Adresse für den Cache Index und den Tag. Dies ermöglicht eine geringe Latenz bei Zugriffen auf den Cache, da keine Übersetzung in eine physische Adresse notwendig ist. Ein gemeinsamer Speicherbereich mehrerer Prozesse kann unterschiedliche Cache Indices haben. Dadurch kann ein erhöhter Speicherbedarf auftreten. Außerdem ist der Tag nur für einen virtuellen Adressraum gültig. Daher kann es erforderlich sein, bei einem Context Switch den Cache zu invalidieren. Da TLBs für die Adressübersetzung verwendet werden, werden in ihnen ausschließlich VIVT Caches eingesetzt. [@gruss-thesis, S. 22]
<!-- VIVT Caches werden beispielsweise in TLBs eingesetzt. Da diese für die Adressübersetzung verwendet werden, sind sie notwendigerweise virtually-indexed virtually-tagged. -->

#### Physically-Indexed Physically-Tagged

<!-- - PIPT: höhere Latenz, dafür keine Invalidation bei Context Switch notwendig -->

PIPT Caches nutzen die physische Adresse für den Cache Index und den Tag. Dadurch werden die Nachteile von VIVT Caches vermieden. Gemeinsamer Speicher wird immer auf den gleichen Cache Index abgebildet und alle Tags sind nach einem Context Switch gültig. Jedoch ist dies mit einer höheren Latenz verbunden, da eine Übersetzung der virtuellen in eine physische Adresse notwendig ist. PIPT Caches werden zum Beispiel als Caches für Nutzdaten eingesetzt. [@gruss-thesis, S.\ 22]

#### Virtually-Indexed Physically-Tagged

<!-- - VIPT: address translation kann mit cache lookup parallelisiert werden, keine Invalidierung notwendig -->

VIPT Caches nutzen die virtuelle Adresse für den Cache Index und die physische Adresse für den Tag. Der Cache Index ist unmittelbar verfügbar und wird genutzt um den entsprechenden Cacheeintrag zu laden. Parallel zu diesem Ladevorgang findet die Übersetzung von der virtuellen Adresse in eine physische Adresse statt. Dadurch wird wie bei VIVT Caches eine geringe Latenz bei Zugriffen auf den Cache ermöglicht. Außerdem werden die Tags wie bei PIPT Caches bei einem Context Switch nicht invalidiert. VIPT Adressierung wird typischerweise in den kleinsten und schnellsten Caches eines Systems verwendet. [@gruss-thesis, S. 22--23]

### Cache Replacement Strategien

<!-- - LRU: jeder Eintrag hat einen ungefähren Zeitstempel
- pseudo-random: zufälliger Eintrag wird ersetzt, einfacher zu implementieren und sogar teilweise höhere Performance in der Praxis
- moderne cpus können auch zwischen verschiedenen strategien wechseln -->

Ist im Cache die Kapazität an kongruenten Cachezeilen ausgelastet, so muss beim Zugriff auf eine weitere kongruente Cachezeile einer der vorhandenen Cacheeinträge ersetzt werden. Für optimale Performance müsste der Eintrag ersetzt werden, auf den in Zukunft am wenigsten zugegriffen wird. Um dieses Verhalten zu approximieren verwenden Prozessoren verschiedene Strategien.

Eine verwendete Strategie ist *least-recently used* (LRU) [@gruss-thesis, S. 20]. Hierbei wird der Eintrag ersetzt, auf den am längsten nicht mehr zugegriffen wurde. Um dies zu implementieren, beinhaltet jeder Cacheeintrag einen Zeitstempel des letzten Zugriffs [@gruss-thesis, S. 20]. Zu Problemen führt diese Strategie, wenn viele kongruente Cachezeilen gleichzeitig benötigt werden. Übersteigt diese Anzahl die Kapazität des Caches, kann im schlimmsten Fall jeder Speicherzugriff ein Cache Miss sein [@gruss-thesis, S. 20]. Eine andere Strategie ist die pseudo-zufällige Auswahl des zu ersetzenden Eintrages. Diese ist sehr einfach in Hardware zu implementieren und kann gute Performancewerte erreichen [@cache-random]. Die *last-in first-out* (LIFO) Strategie ersetzt stets den zuletzt geladenen Cacheeintrag.

Einige moderne Prozessoren wechseln dynamisch zwischen mehreren Strategien, abhängig von der beobachteten Nutzung des Caches. Dies ermöglicht es, die Vorteile verschiedener Strategien zu kombinieren. In der Praxis können dadurch Performanceverbesserungen erzielt werden. [@gruss-thesis, S.\ 21] [@cache-adaptive]

### Cache-Hierarchie

<!-- - Multiple Layers
  - Verschiedene Größen und Latenzen
  - Hierarchie kann Inklusiv sein oder nicht
  - In einer Schicht manchmal getrennt für Daten und Instruktionen
  - Pro Kern einen oder shared im gesamten System -->

In modernen Systemen werden Caches nicht einzeln verbaut. Stattdessen sind sie Teil einer mehrschichtigen Cache-Hierarchie. Die unteren Schichten dieser Hierarchie bilden schnelle Caches mit geringer Kapazität. Die Caches höherer Schichten sind stets langsamer und größer als die der darunterliegenden Schichten. Manche Caches enthalten immer sämtliche Einträge der darunterliegenden Schichten. In diesem Fall wird der Cache *inklusiv* genannt. Wenn Daten aus dem unteren Cache verdrängt werden, so bleiben sie im inklusiven oberen Cache weiterhin vorhanden. [@gruss-thesis, S. 23--24]

Ein Cache kann für Instruktionen oder für Daten reserviert sein oder sowohl Instruktionen als auch Daten beinhalten [@gruss-thesis, S. 23--24]. Im letzteren Fall wird der Cache auch *unified* genannt. In Mehrkernsystemen wird ein Cache entweder von allen Kernen gemeinsam verwendet, oder es existiert ein privater Cache für jeden Kern [@gruss-thesis, S. 23--24].

### Caches in modernen Intel Prozessoren {#sec:grundlagen-caches-modern}

<!-- - in modernen Intel CPUs:
  - L1d, L1i: per-core, 8-way set-associative, VIPT
  - L2: unified, PIPT, nicht inclusive
  - L3 or LLC: unified, PIPT, shared zwischen allen CPU cores, inclusive to L1 and L2, sliced
  - replacement policy: wechsel zwischen LRU und pseudo-random möglich -->

In modernen Intel x86-64 Prozessoren der Skylake Mikroarchitektur wird eine Cache-Hierarchie bestehend aus drei Schichten verwendet. Die unterste Schicht bilden die L1d und L1i Caches, die jeweils für Daten und für Instruktionen reserviert sind. Die L1 Caches sind virtually-indexed physically-tagged. Darüber liegt der L2 Cache, der sowohl Daten als auch Instruktionen beinhalten kann. Der L2 Cache ist physically-indexed physically-tagged und nicht inklusiv gegenüber den L1 Caches. L1 und L2 existieren pro Kern als private Caches. Der folgende L3 Cache wird auch *Last-Level Cache* (LLC) genannt. Er kann, wie der L2 Cache, sowohl Daten als auch Instruktionen beinhalten und ist physically-indexed physically-tagged. Anders als die L1 und L2 Caches wird derselbe L3 Cache von allen Kernen des Systems verwendet. Außerdem ist der L3 Cache inklusiv gegenüber den L1 und L2 Caches. Die L1 Caches sind 8-Way Set-Associative, der L2 Cache ist 4-Way Set-Associative und der L3 Cache ist bis zu 16-Way Set-Associative. Die Ersetzungsstrategie wechselt dynamisch zwischen LRU und einer Modifikation der LIFO [@gruss-thesis, S. 21]. Level 1 und 2 der Cache-Hierarchie enthalten zusätzlich für jeden Nutzdaten-Cache einen TLB. Der L1i TLB ist 8-Way Set-Associative, der L1d TLB ist 4-Way Set-Associative und der L2 TLB ist 12-Way Set-Associative. [@intel-opt, Kap. 2, S. 20]

![Abstrakter Aufbau und Cache-Hierarchie eines modernen Intel Prozessors. [@intel-sdm-1, Kap. 2, Abb. 8]](figs/cache-hierarchy.pdf){#fig:grundlagen-cache-hierarchy}

<!-- - abb zeigt abstrakten aufbau und cache hierarchie an einem beispiel prozessor
- (zeigt vier paare logischer prozessoren, die jeweils einen L1 und L2 cache und eine execution unit gemeinsam haben)
- zeigt individuelle L1 und L2 Caches für jeden Prozessorkern und einen gemeinsamen L3 Cache -->

[@fig:grundlagen-cache-hierarchy] zeigt den abstrakten Aufbau und die Cache-Hierarchie eines modernen Intel Prozessors. Außerdem zeigt sie individuelle L1 und L2 Caches für jeden Prozessorkern und einen gemeinsamen L3 Cache.

## Prozessororganisation der Intel Skylake Mikroarchitektur {#sec:grundlagen-mikroarch}

<!-- - verbindet die vorher beschriebenen Elemente und beschreibt Interaktionen zwischen ihnen
- beschreibt wie Instruktionen prozessorintern umgesetzt und ausgeführt werden -->

<!-- - Multi-core: Mehrere identische Kerne, ermöglichen mehrere Prozesse gleichzeitig auszuführen
  - Wie vorher bereits beschrieben: Kerne teilen sich Ressourcen (L3, prozessorexterne Elemente wie Hauptspeicher) oder nicht (L1, L2)
- Alle Kerne sind gleich aufgebaut
- Hyper-Threading: zwei logische kerne in einem physischen kern
  - teilen sich u.a. auch L1 und L2
  - Instruktionen beider logischer kerne werden verzahnt ausgeführt -->

Moderne Prozessoren besitzen nicht nur einen, sondern mehrere identische Ausführungskerne. Diese ermöglichen die Ausführung mehrerer Prozesse gleichzeitig, wodurch die Gesamtperformance des Systems signifikant erhöht werden kann [@gruss-thesis, S. 14]. Jede Resource eines Mehrkernsystems wird entweder von allen Kernen gemeinsam verwendet oder es existiert eine private Instanz der Ressource für jeden Kern [@gruss-thesis, S. 14]. Wie in [@sec:grundlagen-caches] beschrieben werden beispielsweise der L3 Cache und der Hauptspeicher gemeinsam verwendet. Die L1 und L2 Caches existieren separat für jeden Kern.

Neben mehreren Ausführungskernen besitzen viele moderne Prozessoren die Möglichkeit, mehrere Ausführungsstränge auf einem Kern gleichzeitig zu bearbeiten. Diese Technik wird bezeichnet als *Simultaneous Multithreading* (SMT), in Prozessoren von Intel auch *Hyper-Threading* [@intel-sdm-3, S. 8-24]. Jeder *physische* Ausführungskern wird dann konzeptuell als mehrere *logische* Kerne betrachtet. Prozessorintern werden die Instruktionen aller logischen Kerne verzahnt ausgeführt [@intel-sdm-3, Kap. 8, S. 27]. Da die logischen Kerne Teil desselben physischen Kerns sind, verwenden sie auch solche Ressourcen gemeinsam, die separat für jeden physischen Kern existieren.

<!-- (out-of-order execution allgemein?) -->

<!-- - Begriffe "Befehlssatzarchitektur" vs "Mikroarchitektur"
  - "Befehlssatzarchitektur": Verhalten des Prozessors gegenüber der Software
  - "Mikroarchitektur": wie die Befehlssatzarchitektur in konkreter Hardware / auf einem konkreten Prozessor implementiert ist -->

Bei der Beschreibung eines Prozessors sind zwei Begriffe zu unterscheiden. Die *Befehlssatzarchitektur* bezeichnet das Verhalten des Prozessors gegenüber der Software. Dies beinhaltet unter anderem Prozessormodi, Instruktionen und Register der Architektur. Die *Mikroarchitektur* bezeichnet eine konkrete Implementierung der Befehlssatzarchitektur. Dies beinhaltet unter anderem, wie die Instruktionen der Architektur prozessorintern umgesetzt und ausgeführt werden. Als *architektureller Zustand* wird der gemeinsame Zustand aller Elemente der Befehlssatzarchitektur bezeichnet. Dies umfasst beispielsweise die Prozessorregister und den Inhalt des Hauptspeichers. [@gruss-habil, S. 13--14, 43]

<!-- - In diesem Kapitel werden moderne Intel x86-64 Prozessoren der Skylake Mikroarchitektur beschrieben
  - Ähnlicher Aufbau in anderen Prozessortypen
- im Folgenden wird der Aufbau eines einzelnen Kerns beschrieben
- Mikroarchitektur in verschiedene Elemente aufgeteilt, die im Folgenden beschrieben werden
- Anschließend noch zwei wichtige Mechanismen -->

In diesem Kapitel werden speziell die Ausführungskerne moderner x86-64 Prozessoren der Intel Skylake Mikroarchitektur beschrieben. Viele andere Prozessortypen folgen jedoch einem ähnlichen Aufbau. [@fig:grundlagen-microarch] zeigt den abstrakten Aufbau einer dieser Ausführungskerne. Jeder Kern ist in drei verschiedene Elemente aufgeteilt, die im Folgenden detailliert beschrieben werden. Das *Front-End* lädt und dekodiert Instruktionen. Die *Execution Engine* bereitet die dekodierten Instruktionen auf ihre Ausführung vor und führt sie anschließend aus. Das *Memory Subsystem* enthält die Prozessorcaches und bildet die Schnittstelle zum Rest des Systems. [@meltdown, Kap. 2.1]

![Vereinfachte Darstellung eines einzelnen Ausführungskerns der Intel Skylake Mikroarchitektur. [@meltdown, Abb. 1] [@ridl, Abb. 9]](figs/microarch.pdf){#fig:grundlagen-microarch}

### Front-End

<!-- - front-end: lädt Instruktionen aus dem Speicher, dekodiert in kleinere Operationen (*µOPs*), gibt weiter an Execution Engine
  - Decoder, µ-Ops: Instruktionen werden in kleinere Operationen dekodiert
  - uop cache: speichert mapping Instruktion->uop für schnelleres dekodieren
- Teil des Front-End ist auch der Branch-Predictor, der später gesondert beschrieben wird
- Teil des Front-End ist auch der microcode sequencer mit seinem ROM, der später gesondert beschrieben wird -->

Das Front-End lädt die Instruktionen aus dem Speicher, die der Kern anschließend ausführen wird. Diese Instruktionen werden vom *Decoder* zunächst in eine oder mehrere kleinere Operationen dekodiert. Die resultierenden Operationen werden auch als Mikrooperationen oder *µOPs* bezeichnet. Um die erwartete Latenz des Dekodierens zu verkürzen, wird die Zuordnung zwischen Instruktionen und Mikrooperationen in dem *µOP Cache* zwischengespeichert. Wenn anschließend eine bereits dekodierte Instruktion nochmal dekodiert werden soll, können die entsprechenden Mikrooperationen direkt aus dem µOP Cache geladen werden. Auf diese Weise wird ein erneutes Dekodieren der Instruktion vermieden. Ein weiterer Teil des Front-End ist der *Branch Predictor*. Dieser trifft Vorhersagen über bedingte Sprünge, deren Bedingungen noch nicht ausgewertet sind. Der Branch Predictor wird in [@sec:grundlagen-mikroarch-transient] detailliert beschrieben. Abschließend gibt das Front-End die Mikrooperationen an die Execution Engine weiter. [@meltdown, Kap. 2.1]

### Execution Engine

<!-- - execution engine: bereitet uops auf ausführung vor, führt uops aus, (speichert ergebnisse)
  - reorder buffer: allokiert Register, benennt diese um und kümmert sich um Übertragung der Ergebnisse in den architekturellen Zustand, führt spezielle Optimierungen durch (z.B. move elimination, zeroing idioms)
  - unified reservation station / scheduler: reiht Operationen an Exit Ports ein, die mit den Execution Units verbunden sind
  - execution unit: führt Berechnung durch, spezialisiert für bestimme Instruktionen (z.B. ALU, AES, address generation, load, store)
    - Load, Store, Address Generation Unit sind mit Memory Subsystem verbunden
- drei Elemente über mehrere Common Data Buses verbunden (ein Bus für jede Art von übertragenen Daten, Load/Store/FP/Int/...)
- uops können in einer beliebigen Reihenfolge ausgeführt werden: uops deren Operanden verfügbar sind werden scheduled, der Rest wartet
  - *out-of-order execution*
- Retirement immer in der durch die Architektur vorgesehenen Reihenfolge -->

Die Execution Engine nimmt die im Front-End dekodierten Mikrooperationen entgegen. Diese werden zunächst in den *Reorder Buffer* eingereiht. Der Reorder Buffer allokiert die von der Mikrooperation benötigten Register. Wenn die Ausführung einer Mikrooperation abgeschlossen ist, überträgt der Reorder Buffer außerdem die Ergebnisse der Ausführung in den Registerspeicher. Die Mikrooperationen werden vom Reorder Buffer weitergeleitet an die *Unified Reservation Station*, auch *Scheduler* genannt. Sind alle Operanden einer Mikrooperation verfügbar, so übergibt der Scheduler die Mikrooperation der nächsten freien *Execution Unit*. Dabei können diese Mikrooperation in beliebiger Reihenfolge ausgeführt werden, unabhängig von der ursprünglichen Reihenfolge der Instruktionen. Diese Art der Ausführung wird *Out-of-Order Execution* genannt. Die Ergebnisse der Ausführung werden jedoch streng in der ursprünglichen Reihenfolge in den architekturellen Zustand übertragen. Mit dieser, *Retirement* genannten, Übertragung ist die Bearbeitung der Mikrooperationen abgeschlossen. Die Execution Units führen die eigentlichen Berechnungen durch. Jede Execution Unit ist auf bestimme Berechnungen spezialisiert, z.B. arithmetische Operationen, Gleitkommaoperationen, Vektoroperationen oder Speicherzugriffe. Execution Units für lesende Speicherzugriffe werden auch *Load Ports* genannt. Der Reorder Buffer, der Scheduler und die Execution Units sind außerdem über die *Common Data Buses* (CDB) miteinander verbunden. Über diese werden unter anderem die Ergebnisse der Mikrooperationen ausgetauscht. [@meltdown, Kap. 2.1] [@ridl, Kap. II.B]

### Memory Subsystem {#sec:grundlagen-mikroarch-memory}

<!-- - memory subsystem: eingehende Verbindung von Execution Units, enthält caches, TLBs, load und store buffer und line-fill buffer
  - store buffer: stores werden in store buffer eingereiht, CPU kann mit der nächsten uop weiter machen -> verbirgt latenz von stores
  - load buffer: benutzt um Fortschritt laufender Loads zu überwachen
  - line-fill buffer: wird benutzt um daten aus L2 oder L3 cache zu laden
    - überwacht loads, die auf einen L1d Cache Miss warten, ermöglicht non-blocking loads
  - store-to-load forwarding??? -->

Das Memory Subsystem beinhaltet die privaten Caches des Kerns, also die L1 und L2 Caches. Es ist mit dem L3 Cache und über diesen mit dem restlichen Speicher des Systems verbunden. Außerdem enthält es drei Pufferspeicher[^pufferspeicher]. Schreibende Speicherzugriffe werden von der jeweiligen Execution Unit in den *Store Buffer* gegeben. Mithilfe dieses Puffers wird der eigentliche Speicherzugriff durchgeführt. Dadurch wird die Latenz des Speicherzugriffs verborgen und die Execution Unit kann bereits die nächste Mikrooperation ausführen. Lesende Speicherzugriffe werden ähnlich zu schreibenden, aber über den *Load Buffer* verwaltet. Löst ein lesender Speicherzugriff einen Cache Miss im L1d Cache aus, so wird zusätzlich ein Eintrag im *Line-Fill Buffer* allokiert. Dieser überwacht den Ladevorgang aus dem L2 oder L3 Cache. Wie bei schreibenden Speicherzugriffen können auf diese Weise auch lesende Speicherzugriffe ohne die Latenz des Speichers bearbeitet werden. [@ridl, Kap. II.D]

<!-- [^pufferspeicher]: Diese Puffer funktionieren anders als die in [@sec:caches] beschriebenen Caches und sind daher hier unter einem eigenen Begriff gefasst. -->

[^pufferspeicher]: Wie bereits in [@sec:grundlagen-caches] dargestellt funktionieren diese Puffer anders als die in [@sec:grundlagen-caches] beschriebenen Caches und sind daher hier unter einem eigenen Begriff gefasst.

### Branch Prediction und Transient Execution {#sec:grundlagen-mikroarch-transient}

<!-- - Speculative Execution bei bedingten Sprüngen: wenn Auswertung der Bedingung noch nicht bekannt ist, versucht Branch Predictor diese vorherzusagen
  - z.B. auf Basis vorhergehender Ergebnisse der gleichen Bedingung
  - Folgende Instruktionen werden schon bearbeitet, bis Ergebnis feststeht
  - Korrekt -> Instruktionen können retiren
  - Falsch -> Instruktionen und ihre Ergebnisse werden verworfen (reorder buffer cleared, unified reservation station neu initialisiert)
  - transient instructions
  - verschiedene Buffer erläutern??? (branch target buffer, return stack buffer, pattern history table)
- Data Speculation???: In manchen Fällen Vorhersage von Daten (Ergebnis eines Loads) -->

Der *Branch Predictor* ist Teil des Front-Ends. Wenn die Bedingung eines bedingten Sprunges nicht unmittelbar bekannt ist, sagt der Branch Predictor den weiteren Kontrollfluss voraus. Diese Vorhersage kann basieren auf vorherigen Auswertungen ähnlicher Bedingungen oder auf dem bedingten Sprung selber [@meltdown, Kap. 2.1]. Auf diese Weise können folgende Instruktionen bereits bearbeitet werden, bevor die Bedingung tatsächlich ausgewertet wurde. Dies wird spekulative Ausführung oder *Speculative Execution* genannt [@ridl, Kap. II.C]. War die Vorhersage korrekt, so können die Ergebnisse der spekulativ ausgeführten Instruktionen in den architekturellen Zustand übernommen werden [@meltdown, Kap. 2.1]. War die Vorhersage inkorrekt, werden die spekulativ ausgeführten Instruktionen und ihre Ergebnisse verworfen. Der Reorder Buffer wird geleert, der Scheduler neu initialisiert und das Front-End beginnt, die Instruktionen auf dem korrekten Pfad zu dekodieren [@meltdown, Kap. 2.1]. Die Instruktionen, die nach einer falschen Vorhersage ausgeführt und anschließend verworfen werden, werden *Transient Instructions* genannt [@fallout, Kap. 2.2]. Die spekulative Ausführung heißt im Falle einer falschen Vorhersage auch *Transient Execution* [@fallout, Kap. 2.2].

<!-- - "Transient Execution tritt nicht nur nach bedingten Sprünge auf, sondern gelegentlich auch in linearem Kontrollfluss."
- Transient Execution kann auch bei linearem Kontrollfluss auftreten
- Wenn eine Instruktion eine Prozessor-Exception auslöst, können folgende Instruktionen noch durch Transient Execution ausgeführt werden
- Exception wird erst bei Retirement der faultenden Instruktion behandelt
- daten des load sind schon vorher verfügbar
- dank out-of-order execution können folgende instruktionen diese daten benutzen -->

Eine andere Form der Transient Execution tritt auch in linearem Kontrollfluss auf. Löst eine Instruktion eine Prozessor-Exception aus, so beginnt eine Transient Execution, die folgende Instruktionen weiterhin ausführen kann. Grund dafür ist, dass der Prozessor die Exception erst bei Retirement der auslösenden Instruktion behandelt. Sind die Operanden folgender Instruktionen bereits verfügbar, können diese also noch ausgeführt werden, bevor die Prozessor-Exception behandelt wird. Siehe auch [@sec:impl-spectre-exception] für Angriffe, die diese Form der Transient Execution ausnutzen. [@meltdown, Kap. 3]

<!-- - "Die Ergebnisse dieser Instruktionen werden bei Behandlung der Prozessor-Exception verworfen."
- folgende instruktionen werden zurückgerollt, zustand des caches jedoch nicht -->

Die Transient Instructions und ihre Ergebnisse werden bei Abschluss der Transient Execution verworfen. Der Zustand der Caches wird jedoch nicht in den Zustand vor Beginn der Transient Execution zurückversetzt. Folglich kann ein Speicherzugriff während einer Transient Execution eine Eintragung im Cache verursachen. Diese ist sogar nach Abschluss der Transient Execution über einen Cache-basierten Seitenkanal (siehe [@sec:grundlagen-sidechannel]) beobachtbar. [@meltdown, Kap. 3]

<!-- - faulting load und instruktionen danach werden zur ausführung eingereiht
- sobald daten des loads verfügbar sind können folgende instruktionen ausgeführt werden
- exception wird jedoch erst generiert wenn load retiret
- folgende instruktionen werden zurückgerollt, zustand des caches jedoch nicht -->

<!-- ### Microcode Assists

- microcode assists "small programs injected into the execution stream"
  - Unter Bestimmten Umständen löst der decoder einen microcode assist aus
  - Selten benutzte Instruktionen müssen nicht in Hardware kodiert werden, sondern werden in ROM (ist das wirklich readonly? Siehe microcode updates) gespeichert
    - Beispiel: Viele der VMX oder SGX Instruktionen?
  - Selten eintretende Fälle können separat vom Rest der Instruktion implementiert werden
    - Beispiel: Page Table Walk der accessed/dirty Bits updaten muss
    - Beispiel: subnormal floating point numbers
  - Verhalten von Instruktionen kann per Microcode-Update verändert werden
  - MS = microcode sequencer
  - "microcode sequences might be hundreds of instructions long" <https://software.intel.com/content/www/us/en/develop/documentation/vtune-help/top/reference/cpu-metrics-reference/bad-speculation-back-end-bound-pipeline-slots/ms-assists.html>
  - assists trigger transient execution similar to faults but require no fault suppression
  - assist triggers machine clear -> pipeline flush

- Image from RIDL paper
- Image from Meltdown paper???
- Take elementes from both images and create own image -->
