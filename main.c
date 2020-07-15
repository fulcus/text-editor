#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main() {

    char input[10];
    char *addrString1, *addrString2;
    char command;
    int addr1, addr2;
    unsigned long len;

    while (1) {
        scanf("%s", input);

        len = strlen(input);

        command = input[len - 1]; //last char of input
        input[len - 1] = '\0'; //deleted command char from input

        //printf("%c %s", command, input);

        //every command:
        //(address1,address2)c
        //(address1,address2)d
        //(address1,address2)p
        //(number)u
        //(number)r
        //q


        if (command == 'c') { //change
            addrString1 = strtok(input, ",");
            addrString2 = strtok(NULL, "");

            addr1 = atoi(addrString1);
            addr2 = atoi(addrString2);
            printf("%d %d %c\n", addr1, addr2, command);
            //change(addr1,addr2);

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
            printf("%d %d %c\n", addr1, addr2, command);

        } else if (command == 'u') { //undo
            addr1 = atoi(input);
            printf("%d %c\n", addr1, command);
        } else if (command == 'r') { //redo
            //numbers is already a single number
            addr1 = atoi(input);
            printf("%d %c\n", addr1, command);

        } else if (command == 'q') { //quit
            return 0;
        }

    }

}
