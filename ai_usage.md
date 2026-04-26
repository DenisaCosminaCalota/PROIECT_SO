Documentație utilizare AI - Faza 1

Pentru realizarea primei etape a proiectului, am utilizat asistența AI pentru implementarea funcțiilor de filtrare a datelor:

Funcția parse_condition: Am folosit AI pentru a genera logica de extragere a câmpurilor din șirul de caractere primit ca argument (ex: severitate:>=:3). AI-ul a propus utilizarea sscanf cu formatul [^:], soluție care s-a dovedit eficientă.

Funcția match_condition: Am solicitat o funcție care să compare datele din structura ReportFile cu valorile filtrate, gestionând atât comparații numerice pentru severitate, cât și comparații de șiruri pentru categorii.

Contribuție personală și corecții:

Am adaptat codul generat pentru a funcționa cu structura mea de date imbricată (coordonatele GPS).

Am identificat și rezolvat o problemă la funcția de adăugare: AI-ul nu a prevăzut consumarea caracterului newline după citirea severității, ceea ce bloca citirea descrierii. Am corectat acest lucru adăugând manual getchar().

Toată logica de gestionare a fișierelor (apelurile de sistem open, write, lseek), setarea precisă a permisiunilor conform tabelului din cerință și crearea link-urilor simbolice au fost implementate manual.
