# COMP304 - Fall 2022 - Project 1

This is the first project of the COMP304 course (Fall '22) in Koç University.

## Contributors
Bora Berke Şahin\
Büşra Işık

## Compiling

Use the following to compile and run the shell.

```bash
gcc shellax.c -o shellax
./shellax
```

## Function Usages


### Uniq Command

Given sorted lines from a pipe
```
cat input.txt | uniq 
```

Given sorted lines using redirector
```
uniq <input.txt
```

Include count of unique elements
```
uniq --count <input.txt 
```

### Chatroom Command

Enter to a room `best_room` with username `bora`
```
chatroom best_room bora
```

Enter to the same `best_room` with username `busra`

```
chatroom best_room busra
```

Then start chatting!

### Wiseman Command


### Bora's Custom Command `snake`
```
usage: snake [-h] [--speed {1,2,3,4,5,6,7,8,9,10}]
             [--frame {small,medium,large}]

A simple and fun classic snake game!

optional arguments:
  -h, --help            show this help message and exit
  --speed {1,2,3,4,5,6,7,8,9,10}
                        the speed of the game. Between 1 and 10
  --frame {small,medium,large}
                        Size of the frame. small, medium, or big.
```
![](snake_bora.gif)

### Busra's Custom Command `dance`

### Psvis Command
