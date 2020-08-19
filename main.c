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

// A linked list node
typedef struct Node {
    char command; //command
    int addr1;
    int addr2;
    darray *lines;
    struct Node *next; //pointer to the next node
} stack_node;

typedef struct {
    stack_node *top;
    int size; //number of commands saved in stack
} stack_t;

darray *text_array;
stack_t *undo_stack;
stack_t *redo_stack;

bool first_print;   //true if a line has already been printed
bool is_redoable;
int pending;


bool resize_darray(darray *array, int new_capacity);

bool enlarge_darray(darray *array);

darray *new_darray(int initial_capacity);

void append_string_by_copy(darray *array, char *string);

char *get_string_at(const darray *array, int index);

void save_and_replace(darray *save_array, darray *write_array, char *new_line, int index);

void save_and_remove(darray *lines_deleted, darray *text, int index);

void free_darray(darray *array);

bool contains_index(darray *array, int index);

bool valid_addresses(int addr1, int addr2);

void save_in_node_and_remove(darray *text, stack_node *node, int index, int num_lines_to_remove);

void push(stack_t *stack, char command, int addr1, int addr2, darray *lines_to_save); // insert at the beginning

stack_node *peek(stack_t *stack);

void pop(stack_t *stack); // remove at the beginning

void swap_stack(stack_t *sender_s, stack_t *receiver_s);

void execute_pending_undo();

void change(int addr1, int addr2);

void print(int addr1, int addr2);

void delete(int addr1, int addr2);

void delete_without_undo(stack_node *node, int addr1, int addr2);

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

    int len = strlen(string) + 1;

    array->strings[array->n] = malloc(len);
    //strcpy(array->strings[array->n], string);
    memcpy(array->strings[array->n], string, len);

    array->n++;

}

void insert_string_by_reference(darray *array, int index, char *string) {

    if (array->n == array->capacity && !enlarge_darray(array))
        return;

    array->n++;

    for (int i = array->n - 1; i > index; i--)
        array->strings[i] = array->strings[i - 1];

    array->strings[index] = string;
}


char *get_string_at(const darray *array, int index) {
    return array->strings[index];
}

void replace_string_by_reference(darray *array, int index, char *string) {
    array->strings[index] = string;
}

//saves overwritten string write_array->strings[index] in save_array (appends)
//and writes new_line in its place
void save_and_replace(darray *save_array, darray *write_array, char *new_line, int index) {
    //equivalent to:
    //append_string_by_copy(lines_edited, get_string_at(text_array, current_text_index)); //save old string to undo stack
    //replace_string_at(text_array, current_text_index, get_string_at(lines_to_rewrite, i)); //overwrite in text

    //append to save_array
    if (save_array->n == save_array->capacity && !enlarge_darray(save_array))
        return;


    //save pointer to old string in save_array
    save_array->strings[save_array->n] = write_array->strings[index];
    save_array->n++;

    int len = strlen(new_line) + 1;

    //allocate new string that position, losing pointer to old string, that is now saved in save_array
    write_array->strings[index] = malloc(len);
    //strcpy(write_array->strings[index], new_line);
    memcpy(write_array->strings[index], new_line, len);

}

void save_and_remove(darray *lines_deleted, darray *text, int index) {
    //equivalent to:
    //append_string_by_copy(lines_deleted, get_string_at(text_array, line_to_delete)); //save deleted string to undo stack
    //remove_string_at(text_array, line_to_delete);

    if (lines_deleted->n == lines_deleted->capacity && !enlarge_darray(lines_deleted))
        return;


    //append to lines deleted
    lines_deleted->strings[lines_deleted->n] = text->strings[index];
    lines_deleted->n++;

    //shift all strings by one and free the deleted deleted_string
    for (int i = index + 1; i < text->n; i++)
        text->strings[i - 1] = text->strings[i];

    text->n--;

}

void free_darray(darray *array) {
    int n = array->n;

    for (int i = 0; i < n; i++) {
        if (array->strings[i] != NULL)
            free(array->strings[i]);
    }
    free(array->strings);
    free(array);
}

bool contains_index(darray *array, int index) {
    return index < array->n && index >= 0;
}

bool valid_addresses(int addr1, int addr2) {
    return addr1 > 0 && addr2 > 0 && addr1 <= addr2 && (addr1 <= text_array->n || addr1 == 1);
}

void push(stack_t *stack, char command, int addr1, int addr2, darray *lines_to_save) {

    stack->size++;

    struct Node *node = malloc(sizeof(struct Node));

    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;
    node->lines = lines_to_save;

    node->next = stack->top;
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
    //update the top pointer to point to the next node
    stack->top = stack->top->next;
    //free memory for array of strings
    if (node->lines != NULL)
        free_darray(node->lines);
    //free memory for node popped
    free(node);
}

//append top of undo_stack to redo_stack
//sender: undo; receiver: redo
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

    //redo_stack->top = NULL; //intentional memory leak
    while (redo_stack->size > 0) {
        pop(redo_stack);
    }
}

//debugging
void printStack(stack_t *stack) {

    printf("\n\n\nUNDO STACK:\n");
    stack_node *node = stack->top;
    while (node != NULL) {
        if (node->lines != NULL) {
            for (int i = 0; i < node->lines->n; i++) {
                printf("%d ", i);
                puts(node->lines->strings[i]);
            }
        }
        node = node->next;
    }

}

void free_stack(stack_t *stack) {
    while (stack->top != NULL)
        pop(stack);
}


int main() {
    //Rolling_Back_2_input
    //Altering_History_2_input
    //simple_redo_input
    //freopen("test10000l6.txt", "r", stdin);
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

    int current_index = addr1 - 1;
    char input_line[STRING_LENGTH];
    darray *lines_edited = NULL;
    bool first_line_edited = true;

    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    while (true) {

        fgets(input_line, STRING_LENGTH, stdin);
        input_line[strlen(input_line) - 1] = '\0'; //removes \n

        if (strcmp(input_line, ".") == 0) {
            //save edited (and not added) lines to undo stack
            push(undo_stack, 'c', addr1, addr2, lines_edited);
            return;
        }

        if (current_index < text_array->n) { //overwrite existing string

            if (first_line_edited)
                lines_edited = new_darray(INITIAL_CAPACITY);
            first_line_edited = false;

            //save old string to lines_edited for undo stack and overwrite it with input_line
            save_and_replace(lines_edited, text_array, input_line, current_index);
        } else //add new string
            append_string_by_copy(text_array, input_line);

        current_index++;
    }

}

//optimization: put first_print bool outside loops
void print(int addr1, int addr2) {

    int current_index = addr1 - 1;

    execute_pending_undo();

    //append \n before each line, except if it's first print
    if (current_index < 0) {
        if (!first_print) fputc_unlocked('\n', stdout);
        fputc_unlocked('.', stdout);
        return;
    }

    while (current_index <= addr2 - 1) {

        if (!first_print) fputc_unlocked('\n', stdout);

        if (contains_index(text_array, current_index)) {
            fputs(get_string_at(text_array, current_index), stdout);
        } else
            fputc_unlocked('.', stdout);

        current_index++;
        first_print = false;
    }
}

void delete(int addr1, int addr2) {

    darray *lines_deleted = NULL;
    bool first_line_deleted = true;

    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    if (!valid_addresses(addr1, addr2)) {
        //if delete is invalid node->lines == NULL
        push(undo_stack, 'd', addr1, addr2, NULL);
        return;
    }

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;
    int number_of_lines = last_index - addr1 + 2;
    int index_to_delete = addr1 - 1;

    for (int i = 0; i < number_of_lines; i++) {

        if (first_line_deleted)
            lines_deleted = new_darray(INITIAL_CAPACITY);
        first_line_deleted = false;

        //here lines_deleted is allocated for sure
        save_and_remove(lines_deleted, text_array, index_to_delete);

        first_print = false;
    }

    push(undo_stack, 'd', addr1, addr2, lines_deleted);

}

//deletes without saving on undo_stack, and saves on redo_stack instead
//by appending to lines_undone, which will be appended to redo_stack
void delete_without_undo(stack_node *node, int addr1, int addr2) {

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;
    int number_of_lines = last_index - addr1 + 2;
    int index_to_delete = addr1 - 1;

    save_in_node_and_remove(text_array, node, index_to_delete, number_of_lines);
}

void undo(int number) {
    //pop and revert _number_ commands
    stack_node *undo_node;

    for (int i = 0; i < number; i++) {
        undo_node = peek(undo_stack);

        //undo_change(), undo_delete(), lines_undone concern only ONE undo operation
        //undo() orchestrates and calls them _number_ times

        if (undo_node->command == 'c')
            undo_change(undo_node);
        else
            undo_delete(undo_node);

    }
}

void save_in_node_and_replace(darray *text, stack_node *node, int addr1, int num_edited_lines) {

    int text_index;
    char *temp;

    for (int i = 0; i < num_edited_lines; i++) {

        text_index = addr1 + i - 1;

        //1. copy reference to strings of text that will be overwritten
        temp = text->strings[text_index];

        //2. make text strings point to new undo_lines, saved in undo node (undo_lines)
        replace_string_by_reference(text, text_index, node->lines->strings[i]);
        //text->strings[text_index] = node->lines->strings[i];

        //3. make undo_lines point to overwritten lines
        replace_string_by_reference(node->lines, i, temp);
        //node->lines->strings[i] = temp;
    }

}

//appends all lines in node to text
void append_node_lines_to_text(darray *text, stack_node *node, int lines_to_add) {

    for (int i = 0; i < lines_to_add; i++) {

        append_string_by_reference(text, node->lines->strings[i]);

        //doesn't overwrite anything, only appends
        node->lines->strings[i] = NULL;
    }

    //devo deallocare spazio puntatore?
    node->lines->n = 0;
}

void save_in_node_and_remove(darray *text, stack_node *node, int index, int num_lines_to_remove) {

    if (node->lines == NULL)
        node->lines = new_darray(INITIAL_CAPACITY);

    for (int i = 0; i < num_lines_to_remove; i++) {

        append_string_by_reference(node->lines, text->strings[index]);

        //shift all strings by one and free the deleted deleted_string
        for (int j = index + 1; j < text->n; j++)
            text->strings[j - 1] = text->strings[j];

        text->n--;
    }

}

void insert_node_lines_in_text(stack_node *node, int addr1) {

    int lines_to_add = node->lines->n;
    int text_index;

    for (int j = 0; j < lines_to_add; j++) {
        //insert_string_at(text_array, addr1 + j - 1, undo_lines->strings[j]);
        text_index = addr1 + j - 1;

        insert_string_by_reference(text_array, text_index, node->lines->strings[j]);

        node->lines->strings[j] = NULL;
        //devo deallocare spazio puntatore?
    }

    node->lines->n = 0;

}

void undo_change(stack_node *undo_node) {

    int addr1 = undo_node->addr1;
    int addr2 = undo_node->addr2;

    //if c has no undo_lines at all it means it was only an addition without edits -> delete all
    //ONLY ADDED
    if (undo_node->lines == NULL) {
        delete_without_undo(undo_node, addr1, addr2);
        swap_stack(undo_stack, redo_stack);
        return; //skip to next undo node
    }

    //replace edited strings with old ones
    int num_edited_lines = undo_node->lines->n;
    int num_changed_lines = addr2 - addr1 + 1;


    //EDITED LINES
    //lines added will be handled by code below
    if (num_edited_lines > 0)
        save_in_node_and_replace(text_array, undo_node, addr1, num_edited_lines);

    //LINES ADDED AFTER EDITING PREVIOUS LINES
    if (num_changed_lines > num_edited_lines)
        delete_without_undo(undo_node, addr1 + num_edited_lines, addr2); //delete added strings

    swap_stack(undo_stack, redo_stack);

    //if condition above is true:
    //first strings were edited [addr1, addr1 + num_edited_lines - 1] and already reverted in code above
    //last strings were added [addr1 + num_edited_lines, addr2] and need to be deleted
}

void undo_delete(stack_node *undo_node) {

    //delete was invalid and nothing was actually deleted
    if (undo_node->lines == NULL) {
        swap_stack(undo_stack, redo_stack);
        return;
    }

    int addr1 = undo_node->addr1;

    //deleted undo_lines were at the end of text
    if (addr1 > text_array->n)
        append_node_lines_to_text(text_array, undo_node, undo_node->lines->n);

        //deleted undo_lines were between other undo_lines
    else //if (addr1 <= text_array->n)
        insert_node_lines_in_text(undo_node, addr1);

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
    int addr2 = redo_node->addr2;

    int last_line_edited = addr2 > text_array->n ? text_array->n : addr2;
    int num_lines_overwritten = last_line_edited - addr1 + 1;
    int num_lines_changed = addr2 - addr1 + 1;
    int num_lines_added = num_lines_changed - num_lines_overwritten;


    if (num_lines_overwritten > 0) //RESTORE EDITED LINES
        save_in_node_and_replace(text_array, redo_node, addr1, num_lines_overwritten);
    if (num_lines_added > 0) { //RESTORE DELETED LINES
        //append_node_lines_to_text(text_array, redo_node, num_lines_added);

        for (int i = num_lines_overwritten; i < num_lines_changed; i++) {

            append_string_by_reference(text_array, redo_node->lines->strings[i]);
            redo_node->lines->strings[i] = NULL;

        }
        redo_node->lines->n -= num_lines_added;

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

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;
    int number_of_lines = last_index - addr1 + 2;
    int index_to_delete = addr1 - 1;

    save_in_node_and_remove(text_array, redo_node, index_to_delete, number_of_lines);

    swap_stack(redo_stack, undo_stack);

}





