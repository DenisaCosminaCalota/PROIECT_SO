#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 1024

void display_prompt() {
    printf("city_hub > ");
    fflush(stdout);
}

// functie care citeste o linie caracter cu caracter din pipe
int citeste_linie(int fd, char *buffer, int lungime_maxima) {
    int bytes_cititi = 0;
    char caracter;

    while (bytes_cititi < lungime_maxima - 1) {
        int rezultat = read(fd, &caracter, 1);

        // daca s-a terminat fluxul sau apare eroare
        if (rezultat <= 0) return bytes_cititi;
        if (caracter == '\n') break;
        buffer[bytes_cititi++] = caracter;
    }
    buffer[bytes_cititi] = '\0';
    return bytes_cititi + 1;
}

// calculeaza scorurile pentru toate districtele primite
void calculate_scores(char *argumente) {
    char *districte[50];
    int nr_districte = 0;

    // separ argumentele in functie de spatiu
    char *token = strtok(argumente, " ");
    while (token != NULL && nr_districte < 50) {
        districte[nr_districte++] = token;
        token = strtok(NULL, " ");
    }

    if (nr_districte == 0) {
        printf("Eroare: Trebuie sa specifici cel putin un district.\n");
        return;
    }

    int pipe_scoreri[50][2];
    pid_t pid_scoreri[50];

    // pentru fiecare district creez un proces scorer
    for (int i = 0; i < nr_districte; i++) {
        if (pipe(pipe_scoreri[i]) == -1) {
            perror("Eroare la crearea pipe-ului");
            return;
        }

        pid_scoreri[i] = fork();
        if (pid_scoreri[i] < 0) {
            perror("Eroare la fork");
            return;
        }

        // proces copil
        if (pid_scoreri[i] == 0) {
            close(pipe_scoreri[i][0]);
            // redirectionez stdout spre pipe
            if (dup2(pipe_scoreri[i][1], STDOUT_FILENO) == -1) {
                perror("Eroare la dup2");
                exit(1);
            }
            close(pipe_scoreri[i][1]);

            // execut programul scorer
            execl("./scorer", "scorer", districte[i], NULL);
            perror("Eroare la executia programului scorer");
            exit(1);
        } else {
            close(pipe_scoreri[i][1]);
        }
    }

    printf("\n RAPORT CENTRALIZAT WORKLOAD \n");

     // citesc rezultatele venite de la fiecare scorer
    for (int i = 0; i < nr_districte; i++) {
        char buffer_text[1024];
        int caractere_citite;

        while ((caractere_citite = read(pipe_scoreri[i][0], buffer_text, sizeof(buffer_text) - 1)) > 0) {
            buffer_text[caractere_citite] = '\0';
            printf("%s", buffer_text);
        }
        close(pipe_scoreri[i][0]);
        waitpid(pid_scoreri[i], NULL, 0);
    }
    printf("\n\n");
}

int main() {
    char input[MAX_CMD_LEN];
    char *command;
    char *args;

    printf("Comenzi disponibile: start_monitor, calculate_scores <districte>, exit\n\n");

    while (1) {
        display_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) break;

        input[strcspn(input, "\n")] = 0;

        char copie_input[MAX_CMD_LEN];
        strcpy(copie_input, input);

        // extrag comanda introdusa
        command = strtok(input, " ");
        if (command == NULL) continue;

        if (strcmp(command, "exit") == 0) {
            printf("Inchidere City Hub\n");
            break;
        }
        // porneste procesul monitor
        else if (strcmp(command, "start_monitor") == 0) {
            printf("[HUB] Se incearca pornirea monitorului\n");
            pid_t hub_mon_pid = fork();
            if (hub_mon_pid < 0) {
                printf("Eroare la fork\n");
            }

            // proces copil pentru monitor
            if (hub_mon_pid == 0) {
                int pipe_monitor[2];
                if (pipe(pipe_monitor) == -1) {
                    printf("Eroare la pipe\n");
                    exit(1);
                }

                pid_t monitor_pid = fork();
                if (monitor_pid < 0) {
                    printf("Eroare la fork monitor\n");
                    exit(1);
                }

                // procesul care executa monitorul
                if (monitor_pid == 0) {

                    close(pipe_monitor[0]);

                    // trimit stdout in pipe
                    if (dup2(pipe_monitor[1], STDOUT_FILENO) == -1) {
                        perror("Eroare dup2");
                        exit(1);
                    }
                    close(pipe_monitor[1]);

                    execl("./monitor_reports", "monitor_reports", NULL);
                    perror("Eroare la lansarea monitor_reports");
                    exit(1);
                }

                close(pipe_monitor[1]);

                char buffer_linie[512];

                // citesc mesajele venite de la monitor
                while (citeste_linie(pipe_monitor[0], buffer_linie, sizeof(buffer_linie)) > 0) {

                    if (strncmp(buffer_linie, "ERROR:", 6) == 0) {
                        printf("\n[HUB ALERT] Instanta respinsa: %s\n", buffer_linie + 6);
                        fflush(stdout);
                    }
                    else if (strncmp(buffer_linie, "ALERT:", 6) == 0) {
                        printf("\n[MONITOR ALERT] Incident semnalat: %s\n", buffer_linie + 6);
                        fflush(stdout);
                    }
                    else {
                        printf("\n[MONITOR LOG] %s\n", buffer_linie);
                        fflush(stdout);
                    }
                }

                close(pipe_monitor[0]);
                printf("\n[HUB INFO] Procesul monitor s-a terminat.\n");
                fflush(stdout);
                exit(0);
            }
        }

        // calculeaza scorurile pentru districtele date
        else if (strcmp(command, "calculate_scores") == 0) {
            args = copie_input + strlen(command) + 1;

            while (*args == ' ') args++;

            if (strlen(args) == 0) {
                printf("Eroare: Trebuie sa specifici cel putin un district.\n");
            } else {
                printf("[HUB] Se calculeaza scorurile pentru: %s\n", args);
                calculate_scores(args);
            }
        }
        else {
            printf("Comanda necunoscuta: %s\n", command);
        }
    }

    return 0;
}
