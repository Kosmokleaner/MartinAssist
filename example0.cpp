
// #anchor main/include

void main() {
// #anchor main/code
}

// #anchor main/functions

// ################# "add Hello World"

// #begin main/include
#include <stdio.h>
// #end main/include

// #begin main/code
void main() // #ignore
{
    printf("Hello ");
}
// #end main/code

// #begin main/code
void main() // #ignore
{
    printf("World\n");
}
// #end main/code

// ################# "add waitForReturn"

// #begin main/code
void main() // #ignore
{
    waitForReturn();
}
// #end main/code

// #begin main/functions
void waitForReturn()
{
    printf("\n\n      <RETURN>");
    while(getchar() != 13);
}
// #end main/functions


