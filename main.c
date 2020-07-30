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

darray *text_array;
stack_node *undo_top;

int undo_stack_size;
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
 *               add_item_at(text_array, size_darray(text_array), item);
 */
bool append_string(darray *array, char *string);

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
char *remove_string_at(darray *array, long index);

/* replace_string_at:  replaces the item at position index with item and returns
 *                   the item previously at index.
 *                   If index is not a valid index for text_array, the behavior is
 *                   undefined.
 */
char *replace_string_at(darray *array, long index, char *string);

/*
 * free_darray:  frees memory occupied by text_array.
 */
void free_darray(darray *array);

bool contains_index(darray *array, long index);

bool valid_addresses(long addr1, long addr2);

void push(stack_node **top, char command, long addr1, long addr2, darray *edited_lines); // insert at the beginning

bool isEmpty(stack_node *top);

stack_node *peek(stack_node *top);

void pop(stack_node **top); // remove at the beginning

void change(long addr1, long addr2);

void print(long addr1, long addr2);

void delete(long addr1, long addr2);

void undo(long number);

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

    strcpy(array->strings[array->n], string); //todo strcpy or assign?

    array->n++;

    return true;
}

char *get_string_at(const darray *array, long index) {
    assert(index >= 0 && index < size_darray(array));

    return array->strings[index];
}

char *remove_string_at(darray *array, long index) {
    assert(index >= 0 && index < size_darray(array));

    char *string = get_string_at(array, index);

    //shift all strings by one and free the deleted string
    for (long i = index + 1; i < size_darray(array); i++) {
        array->strings[i - 1] = array->strings[i];
    }

    array->n--;
    free(string);

    return string;
}

char *replace_string_at(darray *array, long index, char *string) {
    assert(index >= 0 && index < size_darray(array));

    char *old_string = get_string_at(array, index);

    free(array->strings[index]);
    array->strings[index] = malloc((strlen(string) + 1) * sizeof(char));

    strcpy(array->strings[index], string);
    return old_string;
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
void push(stack_node **top, char command, long addr1, long addr2, darray *edited_lines) // insert at the beginning
{
    //editedLinesCount == edited_lines->n;
    //lines from addr1 to editedLinesCount - 1 are the EDITED lines (undo stack saves old version of these lines)
    //lines from editedLinesCount to addr2 are the ADDED lines
    // (undo stack doesn't save anything and remembers to delete them once an undo is called)

    //todo delete
    undo_stack_size++;

    // Allocate the new node in the heap
    struct Node *node = malloc(sizeof(struct Node));

    // check if stack (heap) is full. Then inserting an element would
    // lead to stack overflow
    if (!node) {
        //printf("\nHeap Overflow");
        exit(1);
    }

    //printf("Inserting %ld,%ld%c\n", addr1, addr2, command);

    //debugging
    //for (int i = 0; i < edited_lines->n; i++)
    //puts(edited_lines->strings[i]);


    // set the command in allocated node
    node->command = command;
    node->addr1 = addr1;
    node->addr2 = addr2;

    //to free up memory, deallocate if array is empty
    //remember that in this case node->lines == NULL
    //and check for it in undo function
    /*if (edited_lines->n == 0) {
        free_darray(edited_lines);
        edited_lines = NULL;
    }*/

    node->lines = edited_lines;

    // Set the .next pointer of the new node to point to the current
    // top node of the list
    node->next = *top;

    // update top pointer
    *top = node;
}

// Utility function to check if the stack is empty or not
bool isEmpty(stack_node *top) {
    return top == NULL;
}

// Utility function to return top element in a stack
stack_node *peek(stack_node *top) {
    // check for empty stack
    if (!isEmpty(top))
        return top;
    else
        exit(EXIT_FAILURE); //todo return NULL and check condition when called
}

// Utility function to pop top element from the stack
void pop(stack_node **top) // remove at the beginning
{
    stack_node *node;

    undo_stack_size--;

    //check for stack underflow
    if (*top == NULL) {
        //printf("\nStack Underflow");
        exit(1);
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

//debugging
//prints undo stack
void printUndoStack() {

    printf("\n\n\nUNDO STACK:\n");

    while (undo_stack_size > 1) {
        printf("\nstack size: %d\n",undo_stack_size);

        //debugging
        for (int i = 0; i < undo_top->lines->n; i++)
            puts(undo_top->lines->strings[i]);

        pop(&undo_top);
    }

}


int main() {
    //Time_for_a_change_1_input.txt
    //freopen("Bulk_Reads_1_input.txt", "r", stdin);
    //freopen("output.txt", "w+", stdout);
    char input[STRING_LENGTH];
    char *addrString1, *addrString2;
    char command;
    int addr1, addr2;
    unsigned int len;
    first_print = true;
    undo_stack_size = 0;

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
            printf("%d %c\n", addr1, command);
        } else if (command == 'r') { //redo
            //numbers is already a single number
            addr1 = atoi(input);
            printf("%d %c\n", addr1, command);
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


    while (true) {

        fgets(input_line, STRING_LENGTH, stdin);
        input_line[strlen(input_line) - 1] = '\0'; //removes \n

        if (strcmp(input_line, ".") == 0) {
            //finished editing, save edited lines to undo stack
            push(&undo_top, 'c', addr1, addr2, lines_edited);
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

    //might add boolean flag for invalid commands in undo stack

    //push(&undo_top,'c',addr1,addr2,lines_deleted);

    if (!valid_addresses(addr1, addr2)) {
        push(&undo_top, 'd', addr1, addr2, lines_deleted);
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

    push(&undo_top, 'd', addr1, addr2, lines_deleted);

}

void undo(long number) {
    //pop and revert _number_ commands
}

