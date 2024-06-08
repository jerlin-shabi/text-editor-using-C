#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <sqlite3.h>

#define MAX_ROWS 100
#define MAX_COLS 100

char text[MAX_ROWS][MAX_COLS]; // Buffer to store text

// Initialize the text buffer with spaces
void initializeTextBuffer() {
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j < MAX_COLS; j++) {
            text[i][j] = ' ';
        }
    }
}

// Display the text buffer on the screen
void displayTextBuffer() {
    for (int i = 0; i < MAX_ROWS; i++) {
        move(i, 0);
        for (int j = 0; j < MAX_COLS; j++) {
            addch((chtype)text[i][j]);
        }
    }
}

// Create the SQLite database and table if not already existing
int createDatabase(sqlite3 *db) {
    const char *sql = "CREATE TABLE IF NOT EXISTS documents (id INTEGER PRIMARY KEY, content TEXT);";
    char *errMsg = 0;

    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return rc;
    }
    return SQLITE_OK;
}

// Save the content to the SQLite database
int saveToDatabase(sqlite3 *db, int id, const char *content) {
    const char *sql = "INSERT INTO documents (id, content) VALUES (?, ?) "
                      "ON CONFLICT(id) DO UPDATE SET content=excluded.content;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, content, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    return rc;
}

// Load content from the SQLite database by ID
char *loadFromDatabase(sqlite3 *db, int id) {
    const char *sql = "SELECT content FROM documents WHERE id = ?;";
    sqlite3_stmt *stmt;
    char *content = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        content = strdup((const char *)sqlite3_column_text(stmt, 0));
    } else {
        fprintf(stderr, "No data found\n");
    }

    sqlite3_finalize(stmt);
    return content;
}

// Get a new unique document ID from the SQLite database
int getNewDocumentId(sqlite3 *db) {
    const char *sql = "SELECT IFNULL(MAX(id), 0) + 1 FROM documents;";
    sqlite3_stmt *stmt;
    int newId = 0;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        newId = sqlite3_column_int(stmt, 0);
    } else {
        fprintf(stderr, "Failed to get new document ID\n");
    }

    sqlite3_finalize(stmt);
    return newId;
}

int main(int argc, char *argv[]) {
    sqlite3 *db;
    int rc = sqlite3_open("text_editor.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = createDatabase(db);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }

    initscr();            // Initialize the screen
    cbreak();             // Disable line buffering
    keypad(stdscr, TRUE); // Enable function keys
    noecho();             // Disable echoing of characters typed

    initializeTextBuffer();
    int ch, x = 0, y = 0;
    int docId = -1;

    if (argc == 3 && strcmp(argv[1], "load") == 0) {
        docId = atoi(argv[2]);
        char *content = loadFromDatabase(db, docId);
        if (content) {
            // Load content into text buffer
            int row = 0, col = 0;
            for (int i = 0; i < strlen(content) && row < MAX_ROWS; i++) {
                if (content[i] == '\n') {
                    row++;
                    col = 0;
                } else if (col < MAX_COLS) {
                    text[row][col++] = content[i];
                }
            }
            free(content);
        }
    } else if (argc == 2 && strcmp(argv[1], "new") == 0) {
        docId = getNewDocumentId(db);
        if (docId == -1) {
            endwin();
            fprintf(stderr, "Failed to create new document\n");
            sqlite3_close(db);
            return 1;
        }
    } else {
        endwin();
        fprintf(stderr, "Usage: %s [new|load <id>]\n", argv[0]);
        sqlite3_close(db);
        return 1;
    }

    displayTextBuffer();
    move(y, x);

    while ((ch = getch()) != 27) { // F1 key to save and exit
        switch (ch) {
            case KEY_UP:
                if (y > 0) y--;
                break;
            case KEY_DOWN:
                if (y < MAX_ROWS - 1) y++;
                break;
            case KEY_LEFT:
                if (x > 0) x--;
                break;
            case KEY_RIGHT:
                if (x < MAX_COLS - 1) x++;
                break;
            case 10: // Enter key
                if (y < MAX_ROWS - 1) {
                    y++;
                    x = 0;
                }
                break;
            case 127: // Backspace key (127 is the ASCII code for backspace)
            case KEY_BACKSPACE: // Handle both backspace and delete keys
                if (x > 0) {
                    x--;
                    text[y][x] = ' ';
                } else if (y > 0) {
                    y--;
                    x = MAX_COLS - 1;
                    while (x > 0 && text[y][x] == ' ') {
                        x--;
                    }
                    text[y][x] = ' ';
                }
                break;
            default:
                if (x < MAX_COLS) {
                    text[y][x++] = ch;
                }
                break;
        }
        clear();
        displayTextBuffer();
        move(y, x);
    }

    endwin(); // Restore terminal settings

    // Convert text buffer to string
    char buffer[MAX_ROWS * MAX_COLS + 1] = {0};
    for (int i = 0; i < MAX_ROWS; i++) {
        strncat(buffer, text[i], MAX_COLS);
        if (i < MAX_ROWS - 1) strcat(buffer, "\n");
    }

    if (docId != -1) {
        saveToDatabase(db, docId, buffer);
    }

    sqlite3_close(db);
    return 0;
}
