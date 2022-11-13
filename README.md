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

## Background Processes

Use the following format to run any process in the background in the shell.

```
<command name> <args> &
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

Give an integer specifying how often a quote is voice overed in terms of minutes.

```
wiseman 5
```

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

This command has two inputs: the first one is an integer indicating which of the three dances you want to be displayed and the second one is an integer indicating how many times you want the animation to be played.

```
dance 1 3
```
![dance1](https://user-images.githubusercontent.com/79407881/201546613-ecadf4ae-9e7e-4c8a-a21c-8602c1f6cd6a.gif)

```
dance 2 3
```
![dance2](https://user-images.githubusercontent.com/79407881/201546625-a1b716c9-efd1-454d-8280-de52b3f5c8d0.gif)

```
dance 3 3
```
![dance3](https://user-images.githubusercontent.com/79407881/201546639-13492039-6f4c-448f-a6d4-681d67b89494.gif)

### Psvis Command

Enter a PID of a process and an image output name for the child processes of that process as a tree graph, eldest children of each process colored red.

```
psvis 1 image
```
