/*
 * Dynamic array.
 *
 * from Wikipedia: "...is a random access, variable-size list data structure
 * that allows elements to be added or removed... Dynamic arrays
 * overcome a limit of static arrays, which have a fixed capacity that needs to
 * be specified at allocation."
 *
 * Indexing                   O(1)
 * Insert/delete at beginning O(n)
 * Insert/delete at end       O(1) amortized
 * Insert/delete at middle    O(n)
 * Average space              O(n)
 *
 * https://en.wikipedia.org/wiki/Dynamic_array
 */


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

darray *array;

/*
 * resize_darray:  changes array total capacity to new_capacity and returns
 *                 true. On failure returns false and leaves array untouched.
 */
static bool resize_darray(darray *array, int new_capacity);

/*
 * enlarge_darray:  increases the total capacity of array by a factor of about
 *                  1.5 and returns true. On failure returns false and leaves
 *                  array untouched.
 *
 *                  The formula used to calculate new capacity is:
 *                  new_capacity = old_capacity + old_capacity / 2 + 1
 */
static bool enlarge_darray(darray *array);

/*
 * new_darray:  creates and returns (a pointer to) a new darray of capacity
 *                 INITIAL_CAPACITY. On failure returns NULL.
 */
darray *new_darray(void);

/*
 * size_darray:  returns the number of strings stored in array.
 */
int size_darray(const darray *array);

/*
 * append_item:  inserts item at the end of array. It is equivalent to:
 *               add_item_at(array, size_darray(array), item);
 */
bool append_item(darray *array, char *string);

/*
 * get_string_at:  returns (but does not remove) the item at position index.
 *               If index is not a valid index for array, the behavior is
 *               undefined.
 */
char *get_string_at(const darray *array, int index);

/*
 * remove_item_at:  removes and returns the item at position index shifting
 *                  other strings to the left by one position.
 *                  If index is not a valid index for array, the behavior is
 *                  undefined.
 */
char *remove_item_at(darray *array, int index);

/* replace_string_at:  replaces the item at position index with item and returns
 *                   the item previously at index.
 *                   If index is not a valid index for array, the behavior is
 *                   undefined.
 */
char *replace_string_at(darray *array, int index, char *string);

/*
 * free_darray:  frees memory occupied by array.
 */
void free_darray(darray *array);


void change(unsigned long addr1, unsigned long addr2);

void print();

static bool resize_darray(darray *array, int new_capacity) {
    void *new_ptr = realloc(array->strings,
                            sizeof(*(array->strings)) * new_capacity);
    if (new_ptr != NULL) {
        array->strings = new_ptr;
        array->capacity = new_capacity;
        return true;
    }
    return false;
}

static bool enlarge_darray(darray *array) {
    return resize_darray(array, array->capacity + array->capacity / 2 + 1);
}

darray *new_darray(void) {
    darray *new_darray = malloc(sizeof(*new_darray));
    if (new_darray == NULL) {
        return NULL;
    }

    new_darray->capacity = 0;
    new_darray->n = 0;
    new_darray->strings = NULL;
    if (!resize_darray(new_darray, INITIAL_CAPACITY)) {
        free_darray(new_darray);
        return NULL;
    }

    return new_darray;
}

int size_darray(const darray *array) {
    return array->n;
}

bool append_item(darray *array, char *string) {

    if (size_darray(array) == array->capacity && !enlarge_darray(array)) {
        return false;
    }

    array->strings[array->n] = malloc(STRING_LENGTH * sizeof(char));

    if (array->strings[array->n] == NULL)
        return false;

    strcpy(array->strings[array->n], string);

    array->n++;

    return true;
}

char *get_string_at(const darray *array, int index) {
    assert(index >= 0 && index < size_darray(array));

    return array->strings[index];
}

char *remove_item_at(darray *array, int index) {
    assert(index >= 0 && index < size_darray(array));

    char *string = get_string_at(array, index);

    for (int i = index + 1; i < size_darray(array); i++) {
        array->strings[i - 1] = array->strings[i];
    }

    array->n--;
    free(string);
    //shift all strings by one and free the deleted string

    return string;
}

char *replace_string_at(darray *array, int index, char *string) {
    assert(index >= 0 && index < size_darray(array));

    char *old_string = get_string_at(array, index);
    strcpy(array->strings[index], string);
    return old_string;
}

void free_darray(darray *array) {
    free(array->strings);
    free(array);
}


int main() {

    char input[10];
    char *addrString1, *addrString2;
    char command;
    int addr1, addr2;
    unsigned long len;

    array = new_darray();


    while (1) {
        scanf("%s", input);

        len = strlen(input);

        command = input[len - 1]; //last char of input
        input[len - 1] = '\0'; //deleted command char from input

        if (command == 'c') { //change
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");

            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);
            //printf("%d %d %c\n", addr1, addr2, command);
            change(addr1, addr2);

        } else if (command == 'd') { //delete
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");

            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);
            printf("%d %d %c\n", addr1, addr2, command);

        } else if (command == 'p') { //print
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");

            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);
            //printf("%d %d %c\n", addr1, addr2, command);
            print();
        } else if (command == 'u') { //undo
            addr1 = atoi(input);
            printf("%d %c\n", addr1, command);
        } else if (command == 'r') { //redo
            //numbers is already a single number
            addr1 = atoi(input);
            printf("%d %c\n", addr1, command);

        } else if (command == 'q') { //quit
            //free_darray(arr);
            return 0;
        }

    }

}

void change(unsigned long addr1, unsigned long addr2) {

    int n = array->n;
    int existing_lines = n - addr1;
    int new_lines = addr2 - n;  //number of lines not yet allocated
    int input_counter = 0;
    char input_line[STRING_LENGTH];

    while (true) {

        scanf("%s", input_line);
        input_counter++;

        if (strcmp(input_line, ".") == 0)
            return;

        //if (addr1 + input_counter < array->n)
        //replace_string_at(array, addr1 + input_counter, input_line);
        //else
        append_item(array, input_line);
    }

}

void print() {
    int i = 0;
    while (i < array->n) {
        printf("%s\n", array->strings[i]);
        i++;
    }
}

