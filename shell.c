#include "shell.h"

// Global variables used for the shell implementation.

char *prompt;
char previousCommand[MAX_BYTES];
char tempCommand[MAX_BYTES];
char currentCommand[MAX_BYTES];
char *arguments[MAX_BYTES];

int currentState = STATE_NEUTRAL;
int currentResult = RESULT_SUCCESS;

int fileDescriptors[2],
    lastCommandStatus = -1,
    commandStatus = 0,
    pipeCount = 0,
    PIPE_WRITER = 1,
    PIPE_READER = 0,
    commandCounter = 0,
    stdoutFileDescriptor,
    mainProcessId,
    lastStatus = 0;

pid_t runningProcess = -1;
CommandHistory *commandHistoryRoot;
LinkedList variables;

void addCommandToHistory(CommandHistoryList *history, char *command) {
    CommandHistory *newCommand = (CommandHistory *)malloc(sizeof(CommandHistory));
    strcpy(newCommand->command, command);
    newCommand->next = NULL;
    newCommand->prev = history->tail;

    if (history->tail) {
        history->tail->next = newCommand;
    } else {
        history->head = newCommand;
    }
    history->tail = newCommand;
    history->current = NULL; // Reset current pointer
}

void navigateHistory(CommandHistoryList *history, int direction) {
    if (direction == 1) { // Up arrow key
        if (history->current == NULL) {
            history->current = history->tail;
        } else if (history->current->prev != NULL) {
            history->current = history->current->prev;
        }
    } else if (direction == -1) { // Down arrow key
        if (history->current != NULL && history->current->next != NULL) {
            history->current = history->current->next;
        } else {
            history->current = NULL; // Reset to allow new command input
        }
    }

    if (history->current != NULL) {
        strcpy(currentCommand, history->current->command);
    } else {
        currentCommand[0] = '\0';
    }
}

// Adds a variable to the linked list.
void addVariable(LinkedList *list, Variable *variable) {
    Node *newNode = (Node *) malloc(sizeof(Node));

    newNode->data = variable;
    newNode->next = list->head;
    list->head = newNode;
}

// Concatenates two strings.
char *concatenateStrings(const char *firstString, const char *secondString) {
    size_t firstLength = strlen(firstString), secondLength = strlen(secondString);
    char *result = (char *) malloc(firstLength + secondLength + 1);

    if (!result)
        return NULL;

    memcpy(result, firstString, firstLength);
    memcpy(result + firstLength, secondString, secondLength + 1);

    return result;
}

// Finds the value of a variable by its key.
char *findVariableValue(char *key) {
    Node *currentNode = variables.head;

    while (currentNode) {
        if (!strcmp(currentNode->data->key, key)) {
            return currentNode->data->value;
        }

        currentNode = currentNode->next;
    }

    return NULL;
}

// Handles the 'read' command to read input into a variable.
void handleReadCommand(char *variableName) {
    char input[MAX_BYTES];
    printf("%s: ", variableName);
    char *environmentVariable = concatenateStrings("$", variableName);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    Variable *variable = (Variable *) malloc(sizeof(Variable));
    variable->key = strdup(environmentVariable);
    variable->value = strdup(input);

    addVariable(&variables, variable);
}

// Counts the number of words in a string
int countWords(char *string) {
    int count = 0;

    if (!*string) {
        return 0;
    }

    while (*string) {
        if (*string == ' ') {
            count++;
        }

        string++;
    }

    return (count + 1);
}

// Changes the current working directory.
void changeDirectory(char *path) {
    if (chdir(path) != 0) {
        printf("Failed to change directory to %s\n", path);
    }

    printf("Directory has been successfully changed to %s\n.", path);
}

// Handles the SIGINT signal (Control + C).
void handleCtrlC(int signal) {
    strcpy(currentCommand, "^C");

    if (signal == SIGTSTP) {
        exit(0);
    }

    if (getpid() == mainProcessId) {
        printf("\nYou typed Control-C!\n");
        write(STDIN_FILENO, prompt, strlen(prompt) + 1);
        write(STDIN_FILENO, " ", 1);
    }
}

// Handles input/output redirection.
int handleRedirection(char **arguments, char **outputFile, int size) {
    if (size >= 2 && (!strcmp(arguments[size - 2], ">") || !strcmp(arguments[size - 2], ">>"))) {
        *outputFile = arguments[size - 1];
        return STDOUT_FILENO;
    }
    else if (size >= 2 && !strcmp(arguments[size - 2], "2>")) {
        *outputFile = arguments[size - 1];
        return STDERR_FILENO;
    }
    else if (size >= 2 && !strcmp(arguments[size - 2], "<")) {
        *outputFile = arguments[size - 1];
        return STDIN_FILENO;
    }

    return -1;
}

// Dismantling a given command with the delimeter " ".
void dismantleCommand(char *command) {
    char *token = strtok(command, " ");
    int i = 0;

    while (token != NULL) {
        arguments[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    arguments[i] = NULL;
}

// Detects a pipe ('|') in the arguments.
char **detectPiping(char **arguments) {
    char **p = arguments;

    while (*p != NULL) {
        if (strcmp(*p, "|") == 0) {
            return p;
        }

        p++;
    }

    return NULL;
}

// Counts the number of arguments.
int getArgumentsNum(char **arguments) {
    char **p = arguments;
    int count = 0;

    while (*p != NULL) {
        p++;
        count++;
    }

    return count;
}

// Executes a given command.
int executeCommand(char **arguments) {
    char *outputFile;
    int argCount = getArgumentsNum(arguments), fd, backgroundProcess, result = -1;
    int hasPipe = 0;

    char **pipePointer = detectPiping(arguments);
    int pipeFd[2];

    if (pipePointer != NULL) {
        hasPipe = 1;
        *pipePointer = NULL;
        argCount = getArgumentsNum(arguments);

        pipe(pipeFd);

        if (fork() == 0) {
            close(pipeFd[PIPE_WRITER]);
            close(STDIN_FILENO);
            dup(pipeFd[PIPE_READER]);
            executeCommand(pipePointer + 1);
            exit(0);
        }

        stdoutFileDescriptor = dup(STDOUT_FILENO);
        dup2(pipeFd[PIPE_WRITER], STDOUT_FILENO);
    }

    if (arguments[0] == NULL) {
        return 0;
    }

    if (!strcmp(arguments[0], "read")) {
        handleReadCommand(arguments[1]);

        return 0;
    }

    if (!strcmp(arguments[0], "cd")) {
        changeDirectory(arguments[1]);

        return 0;
    }

    if (!strcmp(arguments[0], "prompt")) {
        free(prompt);
        prompt = strdup(arguments[2]);

        return 0;
    }

    if (!strcmp(arguments[0], "!!")) {
        strcpy(tempCommand, previousCommand);
        dismantleCommand(tempCommand);
        executeCommand(arguments);

        return 0;
    }

    if (arguments[0][0] == '$' && argCount >= 3) {
        Variable *variable = (Variable *) malloc(sizeof(Variable));
        variable->key = strdup(arguments[0]);
        variable->value = strdup(arguments[2]);
        addVariable(&variables, variable);

        return 0;
    }

    if (!strcmp(arguments[0], "echo")) {
        char **echoArguments = arguments + 1;

        if (!strcmp(*echoArguments, "$?")) {
            printf("%d\n", commandStatus);

            return 0;
        }

        while (*echoArguments) {
            if (*echoArguments[0] == '$') {
                char *value = findVariableValue(*echoArguments);

                if (value != NULL) {
                    printf("%s ", value);
                }
            } 
            
            else {
                printf("%s ", *echoArguments);
            }

            echoArguments++;
        }

        printf("\n");

        return 0;
    }

    if (!strcmp(arguments[argCount - 1], "&")) {
        backgroundProcess = 1;
        arguments[argCount - 1] = NULL;
    } 
    else {
        backgroundProcess = 0;
    }

    int redirectFd = handleRedirection(arguments, &outputFile, argCount);

    if ((runningProcess = fork()) == 0) {
        if (redirectFd >= 0) {
            if (!strcmp(arguments[argCount - 2], ">>")) {
                fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                lseek(fd, 0, SEEK_END);
            } 
            else if (!strcmp(arguments[argCount - 2], ">") || !strcmp(arguments[argCount - 2], "2>")) {
                fd = creat(outputFile, 0644);
            } 
            else {
                fd = open(outputFile, O_RDONLY);
            }

            close(redirectFd);
            dup(fd);
            close(fd);
            arguments[argCount - 2] = NULL;
        }

        execvp(arguments[0], arguments);
    }

    if (backgroundProcess == 0) {
        wait(&commandStatus);
        result = commandStatus;
        runningProcess = -1;
    }

    if (hasPipe) {
        close(STDOUT_FILENO);
        close(pipeFd[PIPE_WRITER]);
        dup(stdoutFileDescriptor);
        wait(NULL);
    }

    return result;
}

// Processes a control command (if, then, else, fi).
int handleControlFlowCommand(char **arguments) {
    char *command = arguments[0];
    int result = -1;

    if (strcmp(command, "if") == 0) {
        if (currentState != STATE_NEUTRAL) {
            printf("unexpected 'if' keyword!\n");
            result = 1;
        } else {
            lastStatus = processCommand(arguments + 1);
            currentResult = (lastStatus == 0) ? RESULT_SUCCESS : RESULT_FAIL;
            currentState = STATE_WANT_THEN;
            result = 0;
        }
    } else if (strcmp(command, "then") == 0) {
        if (currentState != STATE_WANT_THEN) {
            printf("unexpected 'then' keyword!\n");
            result = 1;
        } else {
            currentState = STATE_THEN_BLOCK;
            result = 0;
        }
    } else if (strcmp(command, "else") == 0) {
        if (currentState != STATE_THEN_BLOCK) {
            printf("unexpected 'else' keyword!\n");
            result = 1;
        } else {
            currentState = STATE_ELSE_BLOCK;
            result = 0;
        }
    } else if (strcmp(command, "fi") == 0) {
        if (currentState != STATE_THEN_BLOCK && currentState != STATE_ELSE_BLOCK) {
            printf("unexpected 'fi' keyword!\n");
            result = 1;
        } else {
            currentState = STATE_NEUTRAL;
            result = 0;
        }
    }

    return result;
}

// Determines if a command is a shell control flow command (e.g., if, then, else, fi).
int isControlFlowCommand(char *command) {
    return (strcmp(command, "if") == 0 || strcmp(command, "then") == 0 || strcmp(command, "else") == 0 || strcmp(command, "fi") == 0);
}

// Checks if it is valid to execute the next command based on the current state
int isValidExecution() {
    // Default validation value (true).
    int validation = 1;

    // Update validation based on the current state and result
    if ((currentState == STATE_WANT_THEN) || 
        (currentState == STATE_THEN_BLOCK && currentResult == RESULT_FAIL) || 
        (currentState == STATE_ELSE_BLOCK && currentResult == RESULT_SUCCESS)) {
        validation = 0;
    }

    return validation;
}

// Processes a command.
int processCommand(char **arguments) {
    int result = -1;

    if (arguments[0] == NULL) {
        result = 0;
    } else if (isControlFlowCommand(arguments[0])) {
        result = handleControlFlowCommand(arguments);
    } else if (isValidExecution()) {
        result = executeCommand(arguments);
    }

    return result;
}

// Handles up and down arrow key commands for command history browsing.
void handleArrowKeys(char *token) {
    if (strcmp(token, "[A") == 0) { // Up arrow key
        if (commandHistoryRoot->prev != NULL) {
            commandHistoryRoot = commandHistoryRoot->prev;
            strcpy(currentCommand, commandHistoryRoot->command);
            printf("\r%s %s", prompt, currentCommand); // Print the command
            fflush(stdout);
        }
    } else if (strcmp(token, "[B") == 0) { // Down arrow key
        if (commandHistoryRoot->next != NULL) {
            commandHistoryRoot = commandHistoryRoot->next;
            strcpy(currentCommand, commandHistoryRoot->command);
            printf("\r%s %s", prompt, currentCommand); // Print the command
            fflush(stdout);
        }
    }
}

// Initializing the shell.
int main() {
    mainProcessId = getpid();
    signal(SIGINT, handleCtrlC);

    prompt = strdup("hello:");
    commandHistoryRoot = (CommandHistory *) malloc(sizeof(CommandHistory));
    commandHistoryRoot->next = NULL;
    commandHistoryRoot->prev = NULL;

    CommandHistoryList history = {0};

    while (SHELL_IS_RUNNING) {
        printf("%s ", prompt);

        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        int position = 0;
        int ch;
        while ((ch = getchar()) != '\n') {
            if (ch == 27) { // Escape sequence
                getchar();  // skip the [
                switch (getchar()) {
                    case 'A': // Up arrow
                        navigateHistory(&history, 1);
                        position = strlen(currentCommand);
                        printf("\r\033[K%s %s", prompt, currentCommand);
                        fflush(stdout);
                        break;
                    case 'B': // Down arrow
                        navigateHistory(&history, -1);
                        position = strlen(currentCommand);
                        printf("\r\033[K%s %s", prompt, currentCommand);
                        fflush(stdout);
                        break;
                }
            } else if (ch == 127) { // Backspace
                if (position > 0) {
                    position--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else {
                if (position < MAX_BYTES - 1) {
                    currentCommand[position++] = ch;
                    putchar(ch);
                    fflush(stdout);
                }
            }
        }
        currentCommand[position] = '\0';
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        printf("\n"); // Add newline after command input

        if (strlen(currentCommand) > 0) {
            addCommandToHistory(&history, currentCommand);
            strcpy(previousCommand, currentCommand);
        }

        // Handle 'if' control flow
        if (strncmp(currentCommand, "if", 2) == 0) {
            char conditionCommand[MAX_BYTES];
            strcpy(conditionCommand, currentCommand + 3);

            char thenCommand[MAX_BYTES] = "";
            char elseCommand[MAX_BYTES] = "";

            currentState = STATE_WANT_THEN;
            char scriptCommand[MAX_BYTES];

            while (SCRIPTING) {
                printf("scripting: ");
                fgets(scriptCommand, MAX_BYTES, stdin);
                scriptCommand[strlen(scriptCommand) - 1] = '\0';

                if (strncmp(scriptCommand, "then", 4) == 0) {
                    currentState = STATE_THEN_BLOCK;
                    continue;
                } else if (strncmp(scriptCommand, "else", 4) == 0) {
                    currentState = STATE_ELSE_BLOCK;
                    continue;
                } else if (strncmp(scriptCommand, "fi", 2) == 0) {
                    currentState = STATE_NEUTRAL;
                    break;
                }

                if (currentState == STATE_THEN_BLOCK) {
                    strcat(thenCommand, scriptCommand);
                    strcat(thenCommand, "; ");
                } else if (currentState == STATE_ELSE_BLOCK) {
                    strcat(elseCommand, scriptCommand);
                    strcat(elseCommand, "; ");
                }
            }

            // Evaluate the if condition
            int conditionResult = system(conditionCommand);

            if (conditionResult == 0) { // Condition is true
                system(thenCommand);
            } else { // Condition is false
                if (strlen(elseCommand) > 0) {
                    system(elseCommand);
                }
            }

            continue;
        }

        if (strcmp(currentCommand, "!!")) {
            strcpy(previousCommand, currentCommand);
        }

        // End the shell.
        if (!strcmp(currentCommand, "quit")) {
            break;
        }

        dismantleCommand(currentCommand);
        commandStatus = processCommand(arguments);
    }

    return 0;
}