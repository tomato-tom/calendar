#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>
#include <string.h>

#define RESET     "\033[0m"
#define RED       "\033[31m"
#define GREEN     "\033[32m"
#define REVERSE   "\033[7m"
#define UNDERLINE "\033[4m"
#define BOLD      "\033[1m"
#define CLEAR     printf("\033[2J\033[1;1H")

#define MAX_LOG_FILE_SIZE 1048576  // 1MB

void log_message(const char *level, const char *message, FILE *logfile) {
    time_t now;
    time(&now);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character
    fprintf(logfile, "[%s] %s: %s\n", timestamp, level, message);
}

void rotate_log_file(FILE **logfile) {
    fseek(*logfile, 0, SEEK_END);
    long filesize = ftell(*logfile);
    if (filesize >= MAX_LOG_FILE_SIZE) {
        fclose(*logfile);
        rename("log.txt", "log_old.txt");
        *logfile = fopen("log.txt", "a");
        if (*logfile == NULL) {
            printf("ERROR: Could not open new log file\n");
            exit(1);
        }
        log_message("INFO", "Log file rotated", *logfile);
    }
}

int get_first_day_of_month(int year, int month) {
    if (month < 3) {
        month += 12;
        year--;
    }
    return (((13 * (month + 1)) / 5) + year + (year / 4) - (year / 100) + (year / 400)) % 7;
}

int get_days_in_month(int year, int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && (year % 4 == 0 && year % 100 != 0 || year % 400 == 0))
        return 29;
    return days[month - 1];
}

int has_event(sqlite3 *db, int year, int month, int day) {
    char sql[200];
    sqlite3_stmt *res;

    sprintf(sql, "SELECT COUNT(*) FROM events WHERE date = '%04d-%02d-%02d';", year, month, day);

    int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(res) == SQLITE_ROW) {
            int count = sqlite3_column_int(res, 0);
            sqlite3_finalize(res);
            return count > 0;
        }
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(res);
    return 0;
}

void print_calendar(int year, int month, int day, sqlite3 *db) {
    char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int first_day = get_first_day_of_month(year, month);
    int days = get_days_in_month(year, month);

    printf(GREEN "\n     %s %d\n\n" RESET, months[month - 1], year);
    printf(RED " Su" RESET " Mo Tu We Th Fr Sa\n");

    for (int i = 0; i < first_day; i++)
        printf("   ");

    for (int d = 1; d <= days; d++) {
        printf(" ");
        if ((d + first_day - 1) % 7 == 0) // 日曜日
            printf(RED);
        if (has_event(db, year, month, d)) { // 予定がある日は下線付きで表示
            printf(UNDERLINE);
        }
        if (d == day)
            printf(REVERSE);
        printf("%2d", d);
        printf(RESET);

        if ((d + first_day) % 7 == 0 || d == days)
            printf("\n");
    }
}

void get_user_date(int *year, int *month) {
    int y, m;
    printf("年を入力してください: ");
    scanf("%d", &y);
    printf("月を入力してください (1-12): ");
    scanf("%d", &m);

    if (m < 1 || m > 12) {
        printf("無効な月です。1から12の間で入力してください。\n");
        return;
    } else {
        *month = m;
        *year = y;
    }
}

void update_date(int *year, int *month, int direction) {
    *month += direction;
    if (*month > 12) {
        *month = 1;
        (*year)++;
    } else if (*month < 1) {
        *month = 12;
        (*year)--;
    }
}

void add_event(sqlite3 *db, int year, int month) {
    char sql[200];
    char *err_msg = 0;
    char memo[100];
    int day;

    printf("日付を入力してください: ");
    scanf("%d", &day);
    printf("予定を入力してください: ");
    scanf(" %[^\n]", memo);
    
    sprintf(sql, "INSERT INTO events (date, memo) VALUES ('%04d-%02d-%02d', '%s');", year, month, day, memo);
    
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("イベントが追加されました\n");
    }
}

void show_events(sqlite3 *db, int year, int month) {
    char sql[200];
    int day;
    sqlite3_stmt *res;

    printf("日付を入力してください (0で月の予定一覧): ");
    scanf("%d", &day);

    if (day == 0) {
        // 月指定でイベント確認
        sprintf(sql, "SELECT date, memo FROM events WHERE date LIKE '%04d-%02d%%' ORDER BY date ASC;", year, month);
    } else {
        // 日付指定でイベント確認
        sprintf(sql, "SELECT memo FROM events WHERE date = '%04d-%02d-%02d';", year, month, day);
    }

    int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc == SQLITE_OK) {
        if (day == 0) {
            printf("日付\t\t予定\n");
            while(sqlite3_step(res) == SQLITE_ROW) {
                printf("%s  %s\n", sqlite3_column_text(res, 0), sqlite3_column_text(res, 1));
            }
        } else {
            printf("%d年%d月%d日の予定:\n", year, month, day);
            while(sqlite3_step(res) == SQLITE_ROW) {
                printf("%s\n", sqlite3_column_text(res, 0));
            }
        }
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(res);
}

int main() {
    int current_year, current_month, current_day;
    int day, year, month;
    char choice[10];

    sqlite3 *db;
    char sql[200];
    char *err_msg = 0;

    FILE *logfile = fopen("log.txt", "a");
    if (logfile == NULL) {
        printf("ERROR: Could not open log file\n");
        return 1;
    }

    rotate_log_file(&logfile);

    log_message("INFO", "Starting the program...", logfile);
    log_message("INFO", "Open database...", logfile);

    int rc = sqlite3_open("event.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    log_message("INFO", "Create table...", logfile);
    sprintf(sql, "CREATE TABLE IF NOT EXISTS events(id INTEGER PRIMARY KEY, date TEXT, memo TEXT);");
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    current_year = now->tm_year + 1900;
    current_month = now->tm_mon + 1;
    current_day = now->tm_mday;


    CLEAR;
    printf("現在の年月日: %d年 %d月 %d日\n", current_year, current_month, current_day);
    print_calendar(current_year, current_month, current_day, db);

    year = current_year;
    month = current_month;

    while(1) {
        printf("\n次の月(n)、前の月(p)、予定追加(a)、予定確認(c)、年月指定(s)、終了(q): ");
        int c = getchar();

        switch(c) {
            case 'n':
                log_message("INFO", "Next month selected", logfile);
                update_date(&year, &month, 1);
                break;
            case 'p':
                log_message("INFO", "Previous month selected", logfile);
                update_date(&year, &month, -1);
                break;
            case 's':
                log_message("INFO", "Specific date selected", logfile);
                get_user_date(&year, &month);
                break;
            case 'a':
                log_message("INFO", "Add event selected", logfile);
                add_event(db, year, month);
                break;
            case 'c':
                log_message("INFO", "Check events selected", logfile);
                show_events(db, year, month);
                break;
            case 'q':
                printf("プログラムを終了します。\n");
                log_message("INFO", "Close the program...", logfile);
                sqlite3_close(db);
                fclose(logfile);
                return 0;
            default:
                printf("無効な入力です。\n");
                continue;
        }

        // 今の所本日の日付の印にしか使ってない
        if (year == current_year && month == current_month) {
            day = current_day;
        } else {
            day = 0;
        }

        if (c == 'n' || c == 'p' || c == 's') {
            CLEAR;
            print_calendar(year, month, day, db);
        }

        // 入力バッファをクリアする
        while (getchar() != '\n');
    }

    log_message("INFO", "Close the program...", logfile);
    sqlite3_close(db);
    fclose(logfile);
    return 0;
}

