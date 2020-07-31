#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#define INITIAL_CAPACITY 0
#define STRING_LENGTH 1025

typedef struct {
    int capacity;   //max number of strings it can contain
    int n;  //number of strings stored
    char **strings;
} darray;

// A linked list node
typedef struct Node {
    char command; //command
    long addr1;
    long addr2;
    darray *lines;
    struct Node *next; //pointer to the next node
} stack_node;

typedef struct {
    stack_node *top;
    int size; //number of commands saved in stack
    int pending; //undos waiting to be executed
    bool is_redoable;
} stack_t;

darray *text_array;
stack_t *undo_stack;
stack_t *redo_stack;

bool first_print;   //true if a line has already been printed

/*
 * resize_darray:  changes text_array total capacity to new_capacity and returns
 *                 true. On failure returns false and leaves text_array untouched.
 */
bool resize_darray(darray *array, int new_capacity);

/*
 * enlarge_darray:  increases the total capacity of text_array by a factor of about
 *                  1.5 and returns true. On failure returns false and leaves
 *                  text_array untouched.
 *
 *                  The formula used to calculate new capacity is:
 *                  new_capacity = old_capacity + old_capacity / 2 + 1
 */
bool enlarge_darray(darray *array);

/*
 * new_darray:  creates and returns (a pointer to) a new darray of capacity
 *                 INITIAL_CAPACITY. On failure returns NULL.
 */
darray *new_darray(int initial_capacity);

/*
 * size_darray:  returns the number of strings stored in text_array.
 */
int size_darray(const darray *array);

/*
 * append_string:  inserts item at the end of text_array. It is equivalent to:
 *               add_string_at(text_array, size_darray(text_array), item);
 */
bool append_string(darray *array, char *string);

bool add_string_at(darray *array, long index, char *string);


/*
 * get_string_at:  returns (but does not remove) the item at position index.
 *               If index is not a valid index for text_array, the behavior is
 *               undefined.
 */
char *get_string_at(const darray *array, long index);

/*
 * remove_string_at:  removes and returns the item at position index shifting
 *                  other strings to the left by one position.
 *                  If index is not a valid index for text_array, the behavior is
 *                  undefined.
 */
void remove_string_at(darray *array, long index);

/* replace_string_at:  replaces the item at position index with item and returns
 *                   the item previously at index.
 *                   If index is not a valid index for text_array, the behavior is
 *                   undefined.
 */
void replace_string_at(darray *array, long index, char *string);

/*
 * free_darray:  frees memory occupied by text_array.
 */
void free_darray(darray *array);

bool contains_index(darray *array, long index);

bool valid_addresses(long addr1, long addr2);

void push(stack_t *stack, char command, long addr1, long addr2, darray *lines_edited); // insert at the beginning

bool isEmpty(stack_t *stack);

stack_node *peek(stack_t *stack);

void pop(stack_t *stack); // remove at the beginning

void increment_pending_undo(int number);

void decrement_pending_undo(int number);

void reset_pending_undo();

void execute_undo();

void change(long addr1, long addr2);

void print(long addr1, long addr2);

void delete(long addr1, long addr2);

void delete_no_undo(long addr1, long addr2, darray *lines_undone);

void undo(long number);

void redo(long number);

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
    return resize_darray(array, array->capacity + array->capacity / 2 + 1);
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


bool add_string_at(darray *array, long index, char *string) {

    assert(index >= 0 && index <= size_darray(array));

    if (size_darray(array) == array->capacity && !enlarge_darray(array)) {
        return false;
    }

    //array->n++;

    //copy last element to index _n_ (size is now n + 1)
    append_string(array, get_string_at(array, size_darray(array) - 1));

    for (int i = size_darray(array) - 2; i > index; i--) {
        //printf("\n");
        //puts(get_string_at(array, i - 1));
        replace_string_at(array, i, get_string_at(array, i - 1));
        //array->strings[i] = array->strings[i - 1];
    }
    replace_string_at(array, index, string);
    return true;
}


char *get_string_at(const darray *array, long index) {
    assert(index >= 0 && index < size_darray(array));
    return array->strings[index];
}

void remove_string_at(darray *array, long index) {
    assert(index >= 0 && index < size_darray(array));

    char *deleted_string = get_string_at(array, index);

    //shift all strings by one and free the deleted deleted_string
    for (long i = index + 1; i < size_darray(array); i++) {
        array->strings[i - 1] = array->strings[i];
    }

    array->n--;
    free(deleted_string);

    //todo make it return deleted string
    //return deleted_string;
}

void replace_string_at(darray *array, long index, char *string) {
    assert(index >= 0 && index < size_darray(array));

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

bool contains_index(darray *array, long index) {
    return index >= 0 && index < array->n;
}

bool valid_addresses(long addr1, long addr2) {
    return addr1 > 0 && addr2 > 0 && addr1 <= addr2 && (addr1 <= text_array->n || addr1 == 1);
}

/*
 * Stack functions
 */

// Utility function to add an element command in the stack
void push(stack_t *stack, char command, long addr1, long addr2, darray *lines_edited) // insert at the beginning
{
    stack_node **top = &(stack->top);
    stack->size++;

    // Allocate the new node in the heap
    struct Node *node = malloc(sizeof(struct Node));

    // check if stack (heap) is full. Then inserting an element would
    // lead to stack overflow
    if (!node) {
        printf("\nHeap Overflow");
        exit(EXIT_FAILURE);
    }
    //printf("Inserting %ld,%ld%c\n", addr1, addr2, command);

    //debugging
    //for (int i = 0; i < lines_edited->n; i++)
    //puts(lines_edited->strings[i]);

    // set the command in allocated node
    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;

    //to free up memory, deallocate if array is empty
    //remember that in this case node->lines == NULL
    //and check for it in undo function
    /*if (lines_edited->n == 0) {
        free_darray(lines_edited);
        lines_edited = NULL;
    }*/

    node->lines = lines_edited;
    // Set the .next pointer of the new node to point to the current
    // top node of the list
    node->next = *top;
    // update top pointer
    *top = node;
}

// Utility function to check if the stack is empty or not
bool isEmpty(stack_t *stack) {
    return stack->top == NULL;
}

// Utility function to return top element in a stack
stack_node *peek(stack_t *stack) {
    // check for empty stack
    if (!isEmpty(stack))
        return stack->top;
    else {
        /*
        printf("\npeek failure\n");
        exit(EXIT_FAILURE);*/ //return NULL and check condition when called
        return NULL;
    }
}

// Utility function to pop top element from the stack
void pop(stack_t *stack) // remove at the beginning
{
    stack_node **top = &(stack->top);
    stack_node *node;
    stack->size--;

    //check for stack underflow
    if (*top == NULL) {
        printf("\nStack Underflow");
        exit(EXIT_FAILURE);
    }
    //stack_node *peeked = peek(*top);
    //printf("Removing %ld,%ld%c\n", peeked->addr1, peeked->addr2, peeked->command);
    node = *top;
    //update the top pointer to point to the next node
    *top = (*top)->next;
    //free memory for array of strings
    free_darray(node->lines);
    //free memory for node popped
    free(node);
}

void increment_pending_undo(int number) {
    undo_stack->pending += number;
    undo_stack->is_redoable = true; //because increment_pending_undo == undo input

    if (undo_stack->pending > undo_stack->size)
        undo_stack->pending = undo_stack->size;
}

void decrement_pending_undo(int number) {
    undo_stack->pending -= number;

    if (undo_stack->pending < 0)
        undo_stack->pending = 0;
}

void reset_pending_undo() {
    undo_stack->pending = 0;
    //undo_stack->is_redoable = false;
}

//executes all pending undos
void execute_undo() {
    if (undo_stack->pending == 0)
        return;

    undo(undo_stack->pending); //executes all pending undos
    reset_pending_undo(); //sets pending counter to 0
}

//debugging
//prints undo stack
/*
void printUndoStack() {

    printf("\n\n\nUNDO STACK:\n");

    while (size > 1) {
        printf("\nstack size: %d\n", size);

        //debugging
        for (int i = 0; i < top->lines->n; i++)
            puts(top->lines->strings[i]);

        pop(&top);
    }

}*/


int main() {
    //freopen("Rolling_Back_2_without_r.txt", "r", stdin);
    //freopen("output.txt", "w+", stdout);
    char input[STRING_LENGTH];
    char *addrString1, *addrString2;
    char command;
    int addr1, addr2;
    unsigned int len;
    first_print = true;

    undo_stack = malloc(sizeof(*undo_stack));
    undo_stack->top = NULL;
    undo_stack->size = 0;
    undo_stack->pending = 0;
    undo_stack->is_redoable = false;

    //unused attributes (to replace with global variables): pending, is_redoable
    redo_stack = malloc(sizeof(*redo_stack));
    undo_stack->top = NULL;
    undo_stack->size = 0;
    //undo_stack->pending = 0; //might be useless
    //undo_stack->is_redoable = false; //useless

    text_array = new_darray(INITIAL_CAPACITY);

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

            increment_pending_undo(addr1);
        } else if (command == 'r') { //redo
            addr1 = atoi(input);

            //todo only valid if undo precedes redo
            redo(addr1);
            //decrement_pending_undo(addr1);
        } else if (command == 'q') { //quit
            //printUndoStack();
            return 0;
        } else {
            printf("%c", command);
            puts("invalid input");
            return -1;
        }
    }

}

void change(long addr1, long addr2) {

    long current_index = addr1 - 1;
    char input_line[STRING_LENGTH];
    darray *lines_edited = new_darray(INITIAL_CAPACITY);

    //if(has_pending_undo())
    execute_undo();
    //todo clear redo

    undo_stack->is_redoable = false;
    //reset_pending_undo();

    while (true) {

        fgets(input_line, STRING_LENGTH, stdin);
        input_line[strlen(input_line) - 1] = '\0'; //removes \n

        if (strcmp(input_line, ".") == 0) {
            //finished editing, save edited lines to undo stack
            push(undo_stack, 'c', addr1, addr2, lines_edited);
            return;
        }

        if (text_array->n == 0 || current_index >= text_array->n)
            append_string(text_array, input_line); //add new string
        else {
            append_string(lines_edited, get_string_at(text_array, current_index)); //save old string to undo stack
            replace_string_at(text_array, current_index, input_line); //edit (overwrite) existing string
        }
        current_index++;

    }

}

void print(long addr1, long addr2) {

    //if(has_pending_undo())
    execute_undo();

    long current_line = addr1 - 1;

    //\n appended before each line, except if it's first print
    if (current_line < 0) {
        if (!first_print) printf("\n");
        printf(".");
        return;
    }

    while (current_line <= addr2 - 1) {

        if (!first_print) printf("\n");

        if (contains_index(text_array, current_line))
            printf("%s", get_string_at(text_array, current_line));
        else
            printf(".");

        current_line++;
        first_print = false;

    }
}

void delete(long addr1, long addr2) {

    long last_index;
    long line_to_delete = addr1 - 1;
    long number_of_lines;
    long i = 0;
    darray *lines_deleted = new_darray(INITIAL_CAPACITY);

    //if(has_pending_undo())
    execute_undo();
    //todo clear redo

    //reset_pending_undo();
    undo_stack->is_redoable = false;

    //might add boolean flag for invalid commands in undo stack


    if (!valid_addresses(addr1, addr2)) {
        push(undo_stack, 'd', addr1, addr2, lines_deleted);
        return;
    }

    //checks if some of the lines to delete don't exist
    if (addr2 >= text_array->n)
        last_index = text_array->n - 1;
    else
        last_index = addr2 - 1;

    number_of_lines = last_index - addr1 + 1;

    while (i <= number_of_lines) {

        if (contains_index(text_array, line_to_delete)) {
            append_string(lines_deleted, get_string_at(text_array, line_to_delete)); //save deleted string to undo stack
            remove_string_at(text_array, line_to_delete);
        } else
            break; //if doesn't contain line is already outside the existing range

        i++;
        first_print = false;

    }

    push(undo_stack, 'd', addr1, addr2, lines_deleted);

}

void delete_no_undo(long addr1, long addr2, darray *lines_undone) {

    long last_index;
    long line_to_delete = addr1 - 1;
    long number_of_lines;
    long i = 0;

    if (!valid_addresses(addr1, addr2))
        return;

    //checks if some of the lines to delete don't exist
    if (addr2 >= text_array->n)
        last_index = text_array->n - 1;
    else
        last_index = addr2 - 1;

    number_of_lines = last_index - addr1 + 1;

    while (i <= number_of_lines) {

        if (contains_index(text_array, line_to_delete)) {
            //save string for redo
            append_string(lines_undone, get_string_at(text_array, line_to_delete));
            //remove string from text
            remove_string_at(text_array, line_to_delete);
        } else
            break; //if doesn't contain line is already outside the existing range
        i++;
        first_print = false;

    }

}

void redo_delete(long addr1, long addr2) {

    long last_index;
    long line_to_delete = addr1 - 1;
    long number_of_lines;
    long i = 0;

    if (!valid_addresses(addr1, addr2))
        return;

    //checks if some of the lines to delete don't exist
    if (addr2 >= text_array->n)
        last_index = text_array->n - 1;
    else
        last_index = addr2 - 1;

    number_of_lines = last_index - addr1 + 1;

    while (i <= number_of_lines) {

        if (contains_index(text_array, line_to_delete)) {
            //remove string from text
            remove_string_at(text_array, line_to_delete);
        } else
            break; //if doesn't contain line is already outside the existing range
        i++;
        first_print = false;

    }

}


//invisible to undo / redo
void redo_change(long addr1, long addr2, darray *lines) {

    long current_index = addr1 - 1;
    int i = 0, n = lines->n;

    while (i < n) {

        if (text_array->n == 0 || current_index >= text_array->n)
            append_string(text_array, lines->strings[i]); //add new string
        else
            replace_string_at(text_array, current_index, lines->strings[i]); //edit (overwrite) existing string

        current_index++;
        i++;
    }

}


//editedLinesCount == edited_lines->n;
//lines from addr1 to editedLinesCount - 1 are the EDITED lines (undo stack saves old version of these lines)
//lines from editedLinesCount to addr2 are the ADDED lines
// (undo stack doesn't save anything and remembers to delete them once an undo is called)
void undo(long number) {
    //pop and revert _number_ commands
    int i = 0, edited_lines_count;
    long addr1, addr2;
    stack_node *node;
    darray *lines_undone = new_darray(INITIAL_CAPACITY);

    while (i < number) {

        node = peek(undo_stack);
        if (node == NULL)
            return; //undo stack is empty

        addr1 = node->addr1;
        addr2 = node->addr2;

        //undo change
        if (node->command == 'c') {

            //replace edited strings with old ones
            edited_lines_count = node->lines->n;

            for (int j = 0; j < edited_lines_count; j++) {
                //save string that is being undone to array for redo stack
                append_string(lines_undone, get_string_at(text_array, addr1 + j - 1));
                replace_string_at(text_array, addr1 + j - 1, node->lines->strings[j]);
            }

            //delete added strings
            if (addr2 - addr1 + 1 > edited_lines_count) {
                delete_no_undo(addr1 + edited_lines_count, addr2, lines_undone);
            }

        } else { //undo delete
            int lines_to_add = node->lines->n;
            //todo implement delete redo

            if (lines_to_add != 0) {

                //deleted lines were at the end of text
                if (node->addr1 > text_array->n) {
                    for (int j = 0; j < lines_to_add; j++) {
                        append_string(text_array, node->lines->strings[j]);
                    }
                } else if (node->addr1 <= text_array->n) {
                    //deleted lines were between other lines
                    for (int j = 0; j < lines_to_add; j++) {
                        add_string_at(text_array, addr1 + j - 1, node->lines->strings[j]);
                    }
                }
            } //else the delete was invalid and nothing was actually deleted

        }

        push(redo_stack, node->command, addr1, addr2, lines_undone);
        pop(undo_stack);
        i++;
    }
}

void redo(long number) {

    if (!undo_stack->is_redoable) {
        return;
    }



    //pop and revert _number_ commands
    int i = 0, edited_lines_count;
    long addr1, addr2;
    stack_node *node;
    darray *lines_undone = new_darray(INITIAL_CAPACITY); //todo ???

    while (i < number) {

        node = peek(redo_stack);
        if (node == NULL)
            return;

        addr1 = node->addr1;
        addr2 = node->addr2;

        //undo change
        if (node->command == 'c') {

            //replace edited strings with old ones
            edited_lines_count = node->lines->n;

            for (int j = 0; j < edited_lines_count; j++) {
                if (addr1 + j - 1 >= text_array->n)
                    append_string(text_array, node->lines->strings[j]);
                else
                    replace_string_at(text_array, addr1 + j - 1, node->lines->strings[j]);
            }
            /*//todo check this out(?)
            //add deleted strings
            if (addr2 - addr1 + 1 > edited_lines_count) {
                redo_change(addr1 + edited_lines_count, addr2, lines_undone);
            }*/

        } else { //undo delete
            int lines_to_add = node->lines->n;

            redo_delete(addr1, addr2);

        }

        //push(redo_stack, node->command, addr1, addr2, lines_undone);
        pop(redo_stack);
        i++;
    }


    decrement_pending_undo(number);

}

