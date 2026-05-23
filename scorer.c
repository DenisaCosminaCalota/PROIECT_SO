#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXIM_CARACTERE 100
#define NUMAR_MAXIM_INSPECTORI 100

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

typedef struct {
    char nume_inspector[MAXIM_CARACTERE];
    int scor_total;
}ScorInspector;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Utilizare: %s <nume_district>\n", argv[0]);
        return 1;
    }

    char cale_rapoarte[512];
    sprintf(cale_rapoarte, "%s/reports.dat", argv[1]);
    int fd= open(cale_rapoarte, O_RDONLY);
    if(fd==-1){
        printf(" District: %s \nEroare: Nu s-a putut deschide fisierul sau nu exista rapoarte.\n", argv[1]);
        return 0;
    }

    int contorInspectori = 0;
    ScorInspector lista[NUMAR_MAXIM_INSPECTORI];

    memset(lista, 0, sizeof(lista));
    ReportFile r;
    while(read(fd, &r, sizeof(ReportFile))== sizeof(ReportFile)){
        int gasit =-1;
        for(int i=0; i<contorInspectori; i++){
            if(strcmp(lista[i].nume_inspector, r.inspectorName)== 0){
                gasit= i;
                break;
            }
        }
        if (gasit != -1) {
            lista[gasit].scor_total += r.severityLevel;
        } else {
            if (contorInspectori < NUMAR_MAXIM_INSPECTORI) {
                strcpy(lista[contorInspectori].nume_inspector, r.inspectorName);
                lista[contorInspectori].scor_total = r.severityLevel;
                contorInspectori++;
            }
        }
    }
    close(fd);

    printf("District curent analizat: %s\n", argv[1]);
    if (contorInspectori == 0) {
        printf("  Nu s-au gasit inspectori activi in acest district.\n");
    } else {
        for (int i = 0; i < contorInspectori; i++) {
            printf("  Inspector: %s | Workload Score: %d\n",
                   lista[i].nume_inspector, lista[i].scor_total);
        }
    }
    printf("\n");


    return 0;
}
