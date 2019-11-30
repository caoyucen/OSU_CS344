#!/usr/bin/python
#*********************************************************************
# Program name:
# Author: YUCEN CAO - CS344 Fall 2019
# Date: Nov 11, 2019
# Description: Be contained in one single file, called "mypython.py". When executed, create 3 files in the same directory as your script, each named differently (the name of the files is up to you), which remain there after your script finishes executing. Each of these 3 files must contain exactly 10 random characters from the lowercase alphabet, with no spaces ("hoehdgwkdq", for example). The final (eleventh) character of each file MUST be a newline character.



import random

if __name__ == '__main__':
    for i in xrange(3):
        with open('file{}.txt'.format(i), 'w') as f: # creat the 3 files
            content = "".join([chr(random.randint(ord('a'), ord('a') + 25)) for i in xrange(10)])                      #get 10 random from a to z
            print >> f, content                      #put it into the files
            print content                            #print it into the screen
    a = random.randint(1, 42)                        #get the number from 1 to 42
    b = random.randint(1, 42)
    print a
    print b
    print a * b
