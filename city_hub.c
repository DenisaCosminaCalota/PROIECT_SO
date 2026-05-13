#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

void display_prompt() {
    printf("city_hub > ");
    fflush(stdout);
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

        command = strtok(input, " ");
        if (command == NULL) continue;

        if (strcmp(command, "exit") == 0) {
            printf("Inchidere City Hub\n");
            break;
        }
        else if (strcmp(command, "start_monitor") == 0) {
            printf("[HUB] Se incearca pornirea monitorului\n");
            pid_t hub_mon_pid = fork();
            if(hub_mon_pid < 0){
                printf("Eroare la fork");
            }
            if(hub_mon_pid == 0){
                int pipefd[2];
                if(pipe(pipefd) == -1){
                    printf("Eroare la pipe");
                }

            }
        }
        else if (strcmp(command, "calculate_scores") == 0) {
            args = strtok(NULL, "");
            if (args == NULL) {
                printf("Eroare: Trebuie sa specifici cel putin un district.\n");
            } else {
                printf("[HUB] Se calculeaza scorurile pentru: %s\n", args);
            }
        }
        else {
            printf("Comanda necunoscuta: %s\n", command);
        }
    }

    return 0;
}
