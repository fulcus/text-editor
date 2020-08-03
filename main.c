#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#define INITIAL_CAPACITY 1
#define STRING_LENGTH 1025
#define GROWTH_FACTOR 0.5

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

int size_darray(const darray *array);

bool append_string(darray *array, char *string);

bool add_string_at(darray *array, int index, char *string);

char *get_string_at(const darray *array, int index);

void remove_string_at(darray *array, int index);

void replace_string_at(darray *array, int index, char *string);

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

void delete_without_undo(int addr1, int addr2, darray **lines_undone);

void undo(int number);

void undo_change(stack_node *undo_node);

void undo_delete(stack_node *undo_node);

void redo(int number);

void redo_change(int addr1, int addr2, darray *lines_to_rewrite);

void redo_delete(int addr1, int addr2);

bool resize_darray(darray *array, int new_capacity) {
    void *new_ptr = realloc(array->strings, sizeof(*(array->strings)) * new_capacity);

    if (new_ptr != NULL) {
        array->strings = new_ptr;
        array->capacity = new_capacity;
        return true;
    }
    return false;
}

bool enlarge_darray(darray *array) {
    return resize_darray(array, array->capacity + array->capacity * GROWTH_FACTOR + 1);
}

darray *new_darray(int initial_capacity) {
    darray *new_darray = malloc(sizeof(*new_darray));
    if (new_darray == NULL)
        return NULL;

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

bool append_string(darray *array, char *string) {
    if (size_darray(array) == array->capacity && !enlarge_darray(array)) {
        return false;
    }

    //allocates only the memory necessary for the given string
    array->strings[array->n] = malloc((strlen(string) + 1) * sizeof(char));
    if (array->strings[array->n] == NULL)
        return false;
    strcpy(array->strings[array->n], string);
    array->n++;
    return true;
}


bool add_string_at(darray *array, int index, char *string) {

    if (size_darray(array) == array->capacity && !enlarge_darray(array)) {
        return false;
    }

    //copy last element to index _n_ (size is now n + 1)
    append_string(array, get_string_at(array, size_darray(array) - 1));

    for (int i = size_darray(array) - 2; i > index; i--)
        replace_string_at(array, i, get_string_at(array, i - 1));

    replace_string_at(array, index, string);
    return true;
}


char *get_string_at(const darray *array, int index) {
    return array->strings[index];
}

void remove_string_at(darray *array, int index) {
    char *deleted_string = get_string_at(array, index);

    //shift all strings by one and free the deleted deleted_string
    for (int i = index + 1; i < size_darray(array); i++) {
        array->strings[i - 1] = array->strings[i];
    }

    array->n--;
    free(deleted_string);

    //todo make it return deleted string
    //return deleted_string;
}

void replace_string_at(darray *array, int index, char *string) {

    char *old_string = get_string_at(array, index);

    free(old_string);
    array->strings[index] = malloc((strlen(string) + 1) * sizeof(char));

    strcpy(array->strings[index], string);

    //todo make it return old string
    //return old_string;
}

void free_darray(darray *array) {
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
void push(stack_t *stack, char command, int addr1, int addr2, darray *lines_edited) {

    stack->size++;

    // Allocate the new node in the heap
    struct Node *node = malloc(sizeof(struct Node));

    // check if stack (heap) is full. Then inserting an element would
    // lead to stack overflow
    /*if (!node) {
        printf("\nHeap Overflow");
        exit(EXIT_FAILURE);
    }*/

    // set the command in allocated node
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

    /*
    //check for stack underflow
    if (stack->top == NULL) {
        printf("\nStack Underflow");
        exit(EXIT_FAILURE);
    }*/

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
    //freopen("Rolling_Back_2_input.txt", "r", stdin);
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

            free_stack(redo_stack);
            free_stack(undo_stack);
            free_darray(text_array);

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
    char input_line[STRING_LENGTH], c;
    //allocate darray only if written to
    //else put NULL in undo_stack->lines attribute
    darray *lines_edited = NULL;
    bool first_line_edited = true;

    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    while (true) {

        len = 0;
        do {
            c = (char) fgetc_unlocked(stdin);
            if (c == '\n' || c == '\0') {
                input_line[len] = 0;
                break;
            }
            input_line[len++] = c;
        } while (true);


        if (strcmp(input_line, ".") == 0) {
            //finished editing, save edited lines to undo stack
            push(undo_stack, 'c', addr1, addr2, lines_edited);
            //printUndoStack();
            return;
        }

        //add new string
        if (text_array->n == 0 || current_index >= text_array->n)
            append_string(text_array, input_line);

        else { //overwrite existing string

            if (first_line_edited)
                lines_edited = new_darray(INITIAL_CAPACITY);
            first_line_edited = false;

            append_string(lines_edited, get_string_at(text_array, current_index)); //save old string to undo stack
            replace_string_at(text_array, current_index, input_line); //edit (overwrite) existing string
        }

        current_index++;
    }

}

//todo optimization put first print bool outside loops
void print(int addr1, int addr2) {

    int current_line = addr1 - 1;

    execute_pending_undo();

    //\n appended before each line, except if it's first print
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
    int line_to_delete = addr1 - 1;
    int number_of_lines;
    int i = 0;
    //allocate darray only if written to
    //else put NULL in undo_stack->lines attribute
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
    last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    number_of_lines = last_index - addr1 + 1;

    while (i <= number_of_lines) {

        if (first_line_deleted)
            lines_deleted = new_darray(INITIAL_CAPACITY);
        first_line_deleted = false;

        //here lines_deleted is allocated for sure
        append_string(lines_deleted, get_string_at(text_array, line_to_delete)); //save deleted string to undo stack
        remove_string_at(text_array, line_to_delete);

        i++;
        first_print = false;
    }

    push(undo_stack, 'd', addr1, addr2, lines_deleted);

}

//deletes without saving on undo_stack, and saves on redo_stack instead
//by appending to lines_undone, which will be appended to redo_stack
void delete_without_undo(int addr1, int addr2, darray **lines_undone) {

    //checks if some of the lines to delete don't exist
    int last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    int number_of_lines = last_index - addr1 + 1;
    int line_to_delete = addr1 - 1;
    int i = 0;

    while (i <= number_of_lines) {

        //save string for redo
        append_string(*lines_undone, get_string_at(text_array, line_to_delete));
        //remove string from text
        remove_string_at(text_array, line_to_delete);

        i++;
        first_print = false;

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
        swap_stack(undo_stack, redo_stack, NULL);
        return;
    }

    darray *lines = undo_node->lines;
    darray *lines_undone = new_darray(INITIAL_CAPACITY);

    //if c has no lines at all it means it was only an addition without edits -> delete all
    //ONLY ADDED
    if (lines == NULL) {
        delete_without_undo(addr1, addr2, &lines_undone);
        swap_stack(undo_stack, redo_stack, lines_undone);
        return; //skip to next undo node
    }

    //replace edited strings with old ones
    int edited_lines_count = lines->n;

    //EDITED LINES (might have been also added)
    for (int j = 0; j < edited_lines_count; j++) {

        //save lines that will be overwritten by undo for redo stack
        append_string(lines_undone, get_string_at(text_array, addr1 + j - 1));
        //overwrite those lines
        replace_string_at(text_array, addr1 + j - 1, get_string_at(lines, j));

    }

    //if condition below is true:
    //first strings were edited [addr1, addr1 + edited_lines_count - 1] and already reverted in code above
    //last strings were added [addr1 + edited_lines_count, addr2] and need to be deleted

    //delete added strings
    //LINES ADDED AFTER EDITING PREVIOUS LINES
    if (addr2 - addr1 + 1 > edited_lines_count)
        delete_without_undo(addr1 + edited_lines_count, addr2, &lines_undone);

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
            append_string(text_array, lines->strings[j]);

        //deleted lines were between other lines
    } else if (addr1 <= text_array->n) {

        for (int j = 0; j < lines_to_add; j++)
            add_string_at(text_array, addr1 + j - 1, lines->strings[j]);

    }

    //don't need to save any line to redo_stack
    //redo of undo of delete is a simple delete: addr1, addr2 are sufficient
    swap_stack(undo_stack, redo_stack, NULL);
}

void redo(int number) {

    //pop and revert _number_ commands
    int i = 0;
    int addr1, addr2;

    while (i < number) {

        stack_node *redo_node = peek(redo_stack);

        addr1 = redo_node->addr1;
        addr2 = redo_node->addr2;

        if (redo_node->command == 'c')
            redo_change(addr1, addr2, redo_node->lines);
        else
            redo_delete(addr1, addr2);

        i++;
    }
}

//modified version of change
void redo_change(int addr1, int addr2, darray *lines_to_rewrite) {

    int current_text_index = addr1 - 1;
    int num_lines_to_rewrite = lines_to_rewrite->n;
    int i = 0;

    //allocate darray only if written to
    //else put NULL in undo_stack->lines_to_rewrite attribute
    darray *lines_edited = NULL;

    if (addr1 <= text_array->n && text_array->n > 0)
        lines_edited = new_darray(INITIAL_CAPACITY);

    while (i < num_lines_to_rewrite) {

        //add new string
        if (text_array->n == 0 || current_text_index >= text_array->n)
            append_string(text_array, get_string_at(lines_to_rewrite, i));

        else { //overwrite existing string
            append_string(lines_edited, get_string_at(text_array, current_text_index)); //save old string to undo stack
            replace_string_at(text_array, current_text_index,
                              get_string_at(lines_to_rewrite, i)); //edit (overwrite) existing string
        }

        current_text_index++;
        i++;
    }

    swap_stack(redo_stack, undo_stack, lines_edited);

}

void redo_delete(int addr1, int addr2) {

    if (!valid_addresses(addr1, addr2)) {
        swap_stack(redo_stack, undo_stack, NULL);
        return;
    }

    int last_index, number_of_lines;
    int line_to_delete = addr1 - 1;
    int i = 0;

    darray *lines_deleted = new_darray(INITIAL_CAPACITY);;

    //checks if some of the lines to delete don't exist
    last_index = addr2 >= text_array->n ? text_array->n - 1 : addr2 - 1;

    number_of_lines = last_index - addr1 + 1;

    while (i <= number_of_lines) {

        //here lines_deleted is allocated for sure
        append_string(lines_deleted, get_string_at(text_array, line_to_delete)); //save deleted string to undo stack
        remove_string_at(text_array, line_to_delete);

        i++;
        first_print = false;
    }

    swap_stack(redo_stack, undo_stack, lines_deleted);

}



