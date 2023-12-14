#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *head_char = "#";
char *apple_char = "0";
char *lose_msg = "You lost";
char *continue_msg = "[Press Any Key To Continue]";

WINDOW *win = NULL;

typedef enum direction {
   NONE = 0,
   UP = 1,
   DOWN = 2,
   LEFT = 4,
   RIGHT = 8
} direction;

int up_keys[] = {'w', KEY_UP, 'k'};
int down_keys[] = {'s', KEY_DOWN, 'j'};
int left_keys[] = {'a', KEY_LEFT, 'h'};
int right_keys[] = {'d', KEY_RIGHT, 'l'};

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

void randomize_apple(Pos *apple)
{
   // don't spawn the apple on the edges because that's just mean
   apple->y = rand() % (LINES - 1);
   // 2 here is to be far from the score display
   apple->y = apple->y < 2 ? 2 : apple->y;

   apple->x = rand() % (COLS - 1);
   apple->x = apple->x == 0 ? 1 : apple->x;
}

int snake_collide_with_itself_why_so_long(Snake *snake)
{
   for (int i = 0; i < snake->len; i++) {
      if (snake->positions[i].x == snake->head.x &&
          snake->positions[i].y == snake->head.y) {
         return 1;
      }
   }
   return 0;
}

void on_lose(Snake *snake, Pos *apple)
{
   snake->head.x = 0;
   snake->head.y = 0;
   snake->len = 0;
   mvwaddstr(win, LINES / 2, COLS / 2 - strlen(lose_msg) / 2, lose_msg);
   mvwaddstr(
       win, LINES / 2 + 1, COLS / 2 - strlen(continue_msg) / 2, continue_msg
   );
   wrefresh(win);
   nodelay(win, false);
   int key = wgetch(win);
   nodelay(win, true);
   werase(win);
   randomize_apple(apple);
   snake->head.x = COLS / 2;
   snake->head.y = LINES / 2;
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
   randomize_apple(apple);
}

int main(void)
{
   win = initscr();
   noecho();
   cbreak();
   keypad(win, true);
   nodelay(win, true);
   direction cur_dir = NONE;
   Snake     snake = {
           .len = 0,
           .pos_capacity = 1,
           .positions = malloc(sizeof(Pos)),
           .head = {LINES / 2, COLS / 2}};
   Pos *head = &snake.head;
   Pos  apple = {rand() % LINES, rand() % COLS};
   while (true) {
      int key = wgetch(win);
      // read everything
      while (true) {
         int input = wgetch(win);
         if (input == ERR) {
            break;
         }
         key = input;
      }
      direction dir = get_keydir(key);
      if (dir != NONE) {
         cur_dir = dir;
      }
      int old_end_y = snake.head.y;
      int old_end_x = snake.head.x;

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
         cur_dir = NONE;
      } else if (head->x == apple.x && head->y == apple.y) {
         on_eat(&snake, &apple);
      }
      mvwaddstr(win, head->y, head->x, head_char);
      // delete the last # from the previous render
      mvwprintw(win, 0, COLS / 2, "%d", snake.len);
      mvwaddstr(win, apple.y, apple.x, apple_char);
      wmove(win, head->y, head->x);
      wrefresh(win);
      usleep(60000);
   }
}
