nyar - command line nyancat progress bar
----------------------------------------

## Synopsis

Perhaps you want to watch for a command to finish, but the only way in
which you can determine it's progress is by some random shell garbage
that checks on the output file.

    $ awk '{print $2}' | sort | uniq | wc -l
    856

I know that I'm expecting 1000 unique field #2s. Therefore, I should be
able to tell a program to watch for some commands' numeric output to be
close to my output. Enter nyar

    $ nyar -n 1000 -- awk '{print $2}' | sort | uniq | wc -l
    [===========================================            / 85% ]

