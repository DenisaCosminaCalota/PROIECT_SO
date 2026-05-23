#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define PID_FILE ".monitor_pid"

volatile sig_atomic_t keep_running = 1;

void handle_signal(int signal) {
    if (signal == SIGINT) {
        const char mesaj[] = "\n[MONITOR] SIGINT primit. Se sterge fisierul si se inchide.\n";
        write(STDOUT_FILENO, mesaj, strlen(mesaj));
        keep_running = 0;
    }
    if (signal == SIGUSR1) {
        const char mesaj[] = "[MONITOR] Un nou raport a fost detectat.\n";
        write(STDOUT_FILENO, mesaj, strlen(mesaj));
    }
}

int main() {
    int f_pid_citire = open(PID_FILE, O_RDONLY);
    if (f_pid_citire >= 0) {
        char buf_pid[16];
        ssize_t bytes_cititi = read(f_pid_citire, buf_pid, sizeof(buf_pid) - 1);
        close(f_pid_citire);

        if (bytes_cititi > 0) {
            buf_pid[bytes_cititi] = '\0';
            pid_t pid_existent = atoi(buf_pid);

            if (kill(pid_existent, 0) == 0) {

                printf("ERROR: Un monitor ruleaza deja cu PID-ul %d\n", pid_existent);
                fflush(stdout);
                return 1;
            }
        }
    }
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handle_signal;

    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        perror("Eroare sigaction SIGUSR1");
        exit(-1);
    }
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("Eroare sigaction SIGINT");
        exit(-1);
    }

    pid_t pid = getpid();

    int f_pid = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f_pid == -1) {
        perror("Eroare la crearea fisierului PID");
        exit(EXIT_FAILURE);
    }

    char pidString[16];
    int lungime = snprintf(pidString, sizeof(pidString), "%d", pid);

    if (write(f_pid, pidString, lungime) == -1) {
        perror("Eroare la scrierea in fisier");
    }
    close(f_pid);

    printf("[MONITOR] Pornit cu PID: %d. Astept semnale...\n", pid);

    while (keep_running) {
        pause();
    }

    unlink(PID_FILE);

    return 0;
}
