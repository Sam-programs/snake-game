#include <curses.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *head_char = "#";
char *apple_char = "0";
char *lose_msg = "You lost";
char *continue_msg = "[Press Any Key To Continue]";

WINDOW *win = NULL;

int attempt = 1;

int highest = 0;

typedef enum direction {
   NONE = 0,
   UP = 1,
   DOWN = 2,
   LEFT = 4,
   RIGHT = 8
} direction;

#define OPPSITE_YDIR(dir) dir == UP ? DOWN : UP
#define BETWEEN(x, a, b)  x > a &&x < b

#define OPPSITE_XDIR(dir) dir == RIGHT ? LEFT : RIGHT

int up_keys[] = {'w', KEY_UP, 'k'};
int down_keys[] = {'s', KEY_DOWN, 'j'};
int right_keys[] = {'d', KEY_RIGHT, 'l'};
int left_keys[] = {'a', KEY_LEFT, 'h'};

typedef struct {
   int y;
   int x;
} Pos;

typedef struct {
   int  len;
   int  pos_capacity;
   Pos *positions;
   Pos  head;
} Snake;

int in(int arr[], int len, int x)
{
   for (int i = 0; i < len; i++) {
      if (arr[i] == x) {
         return TRUE;
      }
   }
   return FALSE;
}

int in_pos(Pos arr[], int len, Pos a)
{
   for (int i = 0; i < len; i++) {
      if (arr[i].x == a.x && arr[i].y == a.y) {
         return TRUE;
      }
   }
   return FALSE;
}

double distance(Pos a, Pos b)
{
   int    x = abs(a.x - b.x);
   int    y = abs(a.x - b.y);
   double res = sqrt(x * x + y * y);
   return res;
}

int move_is_safe(Snake *snake, Pos pos)
{
   if (!in_pos(snake->positions, snake->len, pos) && BETWEEN(pos.x, 0, COLS) &&
       BETWEEN(pos.y, 1, LINES)) {
      return TRUE;
   }
   return FALSE;
}

direction get_keydir(int key)
{
   if (in(up_keys, 3, key)) {
      return UP;
   }
   if (in(down_keys, 3, key)) {
      return DOWN;
   }
   if (in(left_keys, 3, key)) {
      return LEFT;
   }
   if (in(right_keys, 3, key)) {
      return RIGHT;
   }
   return NONE;
}

void randomize_apple(Snake *snake, Pos *apple)
{
   apple->y = rand() % (LINES - 1 - 2) + 2;
   apple->x = rand() % (COLS - 1 - 1) + 1;
   while (in_pos(snake->positions, snake->len, *apple)) {
      apple->y = rand() % (LINES - 1 - 2) + 2;
      apple->x = rand() % (COLS - 1 - 1) + 1;
   }
}

int snake_collide_with_itself_why_so_long(Snake *snake)
{
   for (int i = 0; i < snake->len; i++) {
      if (snake->positions[i].x == snake->head.x &&
          snake->positions[i].y == snake->head.y) {
         return TRUE;
      }
   }
   return FALSE;
}

// GOAL goto apple without touching body
direction ai_get_direction(Snake *snake, Pos *apple)
{
   Pos      *head = &snake->head;
   direction xdir = NONE;
   int       xsafe = FALSE;
   direction ydir = NONE;
   int       ysafe = FALSE;
   if (head->x < apple->x) {
      xdir = RIGHT;

      Pos next_pos = *head;
      next_pos.x++;
      if (move_is_safe(snake, next_pos)) {
         xsafe = true;
      }
   }
   if (head->x > apple->x) {
      xdir = LEFT;

      Pos next_pos = *head;
      next_pos.x--;
      if (move_is_safe(snake, next_pos)) {
         xsafe = true;
      }
   }

   if (head->y < apple->y) {
      ydir = DOWN;

      Pos next_pos = *head;
      next_pos.y++;
      if (move_is_safe(snake, next_pos)) {
         ysafe = true;
      }
   }
   if (head->y > apple->y) {
      ydir = UP;

      Pos next_pos = *head;
      next_pos.y--;
      if (move_is_safe(snake, next_pos)) {
         ysafe = true;
      }
   }
   direction dir = NONE;
   if (xsafe) {
      dir = xdir;
   } else if (ysafe) {
      dir = ydir;
   } else {
      double xdist;
      double ydist;
      Pos    next_pos = *head;
      if (xdir == NONE) {
         next_pos.x--;
         if (move_is_safe(snake, next_pos)) {
            dir = LEFT;
            xdist = distance(*apple, next_pos);
         }

         next_pos = *head;
         next_pos.x++;
         if (move_is_safe(snake, next_pos)) {
            dir = RIGHT;
            xdist = distance(*apple, next_pos);
         }
      } else {
         if (OPPSITE_XDIR(xdir) == LEFT) {
            next_pos.x--;
         } else {
            next_pos.x++;
         }
         if (move_is_safe(snake, next_pos)) {
            dir = OPPSITE_XDIR(xdir);
         }
      }
      xdist = distance(*apple, next_pos);

      next_pos = *head;
      if (OPPSITE_YDIR(ydir) == DOWN) {
         next_pos.y++;
      } else {
         next_pos.y--;
      }
      ydist = distance(*apple, next_pos);
      if (dir == NONE || ydist < xdist) {
         if (move_is_safe(snake, next_pos)) {
            dir = OPPSITE_YDIR(ydir);
         }
      }
   }
   return dir;
}

void on_lose(Snake *snake, Pos *apple)
{
   attempt++;
   snake->head.x = 0;
   snake->head.y = 0;
   snake->len = 0;
   snake->head.x = COLS / 2;
   snake->head.y = LINES / 2;
   werase(win);
}

void movepre(Snake *snake, Pos *ignore)
{
   if (snake->len == 0) {
      return;
   }
   for (int i = snake->len - 1; i > 0; i--) {
      snake->positions[i] = snake->positions[i - 1];
   }
   snake->positions[0] = snake->head;
}

void on_eat(Snake *snake, Pos *apple)
{
   snake->len++;
   if (snake->len > highest) {
      highest = snake->len;
   }
   if (snake->len > snake->pos_capacity) {
      snake->pos_capacity = snake->len * 1.5;
      snake->positions =
          realloc(snake->positions, snake->pos_capacity * sizeof(Pos));
      if (!snake->positions) {
         perror("realloc failed");
         exit(5);
      }
   }
   snake->positions[snake->len - 1] = (Pos){0};
   randomize_apple(snake, apple);
}

int main(void)
{
   win = initscr();
   noecho();
   cbreak();
   keypad(win, true);
   nodelay(win, true);
   Snake snake = {
       .len = 0,
       .pos_capacity = 1,
       .positions = malloc(sizeof(Pos)),
       .head = {LINES / 2, COLS / 2}
   };
   Pos *head = &snake.head;
   Pos  apple = {rand() % LINES, rand() % COLS};
   int  sleep_time = 60000;
   srand(time(NULL));
   while (true) {
      int key = wgetch(win);
      if (key == ' ') {
         if (sleep_time == 2000) {
            sleep_time = 60000;
         } else {
            sleep_time = 2000;
         }
      }
      direction cur_dir = ai_get_direction(&snake, &apple);
      int       old_end_y = snake.head.y;
      int       old_end_x = snake.head.x;

      if (snake.len > 0) {
         old_end_y = snake.positions[snake.len - 1].y;
         old_end_x = snake.positions[snake.len - 1].x;
      }
      movepre(&snake, &apple);
      switch (cur_dir) {
         case UP: head->y -= 1; break;
         case DOWN: head->y += 1; break;
         case LEFT: head->x -= 1; break;
         case RIGHT: head->x += 1; break;
         case NONE: break;
      }
      mvwaddstr(win, old_end_y, old_end_x, " ");
      // u can't eat while losing
      if (head->y > LINES || head->x > COLS || head->y < 0 || head->x < 0 ||
          snake_collide_with_itself_why_so_long(&snake)) {
         on_lose(&snake, &apple);
      } else if (head->x == apple.x && head->y == apple.y) {
         on_eat(&snake, &apple);
      }
      mvwaddstr(win, head->y, head->x, head_char);
      // delete the last # from the previous render
      mvwprintw(win, 0, COLS / 2, "%d", snake.len);
      mvwprintw(win, 0, COLS - 8 - 5, "attempt:%d", attempt);
      mvwprintw(win, 1, COLS - 8 - 5, "highest:%d", highest);

      mvwaddstr(win, apple.y, apple.x, apple_char);

      wmove(win, head->y, head->x);
      wrefresh(win);
      usleep(sleep_time);
   }
}
