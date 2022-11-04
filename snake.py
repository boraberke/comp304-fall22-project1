from enum import Enum 
import random
import queue
from pynput import keyboard
import time
import os
import sys
import argparse


class Squares(Enum):
    """Squares to fill the game state."""
    Wall = '#'
    Snake = '@'
    Empty = ' '
    Food = '*'


# define clear function
def clear():
    # check and make call for specific operating system
    os.system('clear')

class GameState:
    def __init__(self,width,height,snake_length,snake_facing,speed):
        self.width = width
        self.height = height
        self.state = self._state_with_borders()
        self.snake_facing = snake_facing
        self.snake_positions = self._initialize_snake_pos(snake_length)
        self.is_end = False
        self.add_random_food()
        self.score = 0
        self.speed = speed
        self.help = "[W]->NORTH [A]->WEST [S]->SOUTH [D]->EAST"
    
    def add_random_food(self):
        empty_squares = self.get_empty_positions()
        pos = random.choice(empty_squares)
        self.update_state(pos,Squares.Food)
    def _initialize_snake_pos(self,snake_length):
        snake_positions = queue.deque()
        center_x,center_y = self.get_center_pos()
        # depending on where the snake is facing, enlarge the snake in the opposite way
        for i in range(snake_length):
            if (self.snake_facing == 'NORTH'):
                pos = (center_x,center_y+i) 
            elif(self.snake_facing == 'SOUTH'):
                pos =  (center_x,center_y-i) 
            elif(self.snake_facing == 'EAST'):
                pos = (center_x-i,center_y) 
            elif(self.snake_facing == 'WEST'):
                pos = (center_x+i,center_y) 
            # check if the position is a legal position (i.e. not a wall)
            if(self.is_legal(pos)):
                snake_positions.appendleft(pos)
                self.state[pos[1]][pos[0]] = Squares.Snake
            else:
                raise Exception('Illegal position of snake: {}'.format(pos))
        return snake_positions

    def get_center_pos(self):
        return (self.width//2,self.height//2)
        
    def _state_with_borders(self):
        state = [[Squares.Empty for i in range(self.width)] for i in range(self.height)]
        for i in range(self.width):
            state[0][i] = Squares.Wall
            state[self.height-1][i] = Squares.Wall
        for j in range(self.height):
            state[j][0] = Squares.Wall
            state[j][self.width-1] = Squares.Wall
        return state

    def print_state(self):
        for row in self.state:
            for pos in row:
                print(pos.value,end=" ")
            print()
        print(self.help)
        time.sleep(self.speed)
        clear()


    def get_empty_positions(self):
        empty_positions = []
        for y in range(len(self.state)):
            for x in range(len(self.state[y])):
                if (self.state[y][x] == Squares.Empty):
                    empty_positions.append((x,y))
        return empty_positions

    def is_legal(self,pos):
        x,y = pos
        # tile is empty
        if (self.state[y][x] != Squares.Wall and self.state[y][x] != Squares.Snake):
            return True
        else:
            return False
    def is_ended(self):
        return self.is_end
    
    def apply_action(self, action):
        '''
        Change the direction that snake is facing.
        '''
        if (self.snake_facing =='NORTH' or self.snake_facing =='SOUTH'):
            if (action=='WEST' or action =='EAST'):
                self.snake_facing = action
        else:
            if (action=='NORTH' or action=='SOUTH'):
                self.snake_facing = action
    def next_snake_pos(self,head_pos):
        x = head_pos[0]
        y = head_pos[1]
        if (self.snake_facing == 'NORTH'):
            return (x,y-1) 
        elif(self.snake_facing == 'SOUTH'):
            return (x,y+1) 
        elif(self.snake_facing == 'EAST'):
            return (x+1,y) 
        elif(self.snake_facing == 'WEST'):
            return (x-1,y) 
    def move_snake(self):
        '''
        move the snake and update the game state
        '''
        # get the head of the snake
        head = self.snake_positions[-1]
        next_pos = self.next_snake_pos(head)
        #move to the new location
        self.snake_positions.append(next_pos)
        # if there is a food in the next position, we let snake to grow by not shrinking from tail
        if (not self.is_food(next_pos)):
            #remove the tail
            tail = self.snake_positions.popleft()
            self.update_state(tail,Squares.Empty)
        else:
            self.add_random_food()
            self.score += 5
        # update the state if it is a legal move
        if (self.is_legal(next_pos)):
            self.update_state(next_pos,Squares.Snake)
    
    def check_end_game(self):
        '''
        check the following two conditions:
            snake eat itself: there are duplicates in the deque
            snake hits to wall: head of the snake hits to a wall
        '''
        # get the head of the snake
        head = self.snake_positions[-1]
        if (self.get_pos(head) == Squares.Wall):
            self.is_end = True
        else:
            count = 0
            for pos in self.snake_positions:
                if (pos == head):
                    count=count+1
            if (count==2):
                self.is_end = True
        if (self.is_end):
            print("Game over. Score: {}".format(self.score))
            time.sleep(2)
            sys.exit(1)

    def get_pos(self,pos):
        return self.state[pos[1]][pos[0]]

    def is_food(self,pos):
        if (self.state[pos[1]][pos[0]] == Squares.Food):
            return True
        else:
            return False

    def update_state(self,pos,square_type):
        self.state[pos[1]][pos[0]] = square_type
        

class SnakeGame:
    def __init__(self,speed,frame_size):
        self.speed = 0.5/speed
        self.frame_width_height = 15
        if frame_size=="small":
            self.frame_width_height = 15
        elif frame_size=="medium":
            self.frame_width_height = 20
        elif frame_size=="large":
            self.frame_width_height = 30

        self.state = GameState(self.frame_width_height,self.frame_width_height,5,'NORTH',self.speed)
        self.player = 'Human'
        self.actionQueue = queue.Queue()
        self.listener = None
    
    def check_keystrokes(self):
        if self.listener == None:
            self.listener = keyboard.Listener(on_press = self.add_actions)
            self.listener.start()
    def add_actions(self,key):
        try: 
            key = key.char
        except:
            key = key.name
        if (key == 'w'):
            self.actionQueue.put('NORTH')
        elif (key == 'a'):
            self.actionQueue.put('WEST')
        elif (key == 's'):
            self.actionQueue.put('SOUTH')
        elif (key == 'd'):
            self.actionQueue.put('EAST')
    
    def run(self):
        # clear the screen first
        clear()
        self.check_keystrokes()
        while (not self.state.is_ended()):
            if (not self.actionQueue.empty()):
                action = self.actionQueue.get()
                self.state.apply_action(action)
            self.state.move_snake()
            self.state.check_end_game()
            self.state.print_state()


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='A simple and fun classic snake game!')
    parser.add_argument("--speed", default=3, type=float, choices=range(1,11),help="the speed of the game. Between 1 and 10")
    parser.add_argument("--frame", default="medium", type=str, choices=["small","medium","large"],help="Size of the frame. small, medium, or big.")

    args = parser.parse_args()
    snake = SnakeGame(args.speed,args.frame)
    snake.run()






