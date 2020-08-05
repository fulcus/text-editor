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

void insert_string_at(darray *array, int index, char *string);

char *get_string_at(const darray *array, int index);

void remove_string_at(darray *array, int index);

void replace_string_at(darray *array, int index, char *string);

void save_and_replace(darray *save_array, darray *write_array, char *new_line, int index);

void save_and_remove(darray *lines_deleted, darray *text, int index);

void free_darray(darray *array);

bool contains_index(darray *array, int index);

bool valid_addresses(int addr1, int addr2);

void push(stack_t *stack, char command, int addr1, int addr2, darray *lines_edited); // insert at the beginning

stack_node *peek(stack_t *stack);

void pop(stack_t *stack); // remove at the beginning

void swap_stack(stack_t *sender_s, stack_t *receiver_s, darray *lines_undone);

void execute_pending_undo();

void change(int addr1, int addr2);

void print(int addr1, int addr2);

void delete(int addr1, int addr2);

void delete_without_undo(int addr1, int addr2, darray *lines_undone);

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

    //allocate new string that position, losing pointer to old string, that is now saved in save_array
    write_array->strings[index] = malloc(strlen(new_line) + 1);
    strcpy(write_array->strings[index], new_line);

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

void push(stack_t *stack, char command, int addr1, int addr2, darray *lines_edited) {

    stack->size++;

    struct Node *node = malloc(sizeof(struct Node));

    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;
    node->lines = lines_edited;

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
    //update the top pointer to point to the next node
    stack->top = stack->top->next;
    //free memory for array of strings
    if (node->lines != NULL)
        free_darray(node->lines);
    //free memory for node popped
    free(node);
}

//append top of undo_stack to redo_stack
void swap_stack(stack_t *sender_s, stack_t *receiver_s, darray *lines_undone) {
    stack_node *sender_top = peek(sender_s);

    stack_node *sender_new_top = sender_top->next; //save

    sender_top->next = receiver_s->top;  //point top of undo to top of redo
    receiver_s->top = sender_top; //make new node added the top of redo

    receiver_s->size++;

    //make top of undo_stack point to the old undo top->next
    //overwriting pointer to its former top that is now top of redo_stack
    sender_s->top = sender_new_top;
    sender_s->size--;

    //edit lines pointer
    if (receiver_s->top->lines != NULL)
        free_darray(receiver_s->top->lines);
    receiver_s->top->lines = lines_undone;

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
void printUndoStack() {

    printf("\n\n\nUNDO STACK:\n");
    stack_node *node = undo_stack->top;
    printf("Undo Stack:\n");
    while (node != NULL) {

        for (int i = 0; i < node->lines->n; i++) {
            printf("%d ", i);
            puts(node->lines->strings[i]);
        }
        node = node->next;
    }

}

void free_stack(stack_t *stack) {
    while (stack->top != NULL)
        pop(stack);
}


int main() {
    //Rolling_Back_2_input.txt
    //freopen("test10000.txt", "r", stdin);
    //freopen("output.txt", "w+", stdout);

    first_print = true;
    is_redoable = false;
    pending = 0;

    char input[STRING_LENGTH];
    char *addrString1, *addrString2;
    char command, c;
    int addr1, addr2, len;

    text_array = new_darray(INITIAL_CAPACITY);

    undo_stack = malloc(sizeof(*undo_stack));
    undo_stack->top = NULL;
    undo_stack->size = 0;

    redo_stack = malloc(sizeof(*redo_stack));
    redo_stack->top = NULL;
    redo_stack->size = 0;


    while (true) {

        len = 0;
        do {
            c = (char) fgetc_unlocked(stdin);
            if (c == '\n' || c == '\0') {
                input[len] = 0;
                break;
            }
            input[len++] = c;
        } while (true);

        //len = strlen(input);
        command = input[len - 1]; //get last char of input, counting \n before that
        input[len - 1] = '\0'; //deletes command char and \n from input

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

    int len, current_index = addr1 - 1;
    char input_line[STRING_LENGTH];
    char c;
    //allocate darray only if written to
    //else put NULL in undo_stack->lines attribute
    darray *lines_edited = NULL;
    bool first_line_edited = true;

    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    while (true) {

        len = 0;
        while (true) {
            c = (char) fgetc_unlocked(stdin);
            if (c == '\n' || c == '\0') {
                input_line[len] = 0;
                break;
            }
            input_line[len++] = c;
        }


        if (strcmp(input_line, ".") == 0) {
            //finished editing, save edited lines to undo stack
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

    is_redoable = false; //might be useless (already cleared redo)

    if (!valid_addresses(addr1, addr2)) {
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
void delete_without_undo(int addr1, int addr2, darray *lines_undone) {

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    int number_of_lines = last_index - addr1 + 2;
    int index_to_delete = addr1 - 1;

    for (int i = 0; i < number_of_lines; i++)
        save_and_remove(lines_undone, text_array, index_to_delete);


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
        swap_stack(undo_stack, redo_stack, NULL);
        return;
    }

    darray *lines = undo_node->lines;
    darray *lines_undone = new_darray(INITIAL_CAPACITY);

    //if c has no lines at all it means it was only an addition without edits -> delete all
    //ONLY ADDED
    if (lines == NULL) {
        delete_without_undo(addr1, addr2, lines_undone);
        swap_stack(undo_stack, redo_stack, lines_undone);
        return; //skip to next undo node
    }

    //replace edited strings with old ones
    int edited_lines_count = lines->n;

    //EDITED LINES (might have been also added)
    for (int j = 0; j < edited_lines_count; j++)
        save_and_replace(lines_undone, text_array, get_string_at(lines, j), addr1 + j - 1);

    //if condition below is true:
    //first strings were edited [addr1, addr1 + edited_lines_count - 1] and already reverted in code above
    //last strings were added [addr1 + edited_lines_count, addr2] and need to be deleted

    //LINES ADDED AFTER EDITING PREVIOUS LINES
    if (addr2 - addr1 + 1 > edited_lines_count)
        delete_without_undo(addr1 + edited_lines_count, addr2, lines_undone); //delete added strings

    swap_stack(undo_stack, redo_stack, lines_undone);

}

void undo_delete(stack_node *undo_node) {

    //delete was invalid and nothing was actually deleted
    if (undo_node->lines == NULL) {
        swap_stack(undo_stack, redo_stack, NULL);
        return;
    }

    int addr1 = undo_node->addr1;
    darray *lines = undo_node->lines;
    int lines_to_add = lines->n;

    //deleted lines were at the end of text
    if (addr1 > text_array->n) {

        for (int j = 0; j < lines_to_add; j++)
            append_string_by_copy(text_array, lines->strings[j]);

        //deleted lines were between other lines
    } else if (addr1 <= text_array->n) {

        for (int j = 0; j < lines_to_add; j++)
            insert_string_at(text_array, addr1 + j - 1, lines->strings[j]);

    }

    //don't need to save any line to redo_stack
    //redo of undo of delete is a simple delete: addr1, addr2 are sufficient
    swap_stack(undo_stack, redo_stack, NULL);
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
    darray *lines_to_rewrite = redo_node->lines;

    int num_lines_to_rewrite = lines_to_rewrite->n;
    darray *lines_edited = NULL;

    if (addr1 <= text_array->n && text_array->n > 0)
        lines_edited = new_darray(INITIAL_CAPACITY);


    for (int i = 0; i < num_lines_to_rewrite; i++) {

        if (addr1 - 1 + i < text_array->n) //add new string
            save_and_replace(lines_edited, text_array, get_string_at(lines_to_rewrite, i), addr1 - 1 + i);
        else //includes case text_array->n == 0
            append_string_by_copy(text_array, get_string_at(lines_to_rewrite, i));
    }

    swap_stack(redo_stack, undo_stack, lines_edited);

}

void redo_delete(stack_node *redo_node) {

    int addr1 = redo_node->addr1;
    int addr2 = redo_node->addr2;

    if (!valid_addresses(addr1, addr2)) {
        swap_stack(redo_stack, undo_stack, NULL);
        return;
    }

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;
    int number_of_lines = last_index - addr1 + 2;
    int index_to_delete = addr1 - 1;

    darray *lines_deleted = new_darray(INITIAL_CAPACITY);

    for (int i = 0; i < number_of_lines; i++)
        save_and_remove(lines_deleted, text_array, index_to_delete);

    swap_stack(redo_stack, undo_stack, lines_deleted);

}





