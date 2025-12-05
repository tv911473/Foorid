TARKVARA STRUKTUURI KIRJELDUS

- foor.html - üksiku foori browseri fail
- koondleht.html - administraatori leht
- main.ccp - kontrolleri kood

INSTALLIMISJUHIS

1. Kopeeri Git repositoorium kohalikku masinasse
2. Loo Google Apps Scripti kaks uut skripti ja kopeeri sisu configuration_script.txt ja server_time_script.txt failidest.
3. Deploy skriptid, kopeeri URL aadressid failidesse:
   foor.html: serverTimeURL; confiURL;
   koondleht.html: confiURL;
   main.ccp: serverTimeURL; confiURL;
4. Ava koondleht.html brauseris
5. Ava foor.html ja lisa URL lõppu ?id=`foori number` (nt .../foor.html?id=2)
6. Lae kood kontrollerile:
   Kasuta koodi laadimiseks Arduino IDE või VS Code + PlatformIO IDE (soovituslik)
   Igale kontrollerile määrata ära foori nr 1 - ... : line 32 - const int fooriNr = 1;

LIIKLUSKORRALDUS

Fooride süsteem on kasulik tiheda päevase liiklusega sirgel teel kus autod sujuv
liiklusvoog sõltub paljustki rohelise tule lainest.

ANDMEVAHETUSE KIRJELDUS

1. Kontrollerid/URL foorid pärivad serverist aja ja konfiguratsiooni sätted.
2. Server saab muutujate väärtused google sheets lehelt ja saadab kontrolleritele/URL fooridele.
3. Liikluskiiruse muutumisel koondlehel saadetakse uued väärtused serverisse
4. Server kirjutab uued väärtused google sheets tabelisse.
5. ÖÖrešiimi käivitamisel kirjutab server google sheets lehele TRUE/FALSE.

ELEKTRISKEEM
(elektriskeem.png)
