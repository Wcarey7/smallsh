# smallsh
A shell written in C with a subset of features of well-known shells, such as bash.


&ndash; Provides a prompt for running commands  
&ndash; Handles blank lines and comments, which are lines beginning with the `#` character  
&ndash; Provides expansion for the variable `$$`  
&ndash; Executes 3 commands `exit`, `cd`, and `status` via code built into the shell  
&ndash; Executes other commands by creating new processes using a function from the `exec` family of functions  
&ndash; Supports input and output redirection  
&ndash; Supports running commands in foreground and background processes  
&ndash; Implements custom handlers for 2 signals, `SIGINT` and `SIGTSTP`  

# Demo



https://user-images.githubusercontent.com/13939125/190874310-fbb6b7b5-e14f-45fb-97d5-611f2eec9de9.mp4

