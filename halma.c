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

//Kolko i krzyzyk lab 7 c wspolbiegi master

//Sem + shared memory
#define key 30 //IPC key
#define sem1 0
#define sem2 1

int memory;
int semafors;
int tab[8][8] = {0};
char *playername;
int Player1 = 1;
int Player2 = 2;
int *shm; // shm = shared memory

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
Colormap mycolormap, screen_colormap;
XColor mycolor, mycolor0, mycolor1, mycolor2, mycolor3, mycolor4, mycolor5, dummy, black, white, red, blue;
XEvent myevent;

//Text declaration
XFontStruct *font;
XTextItem ti[3];

//Windows:
//Main window: 1000x800
//Board window: 800x800; 1 cell = 80 px; 8x8 cells = 800px x 800px

void printTable()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            printf("%d ", tab[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void drawBlock(int x, int y, GC mygc, Window board)
{
    XFillRectangle(mydisplay, board, mygc, x, y, 80, 80);
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

//Place for P1 figures:
//x=100, y=150
//Place for P2 figures:
//x=850, y=150
void clearPlayer1()
{
    XClearArea(mydisplay, mywindow, 100, 150, 240, 240, 0);
}

void clearPlayer2()
{
    XClearArea(mydisplay, mywindow, 850, 150, 240, 240, 0);
}

void printPlacementError()
{
    font = XLoadQueryFont(mydisplay, "7x14");
    ti[2].chars = "You cant place that there";
    ti[2].nchars = 25;
    ti[2].delta = 0;
    ti[2].font = font->fid;
    XDrawText(mydisplay, mywindow, DefaultGC(mydisplay, screen), 500, 600, ti + 2, 1);
    XUnloadFont(mydisplay, font->fid);
}

void clearPlacementError()
{
    XClearArea(mydisplay, mywindow, 400, 500, 400, 400, 0);
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

    /* allocate foreground and background colors for this GC. */
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

    /* define the style of lines that will be drawn using this GC. */
    XSetLineAttributes(display, gc,
                       line_width, line_style, cap_style, join_style);

    /* define the fill style for the GC. to be 'solid filling'. */
    XSetFillStyle(display, gc, FillSolid);
    screen_colormap = DefaultColormap(display, DefaultScreen(display));
    // P1
    XAllocNamedColor(display, screen_colormap, "red", &red, &red);
    XAllocNamedColor(display, screen_colormap, "black", &black, &black);
    XAllocNamedColor(display, screen_colormap, "white", &white, &white);
    // P2
    XAllocNamedColor(display, screen_colormap, "blue", &blue, &blue);
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

void drawInitBoard()
{
    // 8x8 board
    int boardI = 0;
    int boardJ = 0;
    int isEven = 0;
    int drawStart = 0;
    XSetForeground(mydisplay, mygc, black.pixel);

    for (boardI = 0; boardI < 640; boardI += 80)
    {
        if (isEven)
        {
            drawStart = 80;
        }
        else
        {
            drawStart = 0;
        }
        for (boardJ = drawStart; boardJ < 640; boardJ += 160)
        {
            drawBlock(boardJ, boardI, mygc, board);
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

int main(int argc, char *argv[])
{
    system("clear");
    // int result = placeOnBoard(0,0,randBlock());
    initGame();
    printTable();
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
    XMapWindow(mydisplay, board);

    //GC for player1
    mygc = create_gc(mydisplay, board, 0);
    //GC for player2
    myGcPlayer2 = create_gc(mydisplay, board, 1);

    int x, y;
    int flag = 0;

    while (1)
    {
        if (flag < 2)
        {
            //UI idk why it needs to draw 2 times
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
            XUnloadFont(mydisplay, font->fid);

            XFlush(mydisplay);
            XSync(mydisplay, False);
            drawInitBoard();
            drawPlayers();
            flag++;
        }
        XNextEvent(mydisplay, &myevent);

        switch (myevent.type)
        {
        case ButtonPress:
            //get x and y from mouse position
            x = myevent.xbutton.x;
            y = myevent.xbutton.y;

            // drawBlock(x, y, mygc, board);
            break;
        case KeyPress:
            //esc closes program
            if (myevent.xkey.keycode == 9)
            {
                clearPlayer1();
                clearPlayer2();
                printf("esc\n");
                XCloseDisplay(mydisplay);
                exit(0);
            }
            break;
        }
    }

    return 0;
}
