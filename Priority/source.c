#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>

// Fungsi untuk mengirim notifikasi menggunakan NFC
void NFC(const char *message) {
    char notifie[512];
    snprintf(notifie, sizeof(notifie),
             "am start -a android.intent.action.MAIN "
             "-e toasttext \"%s\" -n bellavita.toast/.MainActivity > /dev/null 2>&1",
             message);
    system(notifie);
}

// Fungsi untuk menyesuaikan I/O priority ke tinggi
void adjust_ionice_high(const char *pid) {
    char command[256];
    snprintf(command, sizeof(command), "ionice -t -c 0 -n 7 -p %s", pid);
    system(command);
}

// Fungsi untuk menyesuaikan I/O priority ke rendah
void adjust_ionice_low(const char *pid) {
    char command[256];
    snprintf(command, sizeof(command), "ionice -t -c 2 -n 2 -p %s", pid);
    system(command);
}

// Fungsi untuk memproses ID proses
void ps_proses(const char *proses_name) {
    char ponc[512];
    char cmd[512];
    char buffer[2056];
    FILE *file;

    file = popen("ps | grep -o \".*[.][a-z]*\" | grep system | awk '{print $9}'", "r");
    if (file == NULL) {
        printf("Proses tidak ditemukan!\n");
        return;
    }

    while (fgets(ponc, sizeof(ponc), file) != NULL) {
        // Menghapus newline di akhir string
        ponc[strcspn(ponc, "\n")] = '\0';

        snprintf(cmd, sizeof(cmd), "pgrep -f %s", ponc);
        system(cmd);

        FILE *fp_id = popen(cmd, "r");
        if (fp_id == NULL) {
            continue;
        }
        while (fgets(buffer, sizeof(buffer), fp_id) != NULL) {
            // Menghapus newline di akhir string
            buffer[strcspn(buffer, "\n")] = '\0';
            snprintf(cmd, sizeof(cmd), "ionice -c 0 -n 7 -p %s", buffer);
            system(cmd);
        }
        pclose(fp_id);
    }
    pclose(file);
}

// Fungsi untuk memeriksa apakah aplikasi sedang berjalan
int check_app_running(const char *app_name) {
    char command[512];
    snprintf(command, sizeof(command),
             "dumpsys SurfaceFlinger | grep Output | head -n 1 | cut -f1 -d/ | awk -F '(' '{print $2}' | grep -w \"%s\"",
             app_name);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        return 0;
    }

    char buffer[256];
    int is_running = 0;

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        is_running = 1;
    }

    pclose(fp);
    return is_running;
}

int main() {
    NFC("♨️ I/O Priority is running the background");
    sleep(1);
    NFC("♨️ I/O Priorty game: By @Yeye_nat");
    const char *primary_path = "/sdcard/Priority/gamelist.txt";
    char line[1024];
    char app_in_running[1024] = "";

    while (1) {
        FILE *file = fopen(primary_path, "r");

        if (file == NULL) {
            static int error_count = 0;
            if (error_count % 10 == 0) {
                printf("File gamelist.txt tidak ditemukan!\n");
            }
            error_count++;
            sleep(1);
            continue;
        }

        int app_found = 0;

        while (fgets(line, sizeof(line), file) != NULL) {
            line[strcspn(line, "\n")] = '\0';

            if (check_app_running(line)) {
                app_found = 1;

                if (strcmp(line, app_in_running) != 0) {
                    char message[512];
                    snprintf(message, sizeof(message),
                             "♨️ I/O Priority High : game %s", line);
                    NFC(message);
                    strcpy(app_in_running, line);

                    char cmd[512];
                    snprintf(cmd, sizeof(cmd), "pgrep -f %s", line);

                    FILE *pid_fp = popen(cmd, "r");
                    if (pid_fp != NULL) {
                        char pid[16];
                        if (fgets(pid, sizeof(pid), pid_fp) != NULL) {
                            pid[strcspn(pid, "\n")] = '\0';
                            adjust_ionice_high(pid);
                            ps_proses(line);
                        }
                        pclose(pid_fp);
                    }
                }
                break;
            }
        }
        fclose(file);

        if (!app_found) {
            if (strlen(app_in_running) > 0) {
                NFC("♨️ I/O Priority Low : game close");
                strcpy(app_in_running, "");

                char cmd[512];
                snprintf(cmd, sizeof(cmd), "pgrep -f %s", app_in_running);

                FILE *pid_fp = popen(cmd, "r");
                if (pid_fp != NULL) {
                    char pid[16];
                    if (fgets(pid, sizeof(pid), pid_fp) != NULL) {
                        pid[strcspn(pid, "\n")] = '\0';
                        adjust_ionice_low(pid);
                    }
                    pclose(pid_fp);
                }
            }
        }

        sleep(2);
    }

    return 0;
}