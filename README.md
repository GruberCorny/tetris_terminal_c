# Tetris in C

Ein vollständiges Tetris-Spiel in C mit ncurses, entwickelt als Praktikumsprojekt.

# Spielregeln
- Stapel fallende Tetromino-Steine
- Fülle komplette Zeilen um sie zu löschen
- Das Spiel wird schneller mit jedem Level
- Spiel endet wenn die Steine oben ankommen

# Steuerung
 ← → , A D = Stein nach links/rechts bewegen 
 ↓ , S = Stein schneller fallen lassen 
 ↑ , W = Stein sofort platzieren 
 R = Stein rotieren 
 E = Stein speichern/tauschen 
 Q = Spiel beenden 

# Level-Progression
- Level 1: 500ms Fall-Geschwindigkeit
- Level-Up: Alle 5 gelöschte Linien
- Wird um 50ms pro Level schneller
- Maximum: 50ms

Das Spiel verwendet das faire "7-Bag" System:
- Alle 7 Tetromino-Typen kommen garantiert in einem "Bag" vor
- Jeder Bag wird gemischt bevor die Steine ausgegeben werden
- Garantiert faire Verteilung - kein Pech mit zu vielen gleichen Steinen!

