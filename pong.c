#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PADDLE_HEIGHT 100
#define PADDLE_WIDTH 20

#define BALL_SIZE 15
#define BALL_SLOW 0.2
#define BALL_MEDIUM 0.5
#define BALL_FAST 1

#define BALL_SHARP 0.3
#define BALL_FLAT 0.1

#define BOUNDARY_SIZE 15

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAX_SCORE 10

typedef struct {
  float x, y;
  unsigned int size;
  float dx, dy;  // Velocity
} Ball;

typedef struct {
  float x, y;
  int width, height;
  float speed;
} Paddle;

typedef struct {
  int playerScore;
  int aiScore;
} Score;

typedef struct {
  double dy, dx;
} Vector;

typedef enum { HOME, PLAYING, GAME_OVER } GameState;

static Vector calcBounceVector(double angle, double speed) {
  double theta = angle * (M_PI / 180);

  double dy = speed * sin(theta);
  double dx = speed * cos(theta);

  Vector vec = {dy, dx};
  return vec;
}

static void moveBall(Ball *ball, Paddle *paddleLeft, Paddle *paddleRight) {
  ball->x += ball->dx;
  ball->y += ball->dy;

  // Check for collisions with the boundaries
  if (ball->y < BOUNDARY_SIZE ||
      (ball->y + ball->size) > SCREEN_HEIGHT - BOUNDARY_SIZE) {
    // Top boundary collision, reverse the dy
    ball->dy = -ball->dy;
  }

  Paddle *paddles[] = {paddleLeft, paddleRight};

  for (int i = 0; i < 2; i++) {
    Paddle *paddle = paddles[i];

    // Check if ball is aligned with the paddle horizontally
    if (ball->x <= paddle->x + paddle->width &&
        ball->x + ball->size >= paddle->x) {
      // Check if ball is within the paddle's vertical bounds
      if (ball->y + ball->size >= paddle->y &&
          ball->y <= paddle->y + paddle->height) {
        // Reverse the ball's horizontal direction
        int dx_mult = ball->dx < 0 ? 1 : -1;
        // int slope_mult = (ball->dy / ball->dx) < 0 ? -1 : 1;

        // How far away the ball is from the tip of the paddle
        const float offsetPercent = (ball->y - paddle->y) / paddle->height;

        // Hit the ball at the top, set the direction upwards
        if (offsetPercent < 0.2) {
          Vector vec = calcBounceVector(35, 1);
          ball->dx = dx_mult * vec.dx;
          ball->dy = -vec.dy;
        } else if (offsetPercent >= 0.2 &&
                   offsetPercent < 0.8)  // ball in the middle
        {
          Vector vec = calcBounceVector(10, 0.5);

          if (offsetPercent < 0.5) {
            vec.dy *= -1;
          }
          ball->dx = dx_mult * vec.dx;
          ball->dy = vec.dy;
        } else  // ball at the end
        {
          Vector vec = calcBounceVector(35, 1);
          ball->dx = dx_mult * vec.dx;
          ball->dy = vec.dy;
        }

        // Adjust ball's position slightly to prevent sticking
        if (ball->dx > 0) {  // Ball moving right
          ball->x = paddle->x + paddle->width;
        } else {  // Ball moving left
          ball->x = paddle->x - ball->size;
        }
      }
    }
  }
}

static void checkPaddleBounds(Paddle *paddle) {
  if (paddle->y < BOUNDARY_SIZE) {
    paddle->y = BOUNDARY_SIZE;
  } else if (paddle->y > SCREEN_HEIGHT - BOUNDARY_SIZE - PADDLE_HEIGHT) {
    paddle->y = SCREEN_HEIGHT - BOUNDARY_SIZE - PADDLE_HEIGHT;
  }
}

static void movePaddleUp(Paddle *paddle) {
  paddle->y -= paddle->speed;
  checkPaddleBounds(paddle);
}

static void movePaddleDown(Paddle *paddle) {
  paddle->y += paddle->speed;
  checkPaddleBounds(paddle);
}

static void moveAiPaddle(Paddle *paddle, Ball *ball) {
  // Check if the ball is moving towards the AI's paddle
  if (ball->dx > 0) {
    // Try to move the paddle towards the center
    const int center = (SCREEN_HEIGHT / 2) - (paddle->height / 2);

    if (paddle->y < center) {
      movePaddleDown(paddle);
    } else {
      movePaddleUp(paddle);
    }
  }

  // Try to match the y coordinate of the ball
  if (ball->y > paddle->y) {
    movePaddleDown(paddle);
  } else if (ball->y < paddle->y) {
    movePaddleUp(paddle);
  }
}

// Updates the score
static int updateScore(Ball *ball, Score *score) {
  if (ball->x < 0) {
    // Player scored
    score->playerScore += 1;
    return 1;
  }

  else if (ball->x > SCREEN_WIDTH) {
    // AI scored
    score->aiScore += 1;
    return 1;
  }

  return 0;
}

static void initBall(Ball *ball) {
  ball->size = BALL_SIZE;
  ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
  ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;

  // Initialize serve to either player,
  // with an angle between 10 and 20
  int startAngle = (rand() % 10) + 10;
  Vector vec = calcBounceVector(startAngle, BALL_SLOW);

  // Randomize going to left or right
  if (rand() % 2 == 0) {
    vec.dx *= -1;
  }

  // Randomize going up or down
  if (rand() % 2 == 0) {
    vec.dy *= -1;
  }

  ball->dx = vec.dx;
  ball->dy = vec.dy;
}

static void initPaddle(Paddle *paddle, int isLeft) {
  const int height = 100;
  const int width = 20;
  const float speed = 0.5;

  paddle->y =
      SCREEN_HEIGHT / 2 - height / 2;  // Put in the center of the screen

  const int padding = 50;  // Padding between edge of screen and paddle
  if (isLeft) {
    paddle->x = padding;
  } else {
    paddle->x = SCREEN_WIDTH - padding;
  }

  paddle->width = width;
  paddle->height = height;
  paddle->speed = speed;
}

static void drawPaddle(SDL_Renderer *renderer, Paddle *paddle) {
  // Set the paddle color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  // Left paddle
  SDL_Rect paddleRect = {paddle->x, paddle->y, paddle->width,
                         paddle->height};  // x, y, width, height
  SDL_RenderFillRect(renderer, &paddleRect);
}

static void drawBall(SDL_Renderer *renderer, Ball *ball) {
  // Set the ball color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  // Ball
  SDL_Rect ball_rect = {ball->x, ball->y, ball->size, ball->size};
  SDL_RenderFillRect(renderer, &ball_rect);
}

// Draw boundaries at the top and bottom of the screen
static void drawBoundaries(SDL_Renderer *renderer) {
  // Set the boundary colors to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  // Top boundary
  SDL_Rect topBoundary = {0, 0, SCREEN_WIDTH, BOUNDARY_SIZE};
  SDL_RenderFillRect(renderer, &topBoundary);

  // Bottom boundary
  SDL_Rect bottomBoundary = {0, SCREEN_HEIGHT - BOUNDARY_SIZE, SCREEN_WIDTH,
                             BOUNDARY_SIZE};
  SDL_RenderFillRect(renderer, &bottomBoundary);
}

static void drawNet(SDL_Renderer *renderer) {
  const int gap = 40;  // Gap between each piece of the net

  // Set the net color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  int y = BOUNDARY_SIZE + 20;

  while (y < SCREEN_HEIGHT - BOUNDARY_SIZE) {
    SDL_Rect netRect = {SCREEN_WIDTH / 2 - BALL_SIZE, y, BALL_SIZE, BALL_SIZE};
    SDL_RenderFillRect(renderer, &netRect);
    y += gap;
  }
}

static void reset(Ball *ball, Paddle *paddleLeft, Paddle *paddelRight) {
  initBall(ball);
  initPaddle(paddleLeft, 1);
  initPaddle(paddelRight, 0);
}

typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } TextAlignment;

static void drawText(SDL_Renderer *renderer, int fontSize, int x, int y,
                     const char *text, TextAlignment alignment) {
  TTF_Font *font = TTF_OpenFont("megamax_font.ttf", fontSize);
  if (font == NULL) {
    printf("Failed to load font! SDL_ttf error: %s\n", TTF_GetError());
    TTF_Quit();
  }

  SDL_Color color = {255, 255, 255, 255};
  SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect textRect = {x, y, surface->w, surface->h};

  if (alignment == ALIGN_RIGHT) {
    textRect.x = x - surface->w;
  } else if (alignment == ALIGN_CENTER) {
    textRect.x = x - surface->w / 2;
  }

  SDL_RenderCopy(renderer, texture, NULL, &textRect);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);

  TTF_CloseFont(font);
}

typedef enum { WINNER_LEFT, WINNER_RIGHT } Winner;

static void drawWinnerScreen(SDL_Renderer *renderer, Winner winner) {
  int x;
  int y = BOUNDARY_SIZE + 75;
  if (winner == WINNER_LEFT) {
    x = SCREEN_WIDTH / 6;
  } else {
    x = SCREEN_WIDTH * 0.6;
  }

  // Draw the winner box with text
  drawText(renderer, 32, x, y, "Winner!", ALIGN_LEFT);
  drawText(renderer, 16, x, y + 48, "Press R to restart", ALIGN_LEFT);
}

int main(void) {
  // The window we'll be rendering to
  SDL_Window *window = NULL;

  // The surface contained by the window
  SDL_Renderer *renderer = NULL;

  SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "1");
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  // Initialize SDL_ttf
  if (TTF_Init() < 0) {
    printf("SDL_ttf could not initialize! SDL_ttf error: %s\n", TTF_GetError());
    SDL_Quit();
    return 1;
  }

  // Create window
  window = SDL_CreateWindow("Pong Game", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                            SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window could not be created! SDL_error: %s\n", SDL_GetError());
    return 1;
  }

  // Create renderer for window
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Main loop flag
  int quit = 0;

  // Event handler
  SDL_Event e;

  Ball ball;
  initBall(&ball);

  Paddle paddleLeft;
  Paddle paddleRight;
  initPaddle(&paddleLeft, 1);
  initPaddle(&paddleRight, 0);

  Score score = {0, 0};

  const Uint8 *keyState = SDL_GetKeyboardState(NULL);

  GameState state = HOME;

  srand(time(NULL));

  while (!quit) {
    // Handle quit events
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = 1;
      } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
          case SDLK_r:
            if (state == GAME_OVER) {
              reset(&ball, &paddleLeft, &paddleRight);
              state = PLAYING;
              score.aiScore = 0;
              score.playerScore = 0;
            }
            // No break because any key can start the game
          default:
            if (state == HOME) {
              state = PLAYING;
            }
        }
      }
    }

    if (state == PLAYING) {
      if (keyState[SDL_SCANCODE_UP]) {
        movePaddleUp(&paddleRight);
      } else if (keyState[SDL_SCANCODE_DOWN]) {
        movePaddleDown(&paddleRight);
      }

      moveBall(&ball, &paddleLeft, &paddleRight);
      moveAiPaddle(&paddleLeft, &ball);

      if (updateScore(&ball, &score)) {
        if (score.aiScore == MAX_SCORE || score.playerScore == MAX_SCORE) {
          state = GAME_OVER;
        } else {
          reset(&ball, &paddleLeft, &paddleRight);
        }
      }
    }

    /******** Draw updated state to the screen  *********/

    // Set render color to black (background)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // Clear screen
    SDL_RenderClear(renderer);

    if (state == HOME) {
      double x = SCREEN_WIDTH / 2;
      double y = SCREEN_HEIGHT / 2 - 50;
      drawText(renderer, 48, x, y, "Pong", ALIGN_CENTER);
      drawText(renderer, 24, x, y + 48, "Press any key to start", ALIGN_CENTER);
    }

    else if (state == PLAYING || state == GAME_OVER) {
      // Draw the boundaries
      drawBoundaries(renderer);

      // Draw the net
      drawNet(renderer);

      // Draw the paddles
      drawPaddle(renderer, &paddleLeft);
      drawPaddle(renderer, &paddleRight);

      if (state == PLAYING) {
        // Draw the ball
        drawBall(renderer, &ball);
      } else {
        drawWinnerScreen(
            renderer, score.aiScore == MAX_SCORE ? WINNER_LEFT : WINNER_RIGHT);
      }

      // Draw the score
      char playerScoreText[8];
      char aiScoreText[8];
      sprintf(playerScoreText, "%d", score.playerScore);
      sprintf(aiScoreText, "%d", score.aiScore);
      drawText(renderer, 48, SCREEN_WIDTH / 2 - 50, BOUNDARY_SIZE + 20,
               aiScoreText, ALIGN_RIGHT);
      drawText(renderer, 48, SCREEN_WIDTH / 2 + 50, BOUNDARY_SIZE + 20,
               playerScoreText, ALIGN_LEFT);
    }

    // Update screen
    SDL_RenderPresent(renderer);
  }

  // Destroy window
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
