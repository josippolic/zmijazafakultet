#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <string.h>

#define WIDTH 40
#define HEIGHT 20
#define MAX_SNAKE 1000
#define MAX_SCORES 500

typedef struct { int x, y; } Point;

typedef struct {
    char name[50];
    char score[10];
    char timeStr[20];
    char lives[10];
    char reason[50];
} ScoreEntry;

/* ================= GLOBALS ================= */
Point snake[MAX_SNAKE];
int snakeLen;
Point food;

int dx = 1, dy = 0;
int score = 0;
int lives = 3;
int delayMs = 150;

int paused = 0;
int returnToMenu = 0;
int restartGame = 0;
int soundOn = 1;

time_t startTime;
char playerName[50] = "";

const char* SCORE_PATH = "C:\\Users\\Dark Prince\\source\\repos\\novazmija5\\x64\\Debug\\score.txt";

/* ================= CONSOLE ================= */
void gotoxy(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void setColor(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

/* ================= SOUND ================= */
void snd(int f, int d) { if (soundOn) Beep(f, d); }
void soundMenu() { snd(659, 150); snd(587, 150); snd(370, 300); snd(415, 300); }
void soundEat() { snd(800, 120); }
void soundWall() { snd(200, 300); }
void soundSelf() { snd(120, 500); }
void soundLifeLost() { snd(300, 150); snd(200, 150); }
void soundPause() { snd(600, 120); }
void soundRestart() { snd(500, 100); snd(700, 100); }

/* ================= SNAKE ================= */
void resetSnake() {
    snakeLen = 3;
    snake[0] = (Point){ WIDTH / 2, HEIGHT / 2 };
    snake[1] = (Point){ WIDTH / 2 - 1, HEIGHT / 2 };
    snake[2] = (Point){ WIDTH / 2 - 2, HEIGHT / 2 };
    dx = 1;
    dy = 0;
}

void placeFood() {
    int ok;
    do {
        ok = 1;
        food.x = rand() % WIDTH;
        food.y = rand() % HEIGHT;

        for (int i = 0; i < snakeLen; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                ok = 0;
                break;
            }
        }
    } while (!ok);
}

/* ================= DRAW ================= */
void drawBorderTop() {
    printf("%c", 201);
    for (int i = 0; i < WIDTH; i++) printf("%c", 205);
    printf("%c\n", 187);
}

void drawBorderBottom() {
    printf("%c", 200);
    for (int i = 0; i < WIDTH; i++) printf("%c", 205);
    printf("%c\n", 188);
}

void drawGame() {
    gotoxy(0, 0);
    setColor(11);

    printf("SNAKE | Score: %04d | Lives: %d | Sound: %-3s",
        score, lives,
        soundOn ? "ON" : "OFF");

    if (paused) printf(" | PAUSED");
    printf("\n");

    printf("Controls: P Pause | M Menu | R Restart | V Mute | WASD / Arrows\n");

    setColor(7);
    drawBorderTop();

    for (int y = 0; y < HEIGHT; y++) {
        printf("%c", 186);
        for (int x = 0; x < WIDTH; x++) {
            int drawn = 0;

            if (x == food.x && y == food.y) {
                setColor(12);
                printf("*");
                setColor(7);
                drawn = 1;
            }

            for (int i = 0; i < snakeLen && !drawn; i++) {
                if (snake[i].x == x && snake[i].y == y) {
                    setColor(i == 0 ? 10 : 2);
                    printf(i == 0 ? "O" : "o");
                    setColor(7);
                    drawn = 1;
                }
            }

            if (!drawn) printf(" ");
        }
        printf("%c\n", 186);
    }

    drawBorderBottom();
}

/* ================= SCORE ================= */
void saveScore(const char* reason) {
    FILE* f = fopen(SCORE_PATH, "a");
    if (!f) return;

    int t = (int)(time(NULL) - startTime);
    fprintf(f, "%s | Score: %04d | Time: %d sec | Lives left: %d | Reason: %s\n",
        playerName, score, t, lives, reason);

    fclose(f);
}

/* ================= HIGHSCORE HELPERS ================= */
int loadScores(ScoreEntry arr[], int maxCount) {
    FILE* f = fopen(SCORE_PATH, "r");
    if (!f) return 0;

    int count = 0;
    char line[256];

    while (count < maxCount && fgets(line, sizeof(line), f)) {
        arr[count].name[0] = 0;
        arr[count].score[0] = 0;
        arr[count].timeStr[0] = 0;
        arr[count].lives[0] = 0;
        arr[count].reason[0] = 0;

        sscanf(line,
            " %49[^|]| Score: %9[^|]| Time: %19[^|]| Lives left: %9[^|]| Reason: %49[^\n]",
            arr[count].name,
            arr[count].score,
            arr[count].timeStr,
            arr[count].lives,
            arr[count].reason);

        count++;
    }

    fclose(f);
    return count;
}

void saveAllScores(ScoreEntry arr[], int count) {
    FILE* f = fopen(SCORE_PATH, "w");
    if (!f) return;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s | Score: %s | Time: %s | Lives left: %s | Reason: %s\n",
            arr[i].name,
            arr[i].score,
            arr[i].timeStr,
            arr[i].lives,
            arr[i].reason);
    }

    fclose(f);
}

void deleteScoreAt(ScoreEntry arr[], int* count, int index) {
    if (index < 0 || index >= *count) return;

    for (int i = index; i < *count - 1; i++) {
        arr[i] = arr[i + 1];
    }

    (*count)--;
    saveAllScores(arr, *count);
}

void clearAllScores() {
    FILE* f = fopen(SCORE_PATH, "w");
    if (f) fclose(f);
}

/* ================= HIGHSCORE ================= */
void showHighscore() {
    ScoreEntry entries[MAX_SCORES];
    int selected = 0;

    while (1) {
        int count = loadScores(entries, MAX_SCORES);

        system("cls");
        printf("=============== HIGHSCORE ===============\n\n");

        if (count <= 0) {
            printf("Nema high score zapisa.\n\n");
            printf("D - obrisi sve\n");
            printf("ESC - povratak\n");

            char c = _getch();
            if (c == 'd' || c == 'D') {
                clearAllScores();
            }
            else if (c == 27) {
                return;
            }
            continue;
        }

        if (selected < 0) selected = 0;
        if (selected >= count) selected = count - 1;

        printf("Strelice gore/dolje = odabir | DEL = obrisi zapis | D = obrisi sve | ESC = izlaz\n\n");

        printf("%-3s %-22s %-8s %-10s %-14s %-20s\n",
            "", "IME", "SCORE", "TIME", "LIVES", "REASON");
        printf("-------------------------------------------------------------------------------\n");

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                setColor(240);
                printf(">  %-22s %-8s %-10s %-14s %-20s\n",
                    entries[i].name,
                    entries[i].score,
                    entries[i].timeStr,
                    entries[i].lives,
                    entries[i].reason);
                setColor(7);
            }
            else {
                printf("   %-22s %-8s %-10s %-14s %-20s\n",
                    entries[i].name,
                    entries[i].score,
                    entries[i].timeStr,
                    entries[i].lives,
                    entries[i].reason);
            }
        }

        char c = _getch();

        if (c == -32 || c == 0) {
            c = _getch();

            if (c == 72 && selected > 0) {
                selected--;
            }
            else if (c == 80 && selected < count - 1) {
                selected++;
            }
            else if (c == 83 && count > 0) {
                system("cls");
                printf("Obrisati zapis za igraca \"%s\"? (Y/N)\n", entries[selected].name);
                char conf = _getch();

                if (conf == 'y' || conf == 'Y') {
                    deleteScoreAt(entries, &count, selected);
                    if (selected >= count && selected > 0) selected--;
                }
            }
        }
        else if (c == 'd' || c == 'D') {
            system("cls");
            printf("Obrisati SVE highscore zapise? (Y/N)\n");
            char conf = _getch();

            if (conf == 'y' || conf == 'Y') {
                clearAllScores();
                selected = 0;
            }
        }
        else if (c == 27) {
            return;
        }
    }
}

/* ================= INPUT ================= */
void inputGame() {
    if (!_kbhit()) return;

    char c = _getch();

    if (c == -32 || c == 0) {
        c = _getch();
        if (!paused) {
            if (c == 72 && dy == 0) { dx = 0; dy = -1; }
            if (c == 80 && dy == 0) { dx = 0; dy = 1; }
            if (c == 75 && dx == 0) { dx = -1; dy = 0; }
            if (c == 77 && dx == 0) { dx = 1; dy = 0; }
        }
        return;
    }

    if (!paused) {
        if ((c == 'w' || c == 'W') && dy == 0) { dx = 0; dy = -1; }
        if ((c == 's' || c == 'S') && dy == 0) { dx = 0; dy = 1; }
        if ((c == 'a' || c == 'A') && dx == 0) { dx = -1; dy = 0; }
        if ((c == 'd' || c == 'D') && dx == 0) { dx = 1; dy = 0; }
    }

    if (c == 'v' || c == 'V') soundOn = !soundOn;
    if (c == 'p' || c == 'P') { paused = !paused; soundPause(); }
    if (c == 'm' || c == 'M') returnToMenu = 1;
    if (c == 'r' || c == 'R') { restartGame = 1; soundRestart(); }
    if (c == 'q' || c == 'Q') exit(0);
}

/* ================= UPDATE ================= */
void updateGame() {
    for (int i = snakeLen - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    snake[0].x += dx;
    snake[0].y += dy;

    if (snake[0].x < 0 || snake[0].x >= WIDTH ||
        snake[0].y < 0 || snake[0].y >= HEIGHT) {
        soundWall();
        soundLifeLost();
        lives--;

        if (lives <= 0) {
            saveScore("Wall collision");
            returnToMenu = 1;
        }

        resetSnake();
        return;
    }

    for (int i = 1; i < snakeLen; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            soundSelf();
            saveScore("Self collision");
            returnToMenu = 1;
            return;
        }
    }

    if (snake[0].x == food.x && snake[0].y == food.y) {
        soundEat();

        if (snakeLen < MAX_SNAKE) {
            snake[snakeLen] = snake[snakeLen - 1];
            snakeLen++;
        }

        score += (score < 100) ? 1 : 2;
        placeFood();
    }
}

/* ================= MENU ================= */
int menu() {
    const char* items[] = {
        "Start game",
        "Unesi ime igraca",
        "Highscore",
        "Izlaz"
    };

    int sel = 0;
    soundMenu();

    while (1) {
        system("cls");
        printf("===== S N A K E =====\n\n");

        for (int i = 0; i < 4; i++) {
            if (i == sel) {
                setColor(240);
                printf(" > %s\n", items[i]);
                setColor(7);
            }
            else {
                printf("   %s\n", items[i]);
            }
        }

        char c = _getch();
        if (c == -32 || c == 0) {
            c = _getch();
            if (c == 72 && sel > 0) sel--;
            if (c == 80 && sel < 3) sel++;
        }
        else if (c == 13) {
            return sel;
        }
    }
}

/* ================= MAIN ================= */
int main() {
    srand((unsigned)time(NULL));
    SetConsoleOutputCP(437);
    SetConsoleCP(437);

    while (1) {
        int choice = menu();

        if (choice == 3) break;

        if (choice == 2) {
            showHighscore();
            continue;
        }

        if (choice == 1) {
            system("cls");
            printf("Unesite ime igraca: ");
            fgets(playerName, 49, stdin);
            playerName[strcspn(playerName, "\n")] = 0;
        }

        if (strlen(playerName) == 0) {
            strcpy(playerName, "Nepoznati igrac");
        }

        score = 0;
        lives = 3;
        paused = 0;
        returnToMenu = 0;
        restartGame = 0;

        startTime = time(NULL);
        resetSnake();
        placeFood();

        CONSOLE_CURSOR_INFO ci = { 1, FALSE };
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        system("cls");

        while (!returnToMenu) {
            if (restartGame) {
                restartGame = 0;
                score = 0;
                lives = 3;
                paused = 0;
                resetSnake();
                placeFood();
            }

            inputGame();
            if (!paused) updateGame();
            drawGame();
            Sleep(delayMs);
        }
    }

    return 0;
}
