# Flareon 5 magic
This solution uses the linux ptrace.
It bruteforces the answers for the first permutation (which is up to 3 consecutive characters, 95 ^ 3 for each substring)
The next flags are calculated using the hashes that were calculated at the first permutation

A bit more technical:  
The debugger places few breakpoints (apart from temorary ones), one at the start of the function that check the current key, one on the sequence-checking function, and one at each brench of the possible outcomes (current sequence is wrong or right).  
The breakpoint at the start used to save register states (as it will "load" the state on failure).  
The breakpoing at the checking function is to read the length of the sequence and it's index.  
The two others would be called by the result of the sequence-checking function.
If the sequence was calculated as the wrong sequence, the guess would increment and revert back to the known good state that was saved at the start of the function (and after each successful sequence), otherwise we go to the next sequencee, no revert is required.
By the end of the furst permutation, the results are saved and just re-orgenized, until all permutation are done.

NOTE: if 2 substring will get the same "hash" (result from a function), this solution will NOT work!
Make sure that `./magic` is executable.

After the run ends, you should filter the output and supply it to the ORIGINAL file
It is automated using the `run.sh`.

Extra points:
- The code has no classes and the only reason I used C++ over C was std::map.
- Although C++ offer `std::cout`, `printf` is much better for printing string & data mixed. Should have used it
- I could have redirected the output of the program to a file instead of re-running it with the correct output. Looking back, I should have done that, that's much more elegant.


debugger.h (getdata and putdata functions) was based on the code found here:
https://theantway.com/2013/01/notes-for-playing-with-ptrace-on-64-bits-ubuntu-12-10/


~~Also, forgive me for the intensive output, it was a nightmare to debug~~

