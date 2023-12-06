Pomoću biblioteke Sockets API napisana su dva programa (klijent, poslužitelj) koji čine aplikaciju ***Praćenje studenata***. Poslužitelj pohranjuje tablicu u kojoj pišu JMBAG-ovi studenata i brojevi dvorana na fakultetu u kojima se ti studenti nalaze. Serverski program obrađuje svakog klijenta u svojoj dretvi (pritom se pazi na sinkronizaciju pristupa dijeljenom resursu - dvoranama). Postoje tri oblika komunikacije između klijenta i poslužitelja:
- kod prvog oblika komunikacije klijent obavještava poslužitelja da se student sa zadanim JMBAG-om
upravo nalazi u zadanoj dvorani. Poslužitelj potvrđuje klijentu da je primio poruku te ažurira svoju tablicu.
- kod drugog oblika komunikacije klijent pita poslužitelja gdje se nalazi student sa zadanim JMBAG-om. Poslužitelj odgovara slanjem broja dvorane.
- kod trećeg oblika komunikacije klijent pita poslužitelja koliko ima studenata u zadanoj dvorani. Poslužitelj odgovara slanjem broja studenata.
