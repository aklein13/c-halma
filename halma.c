#include <X11/Xlib.h>
#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

//Sem + shared memory
#define key 30 //IPC key
#define sem1 0
#define sem2 1

int memory;
int semafors;
int tab[8][8] = {0};
int playerNumber = 0;
int (*shm)[8]; // shm = shared memory

struct sembuf Player1_lock = {sem2, -1, 0};  // Zablokuj P1
struct sembuf Player1_unlock = {sem1, 1, 0}; // Odblokuj P1
struct sembuf Player2_lock = {sem1, -1, 0};  // Zablokuj P2
struct sembuf Player2_unlock = {sem2, 1, 0}; // Odblokuj P2

//XLIB configuration
Display *mydisplay;
Window mywindow;
Window board;
int screen;
XSetWindowAttributes mywindowattributes;
XGCValues mygcvalues;
GC mygc;
GC myGcPlayer2;
Visual *myvisual;
Colormap screen_colormap;
XColor black, white, red, blue, yellow;
XEvent myevent;

//Text declaration
XFontStruct *font;
XTextItem ti[5];

//Windows:
//Main window: 1000x800
//Board window: 640x640; 1 cell = 80 px; 8x8 cells = 640px x 640px

void printTable()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            printf("%d ", tab[j][i]);
        }
        printf("\n");
    }
    printf("\n");
}

void saveToMemory()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            shm[i][j] = tab[i][j];
        }
    }
}

void readFromMemory()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            tab[i][j] = shm[i][j];
        }
    }
}

int checkIfLoaded()
{
    int flag = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (tab[i][j] == 0)
                flag++;
        }
    }
    if (flag == 64)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void drawBlock(int x, int y, GC mygc, Window board)
{
    XFillRectangle(mydisplay, board, mygc, x, y, 80, 80);
}

void drawConfirm(GC mygc, Window mywindow)
{
    XSetForeground(mydisplay, mygc, black.pixel);
    XFillRectangle(mydisplay, mywindow, mygc, 780, 180, 200, 80);
    XSetForeground(mydisplay, mygc, white.pixel);
    XFillRectangle(mydisplay, mywindow, mygc, 785, 185, 190, 70);
    XSetForeground(mydisplay, mygc, black.pixel);
    font = XLoadQueryFont(mydisplay, "7x14");
    char temp[20] = "Click to finish turn";
    ti[3].chars = temp;
    ti[3].nchars = 20;
    ti[3].delta = 0;
    ti[3].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 810, 220, ti + 3, 1);
    XUnloadFont(mydisplay, font->fid);
}

void clearConfirm(GC mygc, Window mywindow)
{
    XClearArea(mydisplay, mywindow, 780, 180, 200, 80, 0);
}

void drawPlayer(int x, int y, int playerId, GC mygc, Window board)
{
    if (playerId == 1)
    {
        XSetForeground(mydisplay, mygc, red.pixel);
    }
    else
    {
        XSetForeground(mydisplay, mygc, blue.pixel);
    }
    XFillArc(mydisplay, board, mygc, x, y, 80, 80, 0, 360 * 64);
}

void printYourName(int id)
{
    font = XLoadQueryFont(mydisplay, "7x14");
    ti[0].chars = "Player 1 figures";
    ti[0].nchars = 16;
    ti[0].delta = 0;
    ti[0].font = font->fid;
    ti[1].chars = "Player 2 figures";
    ti[1].nchars = 16;
    ti[1].delta = 0;
    ti[1].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 50, 50, ti, 1);
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 850, 50, ti + 1, 1);
    char temp[14];
    sprintf(temp, "You are Player %d", id);
    ti[4].chars = temp;
    ti[4].nchars = 16;
    ti[4].delta = 0;
    ti[4].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 850, 150, ti + 4, 1);
    XUnloadFont(mydisplay, font->fid);
}

void printPlayerName(int id)
{
    XClearArea(mydisplay, mywindow, 450, 0, 100, 100, 0);
    font = XLoadQueryFont(mydisplay, "7x14");
    char temp[14];
    sprintf(temp, "Player %d turn", id);
    ti[2].chars = temp;
    ti[2].nchars = 13;
    ti[2].delta = 0;
    ti[2].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 450, 50, ti + 2, 1);
    XUnloadFont(mydisplay, font->fid);
}

void printPlayeWon(int id)
{
    XClearArea(mydisplay, mywindow, 450, 0, 100, 100, 0);
    font = XLoadQueryFont(mydisplay, "7x14");
    char temp[14];
    sprintf(temp, "Player %d won!", id);
    ti[2].chars = temp;
    ti[2].nchars = 13;
    ti[2].delta = 0;
    ti[2].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 450, 50, ti + 2, 1);
    XUnloadFont(mydisplay, font->fid);
}

GC create_gc(Display *display, Window win, int reverse_video)
{
    GC gc;
    unsigned long valuemask = 0;
    XGCValues values;
    unsigned int line_width = 2;
    int line_style = LineSolid;
    int cap_style = CapButt;
    int join_style = JoinBevel;
    int screen_num = DefaultScreen(display);

    gc = XCreateGC(display, win, valuemask, &values);
    if (gc < 0)
    {
        printf("XCreateGC ERROR: \n");
    }

    if (reverse_video)
    {
        XSetForeground(display, gc, BlackPixel(display, screen_num) + 200);
        XSetBackground(display, gc, WhitePixel(display, screen_num));
    }
    else
    {
        XSetForeground(display, gc, BlackPixel(display, screen_num));
        XSetBackground(display, gc, WhitePixel(display, screen_num));
    }

    XSetLineAttributes(display, gc,
                       line_width, line_style, cap_style, join_style);

    XSetFillStyle(display, gc, FillSolid);
    screen_colormap = DefaultColormap(display, DefaultScreen(display));
    // P1
    XAllocNamedColor(display, screen_colormap, "red", &red, &red);
    // P2
    XAllocNamedColor(display, screen_colormap, "blue", &blue, &blue);
    // Other colors
    XAllocNamedColor(display, screen_colormap, "black", &black, &black);
    XAllocNamedColor(display, screen_colormap, "white", &white, &white);
    XAllocNamedColor(display, screen_colormap, "yellow", &yellow, &yellow);
    return gc;
}

void initGame()
{
    // Player 1
    tab[0][0] = 1;
    tab[0][1] = 1;
    tab[0][2] = 1;
    tab[0][3] = 1;
    tab[1][0] = 1;
    tab[1][1] = 1;
    tab[1][2] = 1;
    tab[2][0] = 1;
    tab[2][1] = 1;
    tab[3][0] = 1;
    // Player 2
    tab[7][7] = 2;
    tab[7][6] = 2;
    tab[7][5] = 2;
    tab[7][4] = 2;
    tab[6][7] = 2;
    tab[6][6] = 2;
    tab[6][5] = 2;
    tab[5][7] = 2;
    tab[5][6] = 2;
    tab[4][7] = 2;
}

void drawInitBoard(int selectedX, int selectedY)
{
    // 8x8 board
    int boardI = 0;
    int boardJ = 0;
    int isEven = 0;
    int isEvenJ = 0;
    int drawStart = 0;
    for (boardI = 0; boardI < 640; boardI += 80)
    {
        if (isEven)
        {
            drawStart = 80;
            XSetForeground(mydisplay, mygc, white.pixel);
            drawBlock(0, boardI, mygc, board);
        }
        else
        {
            drawStart = 0;
        }
        isEvenJ = 0;
        for (boardJ = drawStart; boardJ < 640; boardJ += 80)
        {
            if (isEvenJ)
            {
                XSetForeground(mydisplay, mygc, white.pixel);
            }
            else
            {
                XSetForeground(mydisplay, mygc, black.pixel);
            }
            drawBlock(boardJ, boardI, mygc, board);
            if (isEvenJ)
            {
                isEvenJ = 0;
            }
            else
            {
                isEvenJ = 1;
            }
        }
        if (isEven)
        {
            isEven = 0;
        }
        else
        {
            isEven = 1;
        }
    }
    if (selectedX != -1 && selectedY != -1)
    {
        XSetForeground(mydisplay, mygc, yellow.pixel);
        drawBlock(selectedX * 80, selectedY * 80, mygc, board);
    }
}

void drawPlayers()
{
    int i = 0;
    int j = 0;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (tab[i][j] == 1)
            {
                drawPlayer(i * 80, j * 80, 1, mygc, board);
            }

            else if (tab[i][j] == 2)
            {
                drawPlayer(i * 80, j * 80, 2, mygc, board);
            }
        }
    }
}

int checkIfPlayerWon(int id)
{
    if (
        id == 1 &&
        tab[7][7] == 1 &&
        tab[7][6] == 1 &&
        tab[7][5] == 1 &&
        tab[7][4] == 1 &&
        tab[6][7] == 1 &&
        tab[6][6] == 1 &&
        tab[6][5] == 1 &&
        tab[5][7] == 1 &&
        tab[5][6] == 1 &&
        tab[4][7] == 1)
    {
        return id;
    }
    if (
        id == 2 &&
        tab[0][0] == 2 &&
        tab[0][1] == 2 &&
        tab[0][2] == 2 &&
        tab[0][3] == 2 &&
        tab[1][0] == 2 &&
        tab[1][1] == 2 &&
        tab[1][2] == 2 &&
        tab[2][0] == 2 &&
        tab[2][1] == 2 &&
        tab[3][0] == 2)
    {
        return id;
    }
    return 0;
}

int playerTurn = 1;
int isOver = 0;
int selected[2] = {-1, -1};

int play()
{
    int x, y;
    int flag = 0;
    int longJump = 0;
    while (1)
    {
        if (flag == 0)
        {
            XFlush(mydisplay);
            XSync(mydisplay, False);
            drawInitBoard(-1, -1);
            drawPlayers();
            printPlayerName(playerTurn);
            flag++;
        }
        XNextEvent(mydisplay, &myevent);

        if (myevent.xany.window == board)
        {
            switch (myevent.type)
            {
            case ButtonPress:
                if (isOver)
                {
                    break;
                }
                x = myevent.xbutton.x;
                y = myevent.xbutton.y;
                int posX = x / 80;
                int posY = y / 80;
                if (selected[0] == -1 && tab[posX][posY] == playerTurn)
                {
                    selected[0] = posX;
                    selected[1] = posY;
                    drawInitBoard(posX, posY);
                    drawPlayers();
                }
                else if (selected[0] != -1 && !tab[posX][posY])
                {
                    int xDiff = abs(selected[0] - posX);
                    int yDiff = abs(selected[1] - posY);
                    // Jumps
                    if (longJump == 1 && xDiff != 2 && yDiff != 2)
                    {
                        break;
                    }
                    if ((xDiff == 2 || yDiff == 2) && tab[posX][posY] == 0)
                    {
                        int directionX = selected[0] - posX;
                        int directionY = selected[1] - posY;
                        if (directionX > 0 && directionY == 0)
                        {
                            if (!tab[posX + 1][posY])
                            {
                                break;
                            }
                        }
                        else if (directionX < 0 && directionY == 0)
                        {
                            if (!tab[posX - 1][posY])
                            {
                                break;
                            }
                        }
                        else if (directionY > 0 && directionX == 0)
                        {
                            if (!tab[posX][posY + 1])
                            {
                                break;
                            }
                        }
                        else if (directionY < 0 && directionX == 0)
                        {
                            if (!tab[posX][posY - 1])
                            {
                                break;
                            }
                        }

                        else if (directionX > 0 && directionY > 0)
                        {
                            if (!tab[posX + 1][posY + 1])
                            {
                                break;
                            }
                        }
                        else if (directionX > 0 && directionY < 0)
                        {
                            if (!tab[posX + 1][posY - 1])
                            {
                                break;
                            }
                        }
                        else if (directionX < 0 && directionY > 0)
                        {
                            if (!tab[posX - 1][posY + 1])
                            {
                                break;
                            }
                        }
                        else if (directionX < 0 && directionY < 0)
                        {
                            if (!tab[posX - 1][posY - 1])
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                        longJump = 1;
                        drawConfirm(mygc, mywindow);
                    }
                    else if (xDiff > 1 || yDiff > 1)
                    {
                        break;
                    }
                    tab[posX][posY] = tab[selected[0]][selected[1]];
                    tab[selected[0]][selected[1]] = 0;
                    if (longJump == 0)
                    {
                        drawInitBoard(-1, -1);
                    }
                    else
                    {
                        drawInitBoard(posX, posY);
                        selected[0] = posX;
                        selected[1] = posY;
                    }
                    drawPlayers();
                    isOver = checkIfPlayerWon(playerTurn);
                    if (isOver)
                    {
                        printf("Player %d won!\n", isOver);
                        printPlayeWon(isOver);
                        return 0;
                    }
                    if (longJump == 1)
                    {
                        break;
                    }
                    selected[0] = -1;
                    selected[1] = -1;
                    return 0;
                }
                else if (selected[0] != -1 && tab[posX][posY] == playerTurn && longJump == 0)
                {
                    selected[0] = posX;
                    selected[1] = posY;
                    drawInitBoard(posX, posY);
                    drawPlayers();
                }
                break;
            case KeyPress:
                //esc closes program
                if (myevent.xkey.keycode == 9 || myevent.xkey.keycode == 61)
                {
                    XCloseDisplay(mydisplay);
                    semctl(semafors, 0, IPC_RMID, 0);
                    shmdt(shm);
                    shmctl(memory, IPC_RMID, 0);
                    exit(0);
                }
                break;
            }
        }
        else
        {
            switch (myevent.type)
            {
            case ButtonPress:
                if (longJump != 1)
                {
                    break;
                }
                x = myevent.xbutton.x;
                y = myevent.xbutton.y;
                // Confirm button - 780, 180, 200, 80
                if (x >= 780 && x <= 980 && y >= 180 && y <= 260)
                {
                    clearConfirm(mygc, mywindow);
                    selected[0] = -1;
                    selected[1] = -1;
                    drawInitBoard(-1, -1);
                    drawPlayers();
                    return 0;
                }
                break;
            }
        }
    }
    selected[0] = -1;
    selected[1] = -1;
    drawInitBoard(-1, -1);
    drawPlayers();
    return 0;
}

int move()
{
    int pom;
    drawInitBoard(-1, -1);
    drawPlayers();
    printTable();
    if (playerNumber == 1)
    {
        semctl(semafors, sem1, SETVAL, 0);
        semctl(semafors, sem2, SETVAL, 1);
        playerTurn = 1;
        if (play() == 0)
        {
            saveToMemory();
            playerTurn = 2;
            selected[0] = -1;
            selected[1] = -1;
            drawInitBoard(-1, -1);
            drawPlayers();
            printPlayerName(playerTurn);
            while (1)
            {
                printf("\nWaiting for Player2 move\n");
                semop(semafors, &Player2_lock, 1);
                readFromMemory();
                playerTurn = 1;
                selected[0] = -1;
                selected[1] = -1;
                printPlayerName(playerTurn);
                if (play() == 0)
                {
                    saveToMemory();
                    playerTurn = 2;
                    printPlayerName(playerTurn);
                    drawInitBoard(-1, -1);
                    drawPlayers();
                }
                semop(semafors, &Player2_unlock, 1);
            }
        }
    }
    else
    {
        printPlayerName(2);
        while (1)
        {
            readFromMemory();
            if (checkIfLoaded() == 1)
            {
                while (1)
                {
                    printf("\nWaiting for Player1 move\n");
                    semop(semafors, &Player1_lock, 1);
                    readFromMemory();
                    playerTurn = 2;
                    selected[0] = -1;
                    selected[1] = -1;
                    printPlayerName(playerTurn);
                    if (play() == 0)
                    {
                        saveToMemory();
                        playerTurn = 1;
                        printPlayerName(playerTurn);
                        drawInitBoard(-1, -1);
                        drawPlayers();
                    }
                    semop(semafors, &Player1_unlock, 1);
                }
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    system("clear");
    //SEM
    //Create memory
    if ((memory = shmget(key, sizeof(int[10][10]), IPC_CREAT | IPC_EXCL | 0777)) == -1)
    {
        perror("shmget");
        if ((memory = shmget(key, sizeof(int[10][10]), 0)) == -1)
        {
            printf("\nError with shared memory!\n");
            perror("shmget");
            exit(1);
        }
    }
    semafors = semget(key, 2, 0777 | IPC_CREAT | IPC_EXCL);
    shm = shmat(memory, 0, 0);
    if (semafors == -1)
    {
        semafors = semget(key, 2, 0);
        playerNumber = 2;
        readFromMemory();
    }
    else
    {
        playerNumber = 1;
    }
    printf("\nYour name: Player %d", playerNumber);
    initGame();
    XInitThreads();
    mydisplay = XOpenDisplay(NULL);

    if (mydisplay == NULL)
    {
        printf("Cant open display\n");
        exit(1);
    }

    screen = DefaultScreen(mydisplay);

    //main window
    mywindow = XCreateSimpleWindow(
        mydisplay, RootWindow(mydisplay, screen),
        800, 800, 1000, 800,
        1, BlackPixel(mydisplay, screen), WhitePixel(mydisplay, screen));

    XSelectInput(mydisplay, mywindow, ExposureMask | KeyPressMask);
    XMapWindow(mydisplay, mywindow);

    //board window
    board = XCreateSimpleWindow(
        mydisplay, mywindow,
        100, 100, 640, 640,
        1, BlackPixel(mydisplay, screen), WhitePixel(mydisplay, screen));

    XSelectInput(mydisplay, board, ButtonPressMask);
    XSelectInput(mydisplay, mywindow, ButtonPressMask);
    XMapWindow(mydisplay, board);
    XMapWindow(mydisplay, mywindow);
    mygc = create_gc(mydisplay, board, 0);

    printYourName(playerNumber);
    if (playerNumber == 2)
    {
        printPlayerName(1);
    }
    move();
    return 0;
}
