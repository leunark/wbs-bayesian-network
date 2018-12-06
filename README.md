# wbs-bayesian-network
# book recommendation
This repo is about a program that creates a bayesian network based on an example data set for a uni project.
https://github.com/leunark/wbs-bayesian-network

# dependencies
- dlib library: http://dlib.net/
- sqlite library: https://www.sqlite.org/cintro.html

# task [ger]
"Es soll eine Buchempfehlung ausgesprochen werden. Zur Wahl steht nur eine begrenzte Zahl an Büchern. Als mögliche Informationen können die Angaben zu Altersgruppe, Geschlecht, Familienstand, Kinderzahl, Einkommen, Bildungsstand und Beruf vorliegen. Diese müssen nicht vollständig sein. Erstellen Sie ein Bayes Netz, welches die Zusammenhänge modelliert und plausibel auf Basis der Beispieldaten gefüllte CPTs nutzt.
Legen Sie diesem Bayes Netz Beispieleingaben vor und geben Sie das Klassifikationsergebnis geeignet aus.

Entwickeln Sie eine Software, welche bei Eingabe (Datei, vgl. Beispielformat)
von Testdaten die entsprechenden Klassifikationen mit Hilfe der Bayes Netz
Implementierung geeignet bestimmt und ausgibt."

# run
Das Programm benötigt die Datei "P001.csv" im selben Verzeichnis wie die .exe.
Dann kann das Programm ausgeführt und nur mit den Zahlen 0-9 bedient werden.
Momentan ist dieses Programm nur für die Beispieldatei ausgelegt, kann aber einfach erweitert werden.
Der Datei können neue Beispieldatensätze hinzugefügt werden, die Struktur darf sich aber nicht verändern.

# strategy
Es handelt sich bei den Daten um eine CSV-Datei mit Einträgen zu dem probabilistischen Netz. Jede Spalte repräsentiert ein Node im Netz (außer Nr), die miteinander verstrickt sind. Die Struktur für ein probabilistisches Netz erwartet Expertenwissen, das selber manuell erstellt werden muss. Dafür wurde das Tool Netica verwendet. Theoretisch ist es möglich, wenn ausreichend Daten zur Verfügung stehen, maschinelles Lernen anzuwenden, um Struktur und Kantenstärke herauszufinden. Dies würde aber den Scope sprengen.
Die Daten "buchempfehlung.neta" ist ein Netica File, das geöffnet werden kann, um die zugrunde liegende Struktur dieser Aufgabe zu sehen, alternativ ist auch ein Bild beigefügt ("buchempfehlung.jpg").

Sind einmal die verschiedenen Nodes und States in dem Programm eingepflegt, wird der Rest vollautomatisch generiert. 

Im ersten Schritt werden die Daten aus "P001.csv" gelesen und in eine SQLite Datenbank geschrieben.
Danach wird für jedes Node alle möglichen Kombinationen mit den Parents bestimmt, um die CPT zu erstellen. Die Generation der CPT ist eine Methode "startGeneratingCPT()" der Klasse Node. Tatsächlich ist sie aber nur ein Trigger für den Aufruf der privaten rekursiven Methode "generateCPT(...)".
Nun wird durch jede CPT jedes Nodes durchiteriert, um die Wahrscheinlichkeiten zu berechnen. Hier kommt auch die lokale Datenbank zum Einsatz, da für die Berechnung der CPT, Häufigkeiten von den Kombinationen bestimmt werden müssen. Dafür wird immer ein SQL Statement als String gebaut und dann auf der Datenbank ausgeführt. Ein Callback verarbeitet dann die Daten, um die errechnetet Wahrscheinlichkeit zurückgeben zu können. Sind keine Daten in der Datenbank, bedeutet dies, dass keine Informationen in das Netz geladen werden können. In diesem Fall sind alle Zustände des Nodes gleichwahrscheinlich für die eine Kombination. Schließlich kann die Wahrscheinlichkeit dann in die Logik der dlib/bayesiannetwork-Bibliothek geschrieben werden.
Im letzten Schritt werden Eingaben für Evidenzen vom User abgefragt, um sie in das Netz einzupflegen. Das Buch mit der höchsten Wahrscheinlichkeit wird empfohlen.

Es wird zusätzlich eine Log-Datei erstellt, um den Vorgang etwas nachvollziehen zu können, und ein Einblick in die Wahrscheinlichkeiten des probabilistischen Netzes zu gewähren. Nach dem Ausführen des Programms erscheint eine log-Datei im selben Pfad der .exe!

# build
1. Add $(LocalDebuggerWorkingDirectory) to Configuration Properties / VC++ Directories / Include Directory
2. Add /bigobj to: Configuration Properties / C/C++ / Command Line / Additional Options

