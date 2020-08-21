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
    char command; //command
    int addr1;
    int addr2;
    darray *lines;
    darray *new_lines;
    struct Node *next; //pointer to the next node
} stack_node;

typedef struct {
    stack_node *top;
    int size; //number of commands saved in stack
} stack_type;

darray *text_array;
stack_type *undo_stack;
stack_type *redo_stack;

bool first_print;   //true if a line has already been printed
bool is_redoable;
int pending;


void resize_darray(darray *array, int new_capacity);

void enlarge_darray(darray *array);

darray *new_darray(int initial_capacity);

char *append_string_by_copy(darray *array, char *string);

char *get_string_at(const darray *array, int index);

void remove_string_at(darray *array, int index);

char *save_and_replace(darray *save_array, darray *write_array, char *new_line, int index);

void save_and_remove(darray *lines_deleted, darray *text, int index);

void free_darray(darray *array);

bool contains_index(darray *array, int index);

bool valid_address(int addr1);

void push(stack_type *stack, char command, int addr1, int addr2, darray *lines_to_save,
          darray *new_lines); // insert at the beginning

stack_node *peek(stack_type *stack);

void swap_stack(stack_type *sender_s, stack_type *receiver_s);

void execute_pending_undo();

void change(int addr1, int addr2);

void print(int addr1, int addr2);

void delete(int addr1, int addr2);

void delete_without_undo(int addr1, int addr2);

void remove_lines(darray *text, int index_to_delete, int number_of_lines);

void undo(int number);

void undo_change(stack_node *undo_node);

void undo_delete(stack_node *undo_node);

void redo(int number);

void redo_change(stack_node *redo_node);

void redo_delete(stack_node *redo_node);

void resize_darray(darray *array, int new_capacity) {
    array->strings = realloc(array->strings, sizeof(*(array->strings)) * new_capacity);
    array->capacity = new_capacity;
}

void enlarge_darray(darray *array) {
    resize_darray(array, array->capacity * GROWTH_FACTOR + 1);
}

darray *new_darray(int initial_capacity) {
    darray *new_darray = malloc(sizeof(*new_darray));

    new_darray->capacity = 0;
    new_darray->n = 0;
    new_darray->strings = NULL;

    resize_darray(new_darray, initial_capacity);

    return new_darray;
}

void append_string_by_reference(darray *array, char *string) {

    if (array->n == array->capacity)
        enlarge_darray(array);

    array->strings[array->n] = string;
    array->n++;

}

char *append_string_by_copy(darray *array, char *string) {

    if (array->n == array->capacity)
        enlarge_darray(array);

    int len = strlen(string) + 1;

    array->strings[array->n] = malloc(len);
    memcpy(array->strings[array->n], string, len);
    array->n++;

    return array->strings[array->n - 1];

}

void insert_string_by_reference(darray *array, int index, char *string) {

    if (array->n == array->capacity)
        enlarge_darray(array);

    array->n++;

    for (int i = array->n - 1; i > index; i--)
        array->strings[i] = array->strings[i - 1];

    array->strings[index] = string;
}


char *get_string_at(const darray *array, int index) {
    return array->strings[index];
}

void remove_string_at(darray *array, int index) {

    //shift all strings by one and free the deleted deleted_string
    for (int i = index + 1; i < array->n; i++)
        array->strings[i - 1] = array->strings[i];

    array->n--;
}

void replace_string_by_reference(darray *array, int index, char *string) {
    array->strings[index] = string;
}

//saves overwritten string write_array->strings[index] in save_array (appends)
//and writes new_line in its place
char *save_and_replace(darray *save_array, darray *write_array, char *new_line, int index) {
    //equivalent to:
    //append_string_by_copy(lines_edited, get_string_at(text_array, current_text_index)); //save old string to undo stack
    //replace_string_at(text_array, current_text_index, get_string_at(lines_to_rewrite, i)); //overwrite in text

    //append to save_array
    if (save_array->n == save_array->capacity)
        enlarge_darray(save_array);


    //save pointer to old string in save_array
    save_array->strings[save_array->n] = write_array->strings[index];
    save_array->n++;

    int len = strlen(new_line) + 1;

    //allocate new string that position, losing pointer to old string, that is now saved in save_array
    write_array->strings[index] = malloc(len);
    memcpy(write_array->strings[index], new_line, len);

    return write_array->strings[index];

}

void save_and_remove(darray *lines_deleted, darray *text, int index) {
    //equivalent to:
    //append_string_by_copy(lines_deleted, get_string_at(text_array, line_to_delete)); //save deleted string to undo stack
    //remove_string_at(text_array, line_to_delete);

    if (lines_deleted->n == lines_deleted->capacity)
        enlarge_darray(lines_deleted);


    //append to lines deleted
    lines_deleted->strings[lines_deleted->n] = text->strings[index];
    lines_deleted->n++;

    //shift all strings by one
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

bool valid_address(int addr1) {
    return addr1 > 0 && (addr1 <= text_array->n || addr1 == 1);
}

void push(stack_type *stack, char command, int addr1, int addr2, darray *lines_to_save, darray *new_lines) {

    stack->size++;

    struct Node *node = malloc(sizeof(struct Node));

    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;
    node->lines = lines_to_save;
    node->new_lines = new_lines;

    node->next = stack->top;
    stack->top = node;
}


//returns top element
stack_node *peek(stack_type *stack) {
    return stack->top; //NULL if stack is empty, CHECK CONDITION
}

//append top of undo_stack to redo_stack
//sender: undo; receiver: redo
void swap_stack(stack_type *sender_s, stack_type *receiver_s) {
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
    redo_stack->size = 0;
}

//debugging
void print_stack(stack_type *stack) {

    printf("\n%d nodes in stack:\n", stack->size);
    stack_node *node = stack->top;
    while (node != NULL) {
        printf("node: %d,%d%c\n", node->addr1, node->addr2, node->command);
        if (node->lines != NULL) {
            printf("lines:\n");
            for (int i = 0; i < node->lines->n; i++) {
                printf("%d ", i);
                puts(node->lines->strings[i]);
            }
        }
        if (node->new_lines != NULL) {
            printf("new_lines:\n");
            for (int i = 0; i < node->new_lines->n; i++) {
                printf("%d ", i);
                puts(node->new_lines->strings[i]);
            }
        }

        node = node->next;
    }

}

void print_darray(darray *array) {

    printf("\n\ndarray print: \n\n");
    for (int i = 0; i < array->n; i++)
        puts(array->strings[i]);

    printf("end of print\n");

}


int main() {
    //Rolling_Back_2_input
    //Altering_History_2_input
    //simple_redo_input
    //freopen("Altering_History_2_input.txt", "r", stdin);
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


/*
c
c
_lines_ is overwritten lines
_new_lines_ is new lines added (either overwriting or appended)

d
_lines_ is deleted lines
_new_lines_ is NULL

undo_change
_lines_ overwrites text, appended lines are deleted
those appended lines had been previously saved in _new_lines_ (used in redo_change)
so there's no need to save them again (as done in prev version)

undo_delete
_lines_ are inserted (or appended to text)

redo_change
swap "lines" with "new_lines" in code
ie overwrite text with new_lines (overwriting or appended)

redo_delete
just delete. deleted lines are already saved in _lines_
*/



void change(int addr1, int addr2) {

    int current_index = addr1 - 1;
    char input_line[STRING_LENGTH];
    darray *lines_edited = NULL;
    darray *new_lines = new_darray(INITIAL_CAPACITY);
    bool first_line_edited = true;

    execute_pending_undo();
    clear_redo();

    is_redoable = false;

    while (true) {
        char *new_string = NULL;

        fgets(input_line, STRING_LENGTH, stdin);
        input_line[strlen(input_line) - 1] = '\0'; //removes \n

        if (strcmp(input_line, ".") == 0) {
            //save edited (and not added) lines to undo stack
            push(undo_stack, 'c', addr1, addr2, lines_edited, new_lines);
            return;
        }

        if (current_index < text_array->n) { //overwrite existing string

            if (first_line_edited)
                lines_edited = new_darray(INITIAL_CAPACITY);
            first_line_edited = false;

            //save old string to lines_edited for undo stack and overwrite it with input_line
            new_string = save_and_replace(lines_edited, text_array, input_line, current_index);
        } else //add new string
            new_string = append_string_by_copy(text_array, input_line);

        append_string_by_reference(new_lines, new_string);

        current_index++;
    }

}

//optimization: put first_print bool outside loops
void print(int addr1, int addr2) {

    int current_index = addr1 - 1;

    execute_pending_undo();

    //append \n before each line, except if it's first print
    if (current_index < 0) {
        if (!first_print) fputc('\n', stdout);
        fputc('.', stdout);
        return;
    }

    while (current_index <= addr2 - 1) {

        if (!first_print) fputc('\n', stdout);

        if (contains_index(text_array, current_index)) {
            fputs(get_string_at(text_array, current_index), stdout);
        } else
            fputc('.', stdout);

        current_index++;
        first_print = false;
    }
}

//todo
void delete(int addr1, int addr2) {

    execute_pending_undo();
    clear_redo();
    is_redoable = false;


    //checks if some of the lines to delete don't exist
    int n = text_array->n;

    int first_index = addr1 - 1;
    int last_index = addr2 >= n ? n - 1 : addr2 - 1;
    int num_to_delete = last_index - first_index + 1;
    darray *lines_deleted = NULL;


    if (!valid_address(addr1) || num_to_delete <= 0) {
        //if delete is invalid node->lines == NULL
        push(undo_stack, 'd', addr1, addr2, NULL, NULL);
        return;
    }

    first_print = false;

    lines_deleted = new_darray(num_to_delete);


    for (int i = first_index; i <= last_index; i++)
        append_string_by_reference(lines_deleted, text_array->strings[i]);

    for (int i = first_index; i < n; i++)
        text_array->strings[i] = text_array->strings[num_to_delete + i];

    text_array->n -= num_to_delete;

    push(undo_stack, 'd', addr1, addr2, lines_deleted, NULL);

}

//deletes without saving on undo_stack
void delete_without_undo(int addr1, int addr2) {

    //checks if some of the lines to delete don't exist

    int n = text_array->n;
    int last_index = addr2 >= n ? n - 1 : addr2 - 1;
    int num_to_delete = last_index - addr1 + 2;
    int first_index = addr1 - 1;

    for (int i = first_index; i < n; i++)
        text_array->strings[i] = text_array->strings[num_to_delete + i];

    text_array->n -= num_to_delete;

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

void replace_text_lines(darray *text, stack_node *node, int addr1, int num_edited_lines) {

    //no need to save overwritten strings,
    //they're already saved in node->new_lines
    //make text strings point to new undo_lines, saved in undo node (undo_lines)

    //text_index = addr1 + i - 1;
    for (int i = 0; i < num_edited_lines; i++)
        replace_string_by_reference(text, addr1 + i - 1, node->lines->strings[i]);

}

//appends all lines in node to text
void append_node_lines_to_text(darray *text, stack_node *node, int lines_to_add) {

    for (int i = 0; i < lines_to_add; i++)
        append_string_by_reference(text, node->lines->strings[i]);

}

void remove_lines(darray *text, int index, int num_lines_to_remove) {

    for (int i = 0; i < num_lines_to_remove; i++) {
        //remove_string_at(text, index);

        //shift all strings by one
        for (int i = index + 1; i < text->n; i++)
            text->strings[i - 1] = text->strings[i];

        text->n--;
    }
}

void insert_node_lines_in_text(stack_node *node, int addr1) {

    int lines_to_add = node->lines->n;

    for (int j = 0; j < lines_to_add; j++) {
        //insert_string_by_reference(text_array, addr1 + j - 1, node->lines->strings[j]);

        int text_index = addr1 + j - 1;

        if (text_array->n == text_array->capacity)
            enlarge_darray(text_array);

        text_array->n++;

        //shift all elements
        for (int i = text_array->n - 1; i > text_index; i--)
            text_array->strings[i] = text_array->strings[i - 1];

        text_array->strings[text_index] = node->lines->strings[j];

    }


}


void insert(stack_node *node, int addr1) {

    int num_added = node->lines->n;
    int old_n = text_array->n;
    int new_n = old_n + num_added;

    if (new_n > text_array->capacity)
        resize_darray(text_array, new_n);

    text_array->n = new_n;

    //shift all elements
    for (int i = new_n - 1; i >= addr1 - 1; i--)
        text_array->strings[i] = text_array->strings[i - num_added];

    //insert new ones
    for (int j = 0; j < num_added; j++)
        text_array->strings[addr1 + j - 1] = node->lines->strings[j];

}


void undo_change(stack_node *undo_node) {

    int addr1 = undo_node->addr1;
    int addr2 = undo_node->addr2;

    //if c has no undo_lines at all it means it was only an addition without edits -> delete all
    //ONLY ADDED
    if (undo_node->lines == NULL) {
        delete_without_undo(addr1, addr2);
        swap_stack(undo_stack, redo_stack);
        return; //skip to next undo node
    }

    //replace edited strings with old ones
    int num_edited_lines = undo_node->lines->n;
    int num_changed_lines = addr2 - addr1 + 1;


    //EDITED LINES
    //lines added will be handled by code below
    if (num_edited_lines > 0)
        replace_text_lines(text_array, undo_node, addr1, num_edited_lines);

    //LINES ADDED AFTER EDITING PREVIOUS LINES
    if (num_changed_lines > num_edited_lines)
        delete_without_undo(addr1 + num_edited_lines, addr2); //delete added strings

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
    else
        insert(undo_node, addr1);

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
    darray *new_lines = redo_node->new_lines;

    int last_line_edited = addr2 > text_array->n ? text_array->n : addr2;
    int num_lines_overwritten = last_line_edited - addr1 + 1;
    int num_lines_changed = addr2 - addr1 + 1;
    int num_lines_added = num_lines_changed - num_lines_overwritten;


    if (num_lines_overwritten > 0) { //RESTORE EDITED LINES

        //make text strings point to new_lines, saved in undo node
        //text_index = addr1 + i - 1;
        for (int i = 0; i < num_lines_overwritten; i++)
            replace_string_by_reference(text_array, addr1 + i - 1, new_lines->strings[i]);

    }

    if (num_lines_added > 0) { //RESTORE DELETED LINES

        for (int i = num_lines_overwritten; i < num_lines_changed; i++)
            append_string_by_reference(text_array, new_lines->strings[i]);

        redo_node->new_lines->n -= num_lines_added;
    }

    swap_stack(redo_stack, undo_stack);

}

void redo_delete(stack_node *redo_node) {

    int addr1 = redo_node->addr1;

    if (!valid_address(addr1)) {
        swap_stack(redo_stack, undo_stack);
        return;
    }

    delete_without_undo(addr1, redo_node->addr2);
    swap_stack(redo_stack, undo_stack);

}

