#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
/* Used constants. */
/* =============== */

# define MAX_BYTES 1024
# define SHELL_IS_RUNNING 1
# define SCRIPTING 1

/* Setting up needed structs. */
/* ========================== */

enum shellStates {
    STATE_NEUTRAL,
    STATE_WANT_THEN,
    STATE_THEN_BLOCK,
    STATE_ELSE_BLOCK
};

enum commandResults {
    RESULT_SUCCESS,
    RESULT_FAIL
};

typedef struct Variable {
    char *key;
    char *value;
} Variable;

typedef struct Node {
    Variable *data;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;
} LinkedList;

typedef struct CommandHistory {
    char command[MAX_BYTES];
    struct CommandHistory *next;
    struct CommandHistory *prev;
} CommandHistory;

typedef struct {
    CommandHistory *head;
    CommandHistory *tail;
    CommandHistory *current;
} CommandHistoryList;

/* Functions used for the shell implementation. */
/* ============================================ */

void addVariable(LinkedList *list, Variable *variable);
void handleReadCommand(char *variableName);
void changeDirectory(char *path);
void handleCtrlC(int signal);
void dismantleCommand(char *command);
void navigateHistory(CommandHistoryList *history, int direction);
void addCommandToHistory(CommandHistoryList *history, char *command);

char *concatenateStrings(const char *str1, const char *str2);
char *findVariableValue(char *key);
char **detectPiping(char **arguments);

int countWords(char *str);
int handleRedirection(char **arguments, char **outputFile, int size);
int getArgumentsNum(char **arguments);
int executeCommand(char **arguments);
int handleControlFlowCommand(char **arguments);
int isControlFlowCommand(char *command);
int isValidExecution();
int processCommand(char **arguments);

