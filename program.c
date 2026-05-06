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


int parse_condition(const char *input, char *field, char *op, char *value) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3;
}

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
        execlp("rm", "rm", "-rf", nume_district, NULL);
        perror("Eroare la execlp");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);

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

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) rol_utilizator = argv[++i];
        else if (strcmp(argv[i], "--user") == 0) nume_utilizator = argv[++i];
        else if (comanda == NULL) comanda = argv[i];
        else if (nume_district == NULL) { nume_district = argv[i]; index_filtre = i + 1; }
    }

    if (!rol_utilizator || !comanda || !nume_district) {
        printf("Eroare: Lipsesc argumente obligatorii.\n");
        return 1;
    }

    char cale_rapoarte[256], cale_config[256];
    sprintf(cale_rapoarte, "%s/reports.dat", nume_district);
    sprintf(cale_config, "%s/district.cfg", nume_district);

    if (strcmp(comanda, "add") == 0) {

        mkdir(nume_district, PERMISIUNI_DIRECTOR);
        int descriptor_fisier = open(cale_rapoarte, O_WRONLY | O_CREAT | O_APPEND, PERMISIUNI_RAPOARTE);

        ReportFile raport_nou;
        memset(&raport_nou, 0, sizeof(ReportFile));

        printf("ID Raport: "); scanf("%d", &raport_nou.reportId);
        printf("Coordonate GPS (Lat Lon): "); scanf("%lf %lf", &raport_nou.GPSCoordinates.latitude, &raport_nou.GPSCoordinates.longitude);
        printf("Categorie: "); scanf("%s", raport_nou.issueCategory);
        printf("Severitate: "); scanf("%d", &raport_nou.severityLevel);
        getchar(); // Clear buffer
        printf("Descriere: "); fgets(raport_nou.descriptionText, MAXIM_CARACTERE, stdin);
        raport_nou.descriptionText[strcspn(raport_nou.descriptionText, "\n")] = 0;

        strcpy(raport_nou.inspectorName, nume_utilizator);
        raport_nou.timestamp = time(NULL);

        write(descriptor_fisier, &raport_nou, sizeof(ReportFile));
        close(descriptor_fisier);
        chmod(cale_rapoarte, PERMISIUNI_RAPOARTE);

        char nume_link[256]; sprintf(nume_link, "active_reports-%s", nume_district);
        unlink(nume_link); symlink(cale_rapoarte, nume_link);

        inregistreaza_operatiune_log(nume_district, nume_utilizator, rol_utilizator, "Adaugat raport nou");
    }

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
    else if (strcmp(comanda, "remove_district") == 0) {
        remove_district(nume_district, rol_utilizator);
    }

    return 0;
}
