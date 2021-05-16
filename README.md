# Unix Shell

Simple Unix Shell program written for CSC 360 at the University of Victoria.

This assignment was focused on gaining experience with process creation and management. As such this shell implementation reads user commands and spawns processes to execute them, along with supporting output redirection for the result of these commands and piping of the output between commands. 

The Main method contains the core loop of the program. Here we output a prompt requesting input and poll waiting for user input. Once it is received we proceed to parse it. 
There are two special tokens for this shell, OR (output redirection) and PP (piping).
If either of these symbols appears as the first argument on an input line, the program branches to a helper function to set up the redirection. Otherwise 
