Documentație utilizare AI - Faza 1

Pentru realizarea primei etape a proiectului, am utilizat asistența AI pentru implementarea funcțiilor de filtrare a datelor:

Funcția parse_condition: Am folosit AI pentru a genera logica de extragere a câmpurilor din șirul de caractere primit ca argument (ex: severitate:>=:3). AI-ul a propus utilizarea sscanf cu formatul [^:], soluție care s-a dovedit eficientă.

Funcția match_condition: Am solicitat o funcție care să compare datele din structura ReportFile cu valorile filtrate, gestionând atât comparații numerice pentru severitate, cât și comparații de șiruri pentru categorii.

Contribuție personală și corecții:

Am adaptat codul generat pentru a funcționa cu structura mea de date imbricată (coordonatele GPS).

Am identificat și rezolvat o problemă la funcția de adăugare: AI-ul nu a prevăzut consumarea caracterului newline după citirea severității, ceea ce bloca citirea descrierii. Am corectat acest lucru adăugând manual getchar().

Toată logica de gestionare a fișierelor (apelurile de sistem open, write, lseek), setarea precisă a permisiunilor conform tabelului din cerință și crearea link-urilor simbolice au fost implementate manual.



Documentație utilizare AI - Faza 2
Pentru realizarea celei de-a doua etape a proiectului, am utilizat asistența AI mai mult pentru gestionarea semnalelor și structura de bază a proceselor:

Am cerut ajutor pentru generarea scheletului de cod legat de crearea procesului fundal de tip daemon pentru Hub și pentru logica de scriere a PID-ului în fișierul ascuns .monitor_pid. Pentru partea de notificare în timp real, am solicitat o soluție prin care city_manager să poată anunța monitorul de fiecare dată când apare un raport nou, iar AI-ul a propus utilizarea funcției kill împreună cu semnalul SIGUSR1.

Contribuție personală și corecții:
Codul propus inițial de AI pentru handlerul de semnal folosea funcții nesigure în caz de execuție asincronă, cum ar fi printf direct în interiorul handlerului. Am corectat manual această problemă majoră, eliminând funcțiile nesigure și folosind în schimb variabile de tip volatile sig_atomic_t pentru a asigura o funcționare corectă.
Toată logica de calcul concurent al scorurilor de prioritate și maparea pe directoarele specifice fiecărui district au fost scrise și integrate manual de mine.



Documentație utilizare AI - Faza 3
Pentru realizarea etapei finale a proiectului, am utilizat asistența AI pentru gestionarea cazurilor limită la ștergerea datelor și curățarea legăturilor:

Am folosit AI pentru a pune la punct logica de parcurgere și golire a fișierelor dintr-un director înainte de a putea apela funcția de ștergere a folderului complet pentru comanda remove_district. De asemenea, am cerut o confirmare legată de gestionarea corectă a link-urilor simbolice, AI-ul amintindu-mi să folosesc apelul de sistem unlink pentru a șterge scurtăturile din folderul principal.

Contribuție personală și corecții:
Am modificat codul generat pentru a impune restricțiile administrative din cerință, deoarece AI-ul tindea să lase comenzile globale. Am adăugat manual verificările în program.c pentru ca acțiunea remove_district să poată fi pornită strict dacă utilizatorul are setat flag-ul --role manager.
Am rulat și testat manual tot sistemul în terminal pentru a popula proiectul cu cele două districte cerute (Timis și Dumbravita) și cele 5 rapoarte unice salvate binar, asigurându-mă că directoarele și fișierele reports.dat sunt curate înainte de a le urca pe Git.
