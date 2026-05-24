#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXIM_CARACTERE 100

#define PERMISIUNI_DIRECTOR 0750   // rwxr-x---
#define PERMISIUNI_RAPOARTE 0664   // rw-rw-r--
#define PERMISIUNI_CONFIG   0640   // rw-r-----
#define PERMISIUNI_LOG      0644   // rw-r--r--

// structura folosita pentru salvarea unui raport in fisier
typedef struct {
    int reportId;
    char inspectorName[MAXIM_CARACTERE];
    struct {
        double latitude;
        double longitude;
    } GPSCoordinates;
    char issueCategory[MAXIM_CARACTERE];
    int severityLevel;
    time_t timestamp;
    char descriptionText[MAXIM_CARACTERE];
} ReportFile;

//transforma permisiunile din numere in format (rwx), pentru afisare
void transforma_permisiuni_in_text(mode_t mod_fisier, char *sir_destinatie) {
    sir_destinatie[0] = (mod_fisier & S_IRUSR) ? 'r' : '-';
    sir_destinatie[1] = (mod_fisier & S_IWUSR) ? 'w' : '-';
    sir_destinatie[2] = (mod_fisier & S_IXUSR) ? 'x' : '-';
    sir_destinatie[3] = (mod_fisier & S_IRGRP) ? 'r' : '-';
    sir_destinatie[4] = (mod_fisier & S_IWGRP) ? 'w' : '-';
    sir_destinatie[5] = (mod_fisier & S_IXGRP) ? 'x' : '-';
    sir_destinatie[6] = (mod_fisier & S_IROTH) ? 'r' : '-';
    sir_destinatie[7] = (mod_fisier & S_IWOTH) ? 'w' : '-';
    sir_destinatie[8] = (mod_fisier & S_IXOTH) ? 'x' : '-';
    sir_destinatie[9] = '\0';
}

// scrie actiunile utilizatorilor în fisierul de log
//foloseste O_APPEND pentru a adauga date la finalul fisierului, fara a suprascrie istoricul
void inregistreaza_operatiune_log(const char *nume_district, const char *nume_utilizator, const char *rol_utilizator, const char *actiune) {
    char cale_log[256];
    sprintf(cale_log, "%s/logged_district", nume_district);

    int descriptor_log = open(cale_log, O_WRONLY | O_CREAT | O_APPEND, PERMISIUNI_LOG);
    if (descriptor_log >= 0) {
        char mesaj[512];
        time_t timp_acum = time(NULL);
        char *timp_text = ctime(&timp_acum);
        timp_text[strlen(timp_text)-1] = '\0';

        int lungime = sprintf(mesaj, "[%s] %s (%s): %s\n", timp_text, nume_utilizator, rol_utilizator, actiune);
        write(descriptor_log, mesaj, lungime);
        close(descriptor_log);
        chmod(cale_log, PERMISIUNI_LOG);
    }
}

// extrage campul, operatorul si valoarea pentru filtre
int parse_condition(const char *input, char *field, char *op, char *value) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3;
}

// verifica daca raportul respecta filtrul
int match_condition(ReportFile *raport, const char *camp, const char *operator, const char *valoare) {
    if (strcmp(camp, "severity") == 0) {
        int v = atoi(valoare);
        if (strcmp(operator, "==") == 0) return raport->severityLevel == v;
        if (strcmp(operator, ">=") == 0) return raport->severityLevel >= v;
        if (strcmp(operator, "<=") == 0) return raport->severityLevel <= v;
    } else if (strcmp(camp, "category") == 0) {
        if (strcmp(operator, "==") == 0) return strcmp(raport->issueCategory, valoare) == 0;
    } else if (strcmp(camp, "inspector") == 0) {
        if (strcmp(operator, "==") == 0) return strcmp(raport->inspectorName, valoare) == 0;
    }
    return 0;
}

// sterge directorul unui district verifica mai intai dacă utilizatorul are drepturi de "manager", apoi, creează un proces separat (fork) care ruleaza comanda de sistem "rm - rf" pentru a sterge efectiv dosarul
//parintele asteapta finalizarea operațiunii (waitpid), si, dacă totul a decurs corect, elimina si legatura simbolica (unlink) ramasa
void remove_district(const char *nume_district, const char *rol_utilizator) {
    if (rol_utilizator == NULL || strcmp(rol_utilizator, "manager") != 0) {
        printf("Acces interzis: Doar managerul poate sterge districte.\n");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Eroare la fork");
        return;
    }

    if (pid == 0) {
        // procesul copil executa stergerea fortata a directorului
        execlp("rm", "rm", "-rf", nume_district, NULL);
        perror("Eroare la execlp");
        exit(1);
    } else {
        // procesul parinte monitorizeaza finalizarea copilului
        int status;
        waitpid(pid, &status, 0);

        // daca stergerea directorului a reusit, se elimina si link-ul asociat
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            char nume_link[256];
            sprintf(nume_link, "active_reports-%s", nume_district);

            if (unlink(nume_link) == -1) {
                if (errno != ENOENT) perror("Eroare la eliminarea symlink-ului");
            }
            printf("Districtul '%s' a fost eliminat complet.\n", nume_district);
        } else {
            printf("Eroare: Procesul rm nu a terminat corect.\n");
        }
    }
}


int main(int argc, char *argv[]) {
    char *rol_utilizator = NULL, *nume_utilizator = NULL, *comanda = NULL, *nume_district = NULL;
    int index_filtre = 0;

    // extrag argumentele din linia de comanda
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) rol_utilizator = argv[++i];
        else if (strcmp(argv[i], "--user") == 0) nume_utilizator = argv[++i];
        else if (comanda == NULL) comanda = argv[i];
        else if (nume_district == NULL) { nume_district = argv[i]; index_filtre = i + 1; }
    }

    // verific argumentele obligatorii
    if (!rol_utilizator || !comanda || !nume_district) {
        printf("Eroare: Lipsesc argumente obligatorii.\n");
        return 1;
    }

    char cale_rapoarte[256], cale_config[256];
    sprintf(cale_rapoarte, "%s/reports.dat", nume_district);
    sprintf(cale_config, "%s/district.cfg", nume_district);

    //salvează un raport nou
    //notifica monitorul prin semnalul SIGUSR1
    if (strcmp(comanda, "add") == 0) {
        mkdir(nume_district, PERMISIUNI_DIRECTOR);
        chmod(nume_district, PERMISIUNI_DIRECTOR);
        int descriptor_fisier = open(cale_rapoarte, O_WRONLY | O_CREAT | O_APPEND, PERMISIUNI_RAPOARTE);
        if(descriptor_fisier < 0) { perror("Eroare deschidere fisier rapoarte");
            return 1;

        }

        ReportFile raport_nou;
        memset(&raport_nou, 0, sizeof(ReportFile));

        // genereaza automat ID-ul raportului
        char cale_contor[256];
        sprintf(cale_contor, "%s/last_id.txt", nume_district);

        int id_alocat = 1;
        int fd_contor = open(cale_contor, O_RDWR | O_CREAT, PERMISIUNI_CONFIG);

        if (fd_contor >= 0) {
            char buf_id[16];
            ssize_t bytes_cititi = read(fd_contor, buf_id, sizeof(buf_id) - 1);

            if (bytes_cititi > 0) {
                buf_id[bytes_cititi] = '\0';
                id_alocat = atoi(buf_id) + 1;
            }

            lseek(fd_contor, 0, SEEK_SET);
            ftruncate(fd_contor, 0);

            char buf_scriere[16];
            int len_scriere = sprintf(buf_scriere, "%d", id_alocat);
            write(fd_contor, buf_scriere, len_scriere);
            close(fd_contor);
        }

        raport_nou.reportId = id_alocat;

        // citirea datelor raportului
        printf("ID alocat automat: %d\n", raport_nou.reportId);
        printf("Coordonate GPS (Lat Lon): ");
        scanf("%lf %lf", &raport_nou.GPSCoordinates.latitude, &raport_nou.GPSCoordinates.longitude);
        printf("Categorie: ");
        scanf("%s", raport_nou.issueCategory);
        printf("Severitate: ");
        scanf("%d", &raport_nou.severityLevel);
        getchar();
        printf("Descriere: ");

        fgets(raport_nou.descriptionText, MAXIM_CARACTERE, stdin);
        raport_nou.descriptionText[strcspn(raport_nou.descriptionText, "\n")] = 0;
        strcpy(raport_nou.inspectorName, nume_utilizator);
        raport_nou.timestamp = time(NULL);

        // salvarea raportului in fisier
        write(descriptor_fisier, &raport_nou, sizeof(ReportFile));
        close(descriptor_fisier);
        chmod(cale_rapoarte, PERMISIUNI_RAPOARTE);

        // notific monitorul prin SIGUSR1
        int monitor_a_fost_notificat = 0;
        int descriptor_citire_pid = open(".monitor_pid", O_RDONLY);
        if (descriptor_citire_pid != -1) {
            char buffer_pid[16];
            ssize_t bytes_cititi = read(descriptor_citire_pid, buffer_pid, sizeof(buffer_pid) - 1);
            if (bytes_cititi > 0) {
                buffer_pid[bytes_cititi] = '\0';
                pid_t pid_monitor = atoi(buffer_pid);
                if (kill(pid_monitor, SIGUSR1) == 0) {
                    monitor_a_fost_notificat = 1;
                }
            }
            close(descriptor_citire_pid);
        }

        // creez symlink catre rapoarte
        char nume_link[256];
        sprintf(nume_link, "active_reports-%s", nume_district);
        unlink(nume_link);
        symlink(cale_rapoarte, nume_link);

        char mesaj_log[256];
        if (monitor_a_fost_notificat) {
            sprintf(mesaj_log, "Raport %d adaugat. Monitor notificat.", id_alocat);
        } else {
            sprintf(mesaj_log, "Raport %d adaugat. Monitorul nu a raspuns.", id_alocat);
        }
        inregistreaza_operatiune_log(nume_district, nume_utilizator, rol_utilizator, mesaj_log);

        printf("Raport salvat. Status Monitor: %s\n", monitor_a_fost_notificat ? "NOTIFICAT" : "INACTIV");
    }
    /* Citește și afișează conținutul fișierului de rapoarte. */
    else if (strcmp(comanda, "list") == 0) {
        struct stat info_fisier;
        if (lstat(cale_rapoarte, &info_fisier) < 0) {
            if (errno == ENOENT) printf("Avertisment: Link dangling sau district inexistent.\n");
            return 1;
        }

        char permisiuni_simbolice[10];
        transforma_permisiuni_in_text(info_fisier.st_mode, permisiuni_simbolice);
        printf("Fisier: reports.dat | Permisiuni: %s | Marime: %ld\n", permisiuni_simbolice, info_fisier.st_size);

        int descriptor_fisier = open(cale_rapoarte, O_RDONLY);
        ReportFile raport_citit;
        while (read(descriptor_fisier, &raport_citit, sizeof(ReportFile)) > 0) {
            printf("ID: %d | Cat: %s | Sev: %d | Inspector: %s\n",
                   raport_citit.reportId, raport_citit.issueCategory, raport_citit.severityLevel, raport_citit.inspectorName);
        }
        close(descriptor_fisier);
    }

    /* Elimină un raport prin mutarea datelor următoare peste acesta și redimensionarea fișierului. */
    else if (strcmp(comanda, "remove_report") == 0) {
        if (strcmp(rol_utilizator, "manager") != 0) {
            printf("Acces interzis: Doar managerul poate sterge rapoarte.\n");
            return 1;
        }
        int id_tinta = atoi(argv[index_filtre]);
        int descriptor_fisier = open(cale_rapoarte, O_RDWR);

        ReportFile raport_temp; off_t pozitie = 0; int gasit = 0;
        while (read(descriptor_fisier, &raport_temp, sizeof(ReportFile)) > 0) {
            if (raport_temp.reportId == id_tinta) { gasit = 1; break; }
            pozitie += sizeof(ReportFile);
        }
        if (gasit) {
            ReportFile raport_urmator;
            while (read(descriptor_fisier, &raport_urmator, sizeof(ReportFile)) > 0) {
                lseek(descriptor_fisier, pozitie, SEEK_SET);
                write(descriptor_fisier, &raport_urmator, sizeof(ReportFile));
                pozitie += sizeof(ReportFile);
                lseek(descriptor_fisier, pozitie + sizeof(ReportFile), SEEK_SET);
            }
            ftruncate(descriptor_fisier, pozitie);
            inregistreaza_operatiune_log(nume_district, nume_utilizator, rol_utilizator, "Sters raport");
            printf("Raportul %d a fost eliminat.\n", id_tinta);
        }
        close(descriptor_fisier);
    }

    /* Parcurge rapoartele și afișează doar ce corespunde criteriilor. */
    else if (strcmp(comanda, "filter") == 0) {
        int descriptor_fisier = open(cale_rapoarte, O_RDONLY);
        ReportFile raport_analizat;
        char camp[50], operator[10], valoare[50];

        while (read(descriptor_fisier, &raport_analizat, sizeof(ReportFile)) > 0) {
            int conditie_ok = 1;
            for (int j = index_filtre; j < argc; j++) {
                parse_condition(argv[j], camp, operator, valoare);
                if (!match_condition(&raport_analizat, camp, operator, valoare)) {
                    conditie_ok = 0; break;
                }
            }
            if (conditie_ok) printf("MATCH -> ID: %d | Categorie: %s | Severitate: %d\n",
                                    raport_analizat.reportId, raport_analizat.issueCategory, raport_analizat.severityLevel);
        }
        close(descriptor_fisier);
    }

    /* Actualizează pragul de severitate.
       Doar managerul are voie să modifice pragul. Înainte de scriere,  verifică dacă cineva a umblat la permisiunile fișierului de configurare. */
    else if (strcmp(comanda, "update_threshold") == 0) {
        if (strcmp(rol_utilizator, "manager") != 0) { printf("Refuzat.\n"); return 1; }

        struct stat st;
        if (stat(cale_config, &st) == 0 && (st.st_mode & 0777) != PERMISIUNI_CONFIG) {
            printf("Alerta Securitate: Permisiunile district.cfg au fost modificate!\n");
            return 1;
        }

        int descriptor_fisier = open(cale_config, O_WRONLY | O_CREAT | O_TRUNC, PERMISIUNI_CONFIG);
        char buffer[50];
        int len = sprintf(buffer, "threshold=%s\n", argv[index_filtre]);
        write(descriptor_fisier, buffer, len);
        close(descriptor_fisier);
        chmod(cale_config, PERMISIUNI_CONFIG);
        printf("Prag severitate actualizat.\n");
    }

    /* Șterge complet un district. Apelează logica definită anterior pentru curățarea directoarelor și a link-urilor. */
    else if (strcmp(comanda, "remove_district") == 0) {
        remove_district(nume_district, rol_utilizator);
    }


    return 0;
}
