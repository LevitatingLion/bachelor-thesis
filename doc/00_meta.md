---
title: "Funktionsweise und Evaluation von modernen Spectre-Angriffen"
subtitle: "Am Beispiel von RIDL, Fallout und ZombieLoad"
author: "Jan-Niklas Sohn"
date: "2021-04-19"
lang: "de"
thesis-type: "bachelor"
thesis-type-custom: ""
thesis-matrikel: ""
thesis-degree-course: "Informatik (B.Sc.)"
thesis-submission: "Bonn, 19. April 2021"
thesis-supervisor-one: "Dr. Felix Jonathan Boes"
thesis-supervisor-two:
    name: "Prof. Dr. Karl Jonas"
    affiliation: "Hochschule Bonn-Rhein-Sieg"
thesis-sponsor: "Dr. Felix Jonathan Boes"
thesis-affiliation: |
    Rheinische Friedrich-Wilhelms-Universität Bonn \
    Institut für Informatik IV \
    Arbeitsgruppe für IT-Sicherheit \
thesis-twosided: false
thesis-monochrome: false
thesis-table-position: "end"
thesis-print-declaration: true
thesis-reference-style: ""
abstract: |
    Die im Jahre 2018 vorgestellten Sicherheitslücken Spectre und Meltdown haben die IT-Sicherheit nachhaltig beeinflusst. Das neu begründete Forschungsfeld der Sicherheit spekulativer Ausführung umfasst mittlerweile viele Varianten der ursprünglich entdeckten Sicherheitslücken. Dazu gehören unter anderem RIDL, ZombieLoad, Write Transient Forwarding und Store-to-Leak. Write Transient Forwarding und Store-to-Leak werden zusammen auch als Fallout bezeichnet. Mithilfe dieser Sicherheitslücken können Daten aus verschiedenen Puffern der Mikroarchitektur extrahiert werden. Dies hat weitreichende Konsequenzen für Sicherheitsmodelle mit gemeinsamer Hardware, beispielsweise im Cloud Computing.
    In der vorliegenden Arbeit wird zunächst die Funktionsweise dieser Sicherheitslücken erläutert. Anschließend werden Angriffe auf diese in verschiedenen Varianten implementiert. Die implementierten Angriffe können von unprivilegierten Nutzerprozessen auf Linux-Systemen eingesetzt werden, um Daten anderer Prozesse oder des Betriebssystem-Kerns zu extrahieren. Schließlich werden die implementierten Angriffe hinsichtlich einheitlicher Metriken evaluiert.
    Bei dieser Evaluation zeigen drei von den vier betrachteten Angriffen die erwarteten Ergebnisse in den meisten ihrer Varianten. RIDL, ZombieLoad und Store-to-Leak extrahieren erfolgreich anvisierte Daten. Durch Write Transient Forwarding hingegen können in keiner der evaluierten Varianten die anvisierten Daten erfolgreich extrahiert werden.
thanks: ""
---

<!-- - Einleitung ins Thema
  - transient execution angriffe: spectre, meltdown
  - viele neue varianten
  - insbesondere ridl, zombieload, write transient forwarding, store-to-leak
    - wtf+stl werden auch fallout genannt
    - extrahieren daten aus verschiedenen puffern der mikroarchitektur
- Fragestellung/Ziel der Arbeit
  - in der vorliegenden arbeit wird, in dieser arbeit, das ziel dieser arbeit
  - (funktionsweise moderner cpus soweit wie nötig erarbeiten)
  - funktionsweise dieser angriffe erarbeiten
  - angriffe implementieren und evaluieren
    - verschiedene varianten hinsichtlich einheitlicher metriken
    - (unprivilegierter nutzerprozess auf linux, extrahiert daten aus anderen prozessen)
- Zusammenfassung der Ergebnisse
  - es stellt sich heraus, die evaluation hat ergeben
  - die meisten angriffe (ridl, zombieload, store-to-leak) wurden erfolgreich implementiert
  - ergebnisse von write transient forwarding konnten nicht reproduziert werden -->

\hyphenation{Zombie-Load}
\hyphenation{Reload}
