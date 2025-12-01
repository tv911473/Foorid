TARKVARA STRUKTUURI KIRJELDUS

- foor.html - üksiku foori browseri fail
- koondleht.html - administraatori leht kus mitu foori töötavad
- main.ccp - kontrolleri kood

INSTALLIMISJUHIS

1. Kopeeri Git repositoorium kohalikku masinasse
2. Loo Google Apps Scripti kaks uut skripti ja kopeeri sisu configuration_script.txt ja server_time_script.txt failidest.
3. Deploy skriptid, kopeeri URL aadressid failidesse:
   foor.html: serverTimeURL; confiURL;
   koondleht.html: confiURL;
   main.ccp: serverTimeURL; confiURL;

4. Ava Koondleht.html brauseris
5. Lae kood kontrollerile:
   Igale kontrollerile määrata ära foori nr 1 - ... : line 32 - const int fooriNr = 1;

LIIKLUSKORRALDUS

Fooride süsteem on kasulik tiheda päevase liiklusega sirgel teel kus autod sujuv
liiklusvoog sõltub paljustki rohelise tule lainest.

ANDMEVAHETUSE KIRJELDUS
Kontroller loeb serverist aja ja konfiguratsiooni sätted.

ELEKTRISKEEM
(elektriskeem.png)
