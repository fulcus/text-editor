#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#define INITIAL_CAPACITY 1
#define STRING_LENGTH 1025
#define GROWTH_FACTOR 2

typedef struct {
    int capacity;   //max number of strings it can contain
    int n;  //number of strings actually stored
    char **strings;
} darray;

typedef struct Node {
    char command;
    int addr1;
    int addr2;
    char **old_lines;
    int old_lines_n;
    char **new_lines;
    int new_lines_n;
    struct Node *next;
} stack_node;

typedef struct {
    stack_node *top;
    int size; //number of commands saved in stack
} stack_t;

darray *text_array;
stack_t *undo_stack;
stack_t *redo_stack;

bool first_print; //true if a line has already been printed
bool is_redoable;
int pending;


bool resize_darray(darray *array, int new_capacity);

bool enlarge_darray(darray *array);

darray *new_darray(int initial_capacity);

int size_darray(const darray *array);

void append_string_by_copy(darray *array, char *string);

void insert_string_at(darray *array, int index, char *string);

char *get_string_at(const darray *array, int index);

void remove_string_at(darray *array, int index);

void replace_string_at(darray *array, int index, char *string);

void save_and_replace(darray *save_array, darray *text, char *new_line, int index);

void remove_from_text(darray *text, int index);

void free_darray(darray *array);

bool contains_index(darray *array, int index);

bool valid_addresses(int addr1, int addr2);

void push(stack_t *stack, char command, int addr1, int addr2, char **old_lines, int old_lines_n, char **new_lines,
          int new_lines_n); // insert at the beginning

stack_node *peek(stack_t *stack);

void pop(stack_t *stack); // remove at the beginning

void swap_stack(stack_t *sender_s, stack_t *receiver_s);

void execute_pending_undo();

void change(int addr1, int addr2);

void print(int addr1, int addr2);

void delete(int addr1, int addr2);

void delete_without_undo(int addr1, int addr2);

void undo(int number);

void undo_change(stack_node *undo_node);

void undo_delete(stack_node *undo_node);

void redo(int number);

void redo_change(stack_node *redo_node);

void redo_delete(stack_node *redo_node);

bool resize_darray(darray *array, int new_capacity) {
    array->strings = realloc(array->strings, sizeof(*(array->strings)) * new_capacity);
    array->capacity = new_capacity;
    return true;
}

bool enlarge_darray(darray *array) {
    return resize_darray(array, array->capacity * GROWTH_FACTOR + 1);
}

darray *new_darray(int initial_capacity) {
    darray *new_darray = malloc(sizeof(*new_darray));

    new_darray->capacity = 0;
    new_darray->n = 0;
    new_darray->strings = NULL;
    if (!resize_darray(new_darray, initial_capacity)) {
        free_darray(new_darray);
        return NULL;
    }

    return new_darray;
}

int size_darray(const darray *array) {
    return array->n;
}

void append_string_by_reference(darray *array, char *string) {
    //insert_string_at(array, array->n, string);

    if (array->n == array->capacity && !enlarge_darray(array))
        return;

    array->strings[array->n] = string;
    array->n++;

}

void append_string_by_copy(darray *array, char *string) {

    if (array->n == array->capacity && !enlarge_darray(array))
        return;

    array->strings[array->n] = malloc((strlen(string) + 1));
    strcpy(array->strings[array->n], string);

    array->n++;

}

void insert_string_at(darray *array, int index, char *string) {

    if (array->n == array->capacity && !enlarge_darray(array))
        return;

    array->n++;

    for (int i = array->n - 1; i > index; i--)
        array->strings[i] = array->strings[i - 1];

    array->strings[index] = malloc((strlen(string) + 1));
    strcpy(array->strings[index], string);
}

char *get_string_at(const darray *array, int index) {
    return array->strings[index];
}

void remove_string_at(darray *array, int index) {
    char *deleted_string = get_string_at(array, index);

    //shift all strings by one and free the deleted deleted_string
    for (int i = index + 1; i < array->n; i++)
        array->strings[i - 1] = array->strings[i];

    array->n--;
    free(deleted_string);
}

void replace_string_at(darray *array, int index, char *string) {
    //free(array->strings[index]);
    //array->strings[index] = malloc((strlen(string) + 1));
    array->strings[index] = realloc(array->strings[index], (strlen(string) + 1) * sizeof(char));
    strcpy(array->strings[index], string);
}

//saves overwritten string text->strings[index] in save_array (appends)
//and writes new_line in its place
void save_and_replace(darray *save_array, darray *text, char *new_line, int index) {
    //equivalent to:
    //append_string_by_copy(lines_edited, get_string_at(text_array, current_text_index)); //save old string to undo stack
    //replace_string_at(text_array, current_text_index, get_string_at(lines_to_rewrite, i)); //overwrite in text

    //append to save_array
    if (save_array->n == save_array->capacity && !enlarge_darray(save_array))
        return;


    //save pointer to old string in save_array
    save_array->strings[save_array->n] = text->strings[index];
    save_array->n++;

    //allocate new string that position, losing pointer to old string, that is now saved in save_array
    text->strings[index] = malloc(strlen(new_line) + 1);
    strcpy(text->strings[index], new_line);

}

//saves overwritten string text->strings[index] in save_array (appends)
//and writes new_line in its place
void save_and_replace_by_reference(stack_node *node, darray *text, char *new_line, int index, int lines_n) {
    //equivalent to:
    //append_string_by_copy(lines_edited, get_string_at(text_array, current_text_index)); //save old string to undo stack
    //replace_string_at(text_array, current_text_index, get_string_at(lines_to_rewrite, i)); //overwrite in text

    node->old_lines = &text->strings[index];
    node->old_lines_n = lines_n;

    //allocate new string that position, losing pointer to old string, that is now saved in save_array
    text->strings[index] = malloc(strlen(new_line) + 1);
    strcpy(text->strings[index], new_line);

}



void remove_from_text(darray *text, int index) {

    //shift all strings by one and free the deleted deleted_string
    for (int i = index + 1; i < text->n; i++)
        text->strings[i - 1] = text->strings[i];

    text->n--;

}

void free_darray(darray *array) {
    int n = array->n;

    for (int i = 0; i < n; i++)
        free(array->strings[i]);

    free(array->strings);
    free(array);
}

bool contains_index(darray *array, int index) {
    return index < array->n && index >= 0;
}

bool valid_addresses(int addr1, int addr2) {
    return addr1 > 0 && addr2 > 0 && addr1 <= addr2 && (addr1 <= text_array->n || addr1 == 1);
}

// insert at the beginning
void push(stack_t *stack, char command, int addr1, int addr2, char **old_lines, int old_lines_n, char **new_lines,
          int new_lines_n) {

    stack->size++;

    // Allocate the new node in the heap
    struct Node *node = malloc(sizeof(struct Node));

    // set the command in allocated node
    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;
    node->old_lines = old_lines;
    node->old_lines_n = old_lines_n;
    node->new_lines = new_lines;
    node->new_lines_n = new_lines_n;

    // Set the .next pointer of the new node to point to the current top node of the list
    node->next = stack->top;
    // update top pointer
    stack->top = node;
}


//returns top element
stack_node *peek(stack_t *stack) {
    return stack->top; //NULL if stack is empty, CHECK CONDITION
}

//remove at the beginning
void pop(stack_t *stack) {

    stack->size--;
    stack_node *node = stack->top;
    stack->top = stack->top->next;

    if (node->old_lines != NULL) {
        for (int i = 0; i < node->old_lines_n; i++)
            free(node->old_lines[i]);
        free(node->old_lines);
    }
    if (node->new_lines != NULL) {
        for (int i = 0; i < node->new_lines_n; i++)
            free(node->new_lines[i]);
        free(node->new_lines);
    }

    //free memory for node popped
    free(node);
}

//append top of undo_stack to redo_stack
void swap_stack(stack_t *sender_s, stack_t *receiver_s) {
    stack_node *sender_top = peek(sender_s);

    stack_node *sender_new_top = sender_top->next; //save

    sender_top->next = receiver_s->top;  //point top of undo to top of redo
    receiver_s->top = sender_top; //make new node added the top of redo

    receiver_s->size++;

    //make top of undo_stack point to the old undo top->next
    //overwriting pointer to its former top that is now top of redo_stack
    sender_s->top = sender_new_top;
    sender_s->size--;

}

void update_pending(int number) {

    if (number > 0) //undo
        is_redoable = true;
    else if (!is_redoable) //redo not redoable, doesn't count
        return;

    pending += number;

    if (pending > undo_stack->size) //max as many undo as undoable c,d
        pending = undo_stack->size;
    else if (-pending > redo_stack->size) //max as many redo as undos
        pending = -redo_stack->size;
}


//executes all pending undos
void execute_pending_undo() {

    //puts("\nEXECUTING UNDO/REDO");

    if (pending == 0)
        return;
    else if (pending > 0)
        undo(pending); //executes all pending undos
    else //pending < 0
        redo(-pending);

    pending = 0;
}

void clear_redo() {

    redo_stack->top = NULL; //intentional memory leak
    /*while (redo_stack->size > 0) {
        pop(redo_stack);
    }*/
}

void free_stack(stack_t *stack) {
    while (stack->top != NULL)
        pop(stack);
}


int main() {
    //Rolling_Back_2_input.txt
    //Altering_History_2_input
    freopen("Altering_History_2_input.txt", "r", stdin);
    //freopen("output.txt", "w+", stdout);

    first_print = true;
    is_redoable = false;
    pending = 0;

    char input[STRING_LENGTH];
    char *addrString1, *addrString2;
    char command;
    int addr1, addr2, len;

    text_array = new_darray(INITIAL_CAPACITY);

    undo_stack = malloc(sizeof(*undo_stack));
    undo_stack->top = NULL;
    undo_stack->size = 0;

    redo_stack = malloc(sizeof(*redo_stack));
    redo_stack->top = NULL;
    redo_stack->size = 0;


    while (true) {

        fgets(input, STRING_LENGTH, stdin);

        len = strlen(input);
        command = input[len - 2]; //get last char of input, counting \n before that
        input[len - 2] = '\0'; //deletes command char and \n from input

        if (command == 'c') { //change
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");
            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);

            change(addr1, addr2);
        } else if (command == 'd') { //delete
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");
            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);

            delete(addr1, addr2);
        } else if (command == 'p') { //print
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");
            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);

            print(addr1, addr2);
        } else if (command == 'u') { //undo
            addr1 = atoi(input);

            update_pending(addr1);
        } else if (command == 'r') { //redo
            addr1 = atoi(input);

            update_pending(-addr1);
        } else if (command == 'q') { //quit

            //free_stack(redo_stack);
            //free_stack(undo_stack);
            //free_darray(text_array);
            return 0;
        } else {
            //printf("command: %c ", command);
            //puts("invalid input");
            return -1;
        }
    }

}

void change(int addr1, int addr2) {

    int n = text_array->n;
    char input_line[STRING_LENGTH];

    int last_line_edited = addr2 > n ? n : addr2;
    int num_lines_overwritten = last_line_edited - addr1 + 1;
    int num_lines_changed = addr2 - addr1 + 1;

    char **old_lines = NULL; //allocate: last_line_edited - addr1 + 1
    char **new_lines = NULL; //allocate: addr2 - n

    //for old lines allocate num_lines_overwritten
    if (num_lines_overwritten > 0)
        old_lines = malloc(num_lines_overwritten * sizeof(char *));
    //for new lines allocate addr2 - addr1 + 1
    if (num_lines_changed > 0)
        new_lines = malloc(num_lines_changed * sizeof(char *));


    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    int current_index, i = 0;

    while (true) {
        current_index = i + addr1 - 1;

        fgets(input_line, STRING_LENGTH, stdin);
        input_line[strlen(input_line) - 1] = '\0'; //removes \n

        if (strcmp(input_line, ".") == 0) {
            //finished editing, save edited old_lines to undo stack
            push(undo_stack, 'c', addr1, addr2, old_lines, num_lines_overwritten, new_lines, num_lines_changed);
            return;
        }

        //add new string
        if (text_array->n == 0 || current_index >= text_array->n) {
            append_string_by_copy(text_array, input_line);

        } else { //overwrite existing string

            //first time that it overwrites save pointer to overwritten old_lines
            //and "lose" pointers to them in text
            old_lines[i] = text_array->strings[current_index];

            //OVERWRITE POINTERS TO OLD LINES
            //allocate new string that position, losing pointer to old string, that is now saved in save_array
            text_array->strings[current_index] = malloc(strlen(input_line) + 1);
            strcpy(text_array->strings[current_index], input_line);

        }

        new_lines[i] = text_array->strings[current_index];
        i++;
    }

}

//optimization: put first_print bool outside loops
void print(int addr1, int addr2) {

    int current_line = addr1 - 1;

    execute_pending_undo();

    //append \n before each line, except if it's first print
    if (current_line < 0) {
        if (!first_print) fputc_unlocked('\n', stdout);
        fputc_unlocked('.', stdout);
        return;
    }

    while (current_line <= addr2 - 1) {

        if (!first_print) fputc_unlocked('\n', stdout);

        if (contains_index(text_array, current_line)) {
            fputs(get_string_at(text_array, current_line), stdout);
        } else
            fputc_unlocked('.', stdout);

        current_line++;
        first_print = false;

    }
}

void delete(int addr1, int addr2) {

    int last_index;
    int index_to_delete = addr1 - 1;
    int number_of_lines;
    int i = 0;

    execute_pending_undo();
    clear_redo();

    is_redoable = false; //might be useless (already cleared redo)

    if (!valid_addresses(addr1, addr2)) {
        push(undo_stack, 'd', addr1, addr2, NULL, 0, NULL, 0);
        return;
    }

    //checks if some of the old_lines to delete don't exist
    last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    number_of_lines = last_index - addr1 + 2;

    char **lines_deleted = malloc(number_of_lines * sizeof(char *));

    while (i < number_of_lines) {

        //save pointer before overwriting it
        lines_deleted[i] = text_array->strings[index_to_delete];

        //shift all strings by one and free the deleted deleted_string
        for (int j = index_to_delete + 1; j < text_array->n; j++)
            text_array->strings[j - 1] = text_array->strings[j];

        text_array->n--;

        i++;
        first_print = false;
    }

    push(undo_stack, 'd', addr1, addr2, lines_deleted, number_of_lines, NULL, 0);

}

//deletes without saving on undo_stack, and saves on redo_stack instead
//by appending to lines_undone, which will be appended to redo_stack
void delete_without_undo(int addr1, int addr2) {

    //checks if some of the old_lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    int number_of_lines = last_index - addr1 + 2;
    int line_to_delete = addr1 - 1;
    int i = 0;

    while (i < number_of_lines) {
        remove_from_text(text_array, line_to_delete);
        i++;
    }

}

void undo(int number) {
    //pop and revert _number_ commands
    int counter = 0;
    stack_node *undo_node;

    while (counter < number) {

        //undo_change(), undo_delete(), lines_undone concern only ONE undo operation
        //undo() orchestrates and calls them _number_ times

        undo_node = peek(undo_stack);

        if (undo_node->command == 'c')
            undo_change(undo_node);
        else
            undo_delete(undo_node);

        counter++;
    }
}

void undo_change(stack_node *undo_node) {

    int addr1 = undo_node->addr1;
    int addr2 = undo_node->addr2;

    if (!valid_addresses(addr1, addr2)) {
        swap_stack(undo_stack, redo_stack);
        return;
    }

    char **lines = undo_node->old_lines;

    //if c has no old_lines at all it means it was only an addition without edits -> delete all
    //ONLY ADDED
    if (lines == NULL) {
        delete_without_undo(addr1, addr2);
        swap_stack(undo_stack, redo_stack);
        return; //skip to next undo node
    }

    //replace edited strings with old ones
    int edited_lines_count = undo_node->old_lines_n;

    //EDITED LINES (might have been also added)
    for (int j = 0; j < edited_lines_count; j++) {
        text_array->strings[addr1 + j - 1] = lines[j];
    }

    //if condition below is true:
    //first strings were edited [addr1, addr1 + edited_lines_count - 1] and already reverted in code above
    //last strings were added [addr1 + edited_lines_count, addr2] and need to be deleted

    //LINES ADDED AFTER EDITING PREVIOUS LINES
    if (addr2 - addr1 + 1 > edited_lines_count)
        delete_without_undo(addr1 + edited_lines_count, addr2); //delete added strings

    swap_stack(undo_stack, redo_stack);

}

void undo_delete(stack_node *undo_node) {

    //delete was invalid and nothing was actually deleted
    if (undo_node->old_lines == NULL) {
        swap_stack(undo_stack, redo_stack);
        return;
    }

    int addr1 = undo_node->addr1;
    char **lines = undo_node->old_lines;
    int lines_to_add = undo_node->old_lines_n;

    //deleted old_lines were at the end of text
    if (addr1 > text_array->n) {

        for (int j = 0; j < lines_to_add; j++)
            text_array->strings[text_array->n] = lines[j];

        //deleted old_lines were between other old_lines
    } else if (addr1 <= text_array->n) {

        for (int j = 0; j < lines_to_add; j++) {

            //insert
            text_array->n++;

            for (int i = text_array->n - 1; i > addr1 + j - 1; i--)
                text_array->strings[i] = text_array->strings[i - 1];

            text_array->strings[addr1 + j - 1] = lines[j];
        }
    }

    //don't need to save any line to redo_stack
    //redo of undo of delete is a simple delete: addr1, addr2 are sufficient
    swap_stack(undo_stack, redo_stack);
}

void redo(int number) {
    stack_node *redo_node;

    //revert _number_ commands
    for (int i = 0; i < number; i++) {

        redo_node = peek(redo_stack);

        if (redo_node->command == 'c')
            redo_change(redo_node);
        else
            redo_delete(redo_node);

    }
}


//modified version of change
void redo_change(stack_node *redo_node) {

    int addr1 = redo_node->addr1;
    char **lines_to_rewrite = redo_node->new_lines;

    int num_lines_to_rewrite = redo_node->new_lines_n;


    int i = 0;
    while (i < num_lines_to_rewrite) {

        if (addr1 - 1 + i < text_array->n) //insert
            text_array->strings[addr1 - 1 + i] = lines_to_rewrite[i];
        else //append, includes case text_array->n == 0
            text_array->strings[text_array->n] = lines_to_rewrite[i];

        i++;
    }

    swap_stack(redo_stack, undo_stack);

}

void redo_delete(stack_node *redo_node) {

    int addr1 = redo_node->addr1;
    int addr2 = redo_node->addr2;

    if (!valid_addresses(addr1, addr2)) {
        swap_stack(redo_stack, undo_stack);
        return;
    }

    //checks if some of the old_lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;
    int number_of_lines = last_index - addr1 + 1;
    int index_to_delete = addr1 - 1;

    for (int i = 0; i <= number_of_lines; i++)
        remove_from_text(text_array, index_to_delete);

    swap_stack(redo_stack, undo_stack);

}





