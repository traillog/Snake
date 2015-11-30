//
//  Snake.c
//

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Timer config
#define     ID_TIMER        1       // Move-timer
#define     DELAY_TIMER     300     // Timer interval [ms]

// Directions
#define     UP              0
#define     RIGHT           1
#define     DOWN            2
#define     LEFT            3

// Board config
#define     BRD_SIZE_PX     500     // Board size in pixels  (device units)
#define     BRD_SIZE_SQ     20      // Board size in squares (logical units)

// Snake config
#define     GROWTH_INC          4   // Amount new links added per apple
#define     SNAKE_INI_SIZE      5   // Initial length (links)
#define     SNAKE_MAX_SIZE      25  // Max length (links)
#define     SNAKE_TAIL          0                       // Tail's index
#define     SNAKE_INI_HEAD  ( SNAKE_INI_SIZE - 1 )      // Ini heads's index

// Type of obstacles
#define     WALL            1
#define     BODY            2

// 'Win Proc'
LRESULT CALLBACK WndProc( HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam );

// Auxiliary functions
void initLevel( HWND hwnd,
    int *currentDirPt, BOOL *growingPt,
    BOOL *collisionPt, BOOL *pauseOnPt,
    BOOL *aplOnBrd, POINT *aplPos,
    POINT *snkBdy, int *snkHead,
    char ( *brd )[ BRD_SIZE_SQ ],
    char ( *walls )[ BRD_SIZE_SQ ] );

void moveSnake( HWND hwnd,
    int cX, int cY,
    int *currentDirPt,
    BOOL *growingPt, BOOL *collisionPt,
    POINT *aplPos, BOOL *aplOnBrd,
    POINT *snkBdy, int *snkHead,
    char ( *brd )[ BRD_SIZE_SQ ],
    HPEN tailPen, HBRUSH tailBrush,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol,
    HPEN aplPen, HBRUSH aplBrush,
    HPEN gridPen, HBRUSH gridBrush );

BOOL putAppleOnBrd( POINT *aplPos, char ( *brd )[ BRD_SIZE_SQ ] );

void setUpMappingMode( HDC hdc, int cX, int cY );

void drawGrid( HDC hdc, HPEN gridPen, HBRUSH gridBrush );

void drawLevel( HDC hdc, char ( *brd )[ BRD_SIZE_SQ ] );

void drawHead( HDC hdc,
    BOOL *collisionPt,
    POINT *snkBdy, int *snkHead,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol );

void drawNeck( HDC hdc, POINT *snkBdy, int *snkHead );

void drawBody( HDC hdc, POINT *snkBdy, int *snkHead );

void drawTail( HDC hdc, POINT *snkBdy, HPEN tailPen, HBRUSH tailBrush );

void deleteTail( HDC hdc, POINT *snkBdy, POINT *oldTailPos,
    HPEN gridPen, HBRUSH gridBrush );

void drawApple( HDC hdc, POINT *aplPos, HPEN aplPen, HBRUSH aplBrush );


int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR szCmdLine, int nCmdShow )
{
    PTCHAR szAppName = TEXT( "Snake" );

    HWND hwnd;
    MSG msg;
    BOOL bRet;
    WNDCLASS wc;

    //=========================================================================
    // Register main window class
    //=========================================================================
    memset( &wc, 0, sizeof( WNDCLASS ) );
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = ( HBRUSH )GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAppName;

    if ( !RegisterClass( &wc ) )
    {
        MessageBox( NULL,
            TEXT( "Window Registration Failed!" ),
            szAppName,
            MB_ICONERROR | MB_OK );

        return 0;
    }

    //=========================================================================
    // Create main window
    //=========================================================================
    hwnd = CreateWindow(
        szAppName,                          // Window class name
        szAppName,                          // Window title
        WS_OVERLAPPEDWINDOW,                // Window style
        CW_USEDEFAULT,                      // Ini H pos
        CW_USEDEFAULT,                      // Ini V pos
        BRD_SIZE_PX + 300,                  // Window width
        BRD_SIZE_PX + 200,                  // Window height
        NULL,                               // Parent Window
        NULL,                               // Menu
        hInstance,                          // Application instance handle
        NULL );                             // Additional creation data.

    if ( hwnd == NULL )
    {
        MessageBox( NULL,
            TEXT( "Window Creation Failed!" ),
            szAppName,
            MB_ICONERROR | MB_OK );

        return 0;
    }

    ShowWindow( hwnd, nCmdShow );       // Display the main window
    UpdateWindow( hwnd );               // Send first WM_PAINT message

    //=========================================================================
    // Message Loop
    //=========================================================================

    while( ( bRet = GetMessage( &msg, NULL, 0, 0 ) ) != 0 )
    { 
        if ( bRet == -1 )
        {
            // GetMessage error
            MessageBox( NULL,
                TEXT( "GetMessage error!" ),
                szAppName,
                MB_ICONERROR | MB_OK );

            return 0;
        }
        else
        {
            TranslateMessage( &msg ); 
            DispatchMessage( &msg ); 
        }
    }

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static int i;
    static int  cxClient, cyClient;
    
    // Snake's body
    static POINT snakeBdy[ SNAKE_MAX_SIZE ] = { 0 };
    static int snakeHead = SNAKE_INI_HEAD;

    // Snake's status
    static int currentDir   = UP;
    static BOOL growing     = FALSE;
    static BOOL collision   = FALSE;
    static BOOL pauseOn     = FALSE;

    // Apples
    static BOOL appleOnBoard = FALSE;
    static POINT applePos;
    
    // Colors
    static HPEN redPen, greenPen, brownPen, purplePen, lightGreyPen;
    static HBRUSH redBrush, greenBrush, brownBrush, purpleBrush, lightGreyBrush;
    
    // Board
    static char board[ BRD_SIZE_SQ ][ BRD_SIZE_SQ ] = { 0 };

    // Walls level 1
    static char walls01[ BRD_SIZE_SQ ][ BRD_SIZE_SQ ] =
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    switch( message )
    {
    case WM_CREATE :
        // Initialise random seed
        srand( ( unsigned int )time( NULL ) );

        // Init pens
        brownPen     = CreatePen( PS_SOLID, 0, RGB( 0xA0, 0x5A, 0x2C ) );
        redPen       = CreatePen( PS_SOLID, 0, RGB( 0xFF, 0x00, 0x00 ) );
        greenPen     = CreatePen( PS_SOLID, 0, RGB( 0x55, 0xD4, 0x00 ) );
        purplePen    = CreatePen( PS_SOLID, 0, RGB( 0xD4, 0x2A, 0xFF ) );
        lightGreyPen = CreatePen( PS_SOLID, 0, RGB( 0x00, 0x00, 0x00 ) );
        
        // Init brushes
        brownBrush     = CreateSolidBrush( RGB( 0xA0, 0x5A, 0x2C ) );
        redBrush       = CreateSolidBrush( RGB( 0xFF, 0x00, 0x00 ) );
        greenBrush     = CreateSolidBrush( RGB( 0x55, 0xD4, 0x00 ) );
        purpleBrush    = CreateSolidBrush( RGB( 0xD4, 0x2A, 0xFF ) );
        lightGreyBrush = CreateSolidBrush( RGB( 0x00, 0x00, 0x00 ) );

        // Init level status
        initLevel( hwnd,
            &currentDir, &growing,
            &collision, &pauseOn,
            &appleOnBoard, &applePos,
            snakeBdy, &snakeHead,
            board, walls01 );
        return 0;

    case WM_SIZE :
        // Get size of client area
        cxClient = LOWORD( lParam );
        cyClient = HIWORD( lParam );
        return 0;

    case WM_TIMER :
        // Move snake one square in current dir
        moveSnake( hwnd,
            cxClient, cyClient,
            &currentDir,
            &growing, &collision,
            &applePos, &appleOnBoard,
            snakeBdy, &snakeHead, board,
            brownPen, brownBrush,
            redPen, redBrush,
            greenPen, greenBrush,
            purplePen, purpleBrush,
            lightGreyPen, lightGreyBrush );

        // Stop timer if in collision state
        if ( collision )
            KillTimer( hwnd, ID_TIMER );

        return 0 ;

    case WM_KEYDOWN :
        switch ( wParam )
        {
        case VK_LEFT :
            // Block movement if in collision state          
            if ( collision || pauseOn )
                return 0;

            // Turn left
            KillTimer( hwnd, ID_TIMER );        // Stop timer
            currentDir--;                       // Update dir
            break;

        case VK_RIGHT :
            // Block movement if in collision state
            if ( collision || pauseOn )
                return 0;

            // Turn right
            KillTimer( hwnd, ID_TIMER );        // Stop timer
            currentDir++;                       // Update dir
            break;

        case 0x50 :           // Virtual key code for 'P'
            // If in collision state, do nothing
            if ( collision )
                return 0;

            // Pause game
            if ( pauseOn == FALSE )
            {
                // Stop the timer
                KillTimer( hwnd, ID_TIMER );
                pauseOn = TRUE;
            }
            else
            {
                // Re-start the timer
                SetTimer( hwnd, ID_TIMER, DELAY_TIMER, NULL );
                pauseOn = FALSE;
            }
            return 0;

        case 0x4E :           // Virtual key code for 'N'
            // Reset level status
            initLevel( hwnd,
                &currentDir, &growing,
                &collision, &pauseOn,
                &appleOnBoard, &applePos,
                snakeBdy, &snakeHead,
                board, walls01 );
            return 0;

        case VK_ESCAPE :
            // Close the application
            DestroyWindow( hwnd );
            return 0;

        default :
            return 0;
        }

        // Validate direction
        if ( currentDir > 3 )
            currentDir %= 4;
        
        if ( currentDir < 0 )
            currentDir += 4;

        // Move snake one square in current dir
        moveSnake( hwnd,
            cxClient, cyClient,
            &currentDir,
            &growing, &collision,
            &applePos, &appleOnBoard,
            snakeBdy, &snakeHead, board,
            brownPen, brownBrush,
            redPen, redBrush,
            greenPen, greenBrush,
            purplePen, purpleBrush,
            lightGreyPen, lightGreyBrush );

        // Re-start move-timer, if not in collision state
        if ( !collision )
            SetTimer( hwnd, ID_TIMER, DELAY_TIMER, NULL );

        return 0;

    case WM_PAINT :
        hdc = BeginPaint( hwnd, &ps );

        setUpMappingMode( hdc, cxClient, cyClient );

        drawGrid( hdc, lightGreyPen, lightGreyBrush );

        drawLevel( hdc, board );

        drawBody( hdc, snakeBdy, &snakeHead );

        drawTail( hdc, snakeBdy, brownPen, brownBrush );

        drawHead( hdc, &collision, snakeBdy, &snakeHead, 
            redPen, redBrush, greenPen, greenBrush );

        if ( appleOnBoard )
            drawApple( hdc, &applePos, purplePen, purpleBrush );

        EndPaint( hwnd, &ps );

        return 0 ;

    case WM_DESTROY :
        // Stop move-timer
        KillTimer( hwnd, ID_TIMER );

        // Delete pens
        DeleteObject( redPen );
        DeleteObject( greenPen );
        DeleteObject( brownPen );
        DeleteObject( purplePen );
        DeleteObject( lightGreyPen );

        // Delete brushes
        DeleteObject( redBrush );
        DeleteObject( greenBrush );
        DeleteObject( brownBrush );
        DeleteObject( purpleBrush );
        DeleteObject( lightGreyBrush );

        // Quit application
        PostQuitMessage( 0 );
        return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}

void setUpMappingMode( HDC hdc, int cX, int cY )
{
    // Set up mapping mode
    SetMapMode( hdc, MM_ISOTROPIC );

    // Set up extents
    SetWindowExtEx( hdc, BRD_SIZE_SQ, BRD_SIZE_SQ, NULL );
    SetViewportExtEx( hdc, BRD_SIZE_PX, BRD_SIZE_PX, NULL );

    // Set up 'viewport' origin
    SetViewportOrgEx( hdc,
        cX / 2 - ( BRD_SIZE_PX / 2 ),
        cY / 2 - ( BRD_SIZE_PX / 2 ),
        NULL );
}

void initLevel( HWND hwnd,
    int *currentDirPt, BOOL *growingPt,
    BOOL *collisionPt, BOOL *pauseOnPt,
    BOOL *aplOnBrd, POINT *aplPos,
    POINT *snkBdy, int *snkHead,
    char ( *brd )[ BRD_SIZE_SQ ],
    char ( *walls )[ BRD_SIZE_SQ ] )
{
    int i;          // For-loop counter

    // Stop move-timer
    KillTimer( hwnd, ID_TIMER );

    // Set dir to UP
    *currentDirPt = UP;

    // Reset flags
    *growingPt = FALSE;
    *collisionPt = FALSE;
    *pauseOnPt = FALSE;

    // Reset board --> Load level obstacles
    memcpy( brd, walls, BRD_SIZE_SQ * BRD_SIZE_SQ * sizeof( char ) );

    // Reset snake's size
    *snkHead = SNAKE_INI_HEAD;

    // Reset snake's position
    memset( snkBdy, 0, SNAKE_MAX_SIZE * sizeof( POINT ) );

    for ( i = SNAKE_TAIL; i <= *snkHead; i++ )
    {
        snkBdy[ i ].x = BRD_SIZE_SQ / 2 - 1;
        snkBdy[ i ].y = BRD_SIZE_SQ - 2 - i;

        // Block current sanke position on board
        brd[ snkBdy[ i ].y ][ snkBdy[ i ].x ] = BODY;
    }

    // Put apple on board
    *aplOnBrd = putAppleOnBrd( aplPos, brd );

    // Re-start move-timer
    SetTimer( hwnd, ID_TIMER, DELAY_TIMER, NULL );

    // Trigger a re-paint
    InvalidateRect( hwnd, NULL, TRUE );
}

void moveSnake( HWND hwnd,
    int cX, int cY,
    int *currentDirPt,
    BOOL *growingPt, BOOL *collisionPt,
    POINT *aplPos, BOOL *aplOnBrd,
    POINT *snkBdy, int *snkHead,
    char ( *brd )[ BRD_SIZE_SQ ],
    HPEN tailPen, HBRUSH tailBrush,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol,
    HPEN aplPen, HBRUSH aplBrush,
    HPEN gridPen, HBRUSH gridBrush )
{
    static HDC hdc;
    static unsigned int oldHead;
    static POINT newHeadPos;
    static POINT oldTail;
    static int linksToGrowth = 0;

    //==================================================
    // Update head's position
    //==================================================
    // Save original head's pos
    oldHead = *snkHead;

    // Init new head pos
    newHeadPos = snkBdy[ *snkHead ];

    switch( *currentDirPt )
    {
    case UP :
        newHeadPos.y--;
        break;

    case DOWN :
        newHeadPos.y++;
        break;

    case RIGHT :
        newHeadPos.x++;
        break;

    case LEFT :
        newHeadPos.x--;
        break;

    default :
        break;
    }

    // Validate new head's pos
    if ( newHeadPos.x > ( BRD_SIZE_SQ - 1 ) )
        newHeadPos.x %= BRD_SIZE_SQ;

    if ( newHeadPos.x < 0 )
        newHeadPos.x += BRD_SIZE_SQ;

    if ( newHeadPos.y > ( BRD_SIZE_SQ - 1 ) )
        newHeadPos.y %= BRD_SIZE_SQ;

    if ( newHeadPos.y < 0 )
        newHeadPos.y += BRD_SIZE_SQ;

    //==================================================
    // Detect collisions
    //==================================================
    // Walls == 1
    // Body  == 2
    if ( brd[ newHeadPos.y ][ newHeadPos.x ] )
        *collisionPt = TRUE;
    else
        *collisionPt = FALSE;

    // Block new head pos on board
    if ( !( *collisionPt ) )
        brd[ newHeadPos.y ][ newHeadPos.x ] = BODY;

    // Check for apple at new head pos
    if ( ( *aplOnBrd ) &&
         ( newHeadPos.x == aplPos->x ) &&
         ( newHeadPos.y == aplPos->y ) )
    {
        // Apple founded

        // Snake has eaten an apple --> it starts to grow
        *growingPt = TRUE;

        // Inc links to growth, if max length not reached
        linksToGrowth += GROWTH_INC;

        // Current apple has been eaten and no max length reached
        if ( *snkHead + linksToGrowth < SNAKE_MAX_SIZE - 1 )
            *aplOnBrd = putAppleOnBrd( aplPos, brd );
        else
            *aplOnBrd = FALSE;
    }
    else
    {
        // No apple founded

        // If max length reached --> stop growing
        if ( *snkHead >= SNAKE_MAX_SIZE - 1 )
            linksToGrowth = 0;

        // Check for growth status
        if ( linksToGrowth == 0 ) 
            *growingPt = FALSE;         // Growth finished
    }

    //==================================================
    // Move snake's body ---> only if no apple was eaten
    //==================================================
    if ( !( *growingPt ) )
    {
        oldTail = snkBdy[ SNAKE_TAIL ];

        // Free tail's original pos on board
        brd[ oldTail.y ][ oldTail.x ] = 0;

        // Shift position of all links towards the tail
        // Tail's original pos is overwritten
        memmove( snkBdy, snkBdy + 1, oldHead * sizeof( POINT ) );

        // Save new head pos to snake's head
        snkBdy[ *snkHead ] = newHeadPos;
    }
    else
    {
        // Increment snake's length
        ( *snkHead )++;

        // Save new head pos to snake's head
        snkBdy[ *snkHead ] = newHeadPos;

        // Dec links to growth
        linksToGrowth--;
    }

    //==================================================
    // Update snake's plot and apple'plot
    //==================================================
    hdc = GetDC( hwnd );

    setUpMappingMode( hdc, cX, cY );

    deleteTail( hdc, snkBdy, &oldTail, gridPen, gridBrush );

    drawTail( hdc, snkBdy, tailPen, tailBrush );

    drawNeck( hdc, snkBdy, snkHead );

    drawHead( hdc, collisionPt, snkBdy, snkHead, 
        headPenCol, headBrushCol, headPenNoCol, headBrushNoCol );

    if ( *aplOnBrd )
            drawApple( hdc, aplPos, aplPen, aplBrush );

    ReleaseDC( hwnd, hdc );
}

void drawGrid( HDC hdc, HPEN gridPen, HBRUSH gridBrush )
{
    int i;

    SelectObject( hdc, gridPen );
    SelectObject( hdc, gridBrush );

    for ( i = 0; i <= BRD_SIZE_SQ; i++ )
    {
        MoveToEx( hdc, i, BRD_SIZE_SQ, NULL );
        LineTo( hdc, i, 0 );
    }

    for ( i = 0; i <= BRD_SIZE_SQ; i++ )
    {
        MoveToEx( hdc, 0, i, NULL );
        LineTo( hdc, BRD_SIZE_SQ, i );
    }
}

void drawLevel( HDC hdc, char ( *brd )[ BRD_SIZE_SQ ] )
{
    int i, j;

    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    SelectObject( hdc, GetStockObject( BLACK_BRUSH ) );

    for ( i = 0; i < BRD_SIZE_SQ; i++ )
    {
        for ( j = 0; j < BRD_SIZE_SQ; j++ )
        {
            if ( brd[ i ][ j ] == 1 )
                Rectangle( hdc, j, i, j + 1, i + 1 );
        }
    }

}

void drawHead( HDC hdc,
    BOOL *collisionPt,
    POINT *snkBdy, int *snkHead,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol )
{
    if ( *collisionPt )
    {
        SelectObject( hdc, headPenCol );
        SelectObject( hdc, headBrushCol );
    }
    else
    {
        SelectObject( hdc, headPenNoCol );
        SelectObject( hdc, headBrushNoCol );
    }

    Ellipse( hdc,
        snkBdy[ *snkHead ].x    , snkBdy[ *snkHead ].y,
        snkBdy[ *snkHead ].x + 1, snkBdy[ *snkHead ].y + 1 );
}

// Draw only first link after the head
void drawNeck( HDC hdc, POINT *snkBdy, int *snkHead )
{
    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    SelectObject( hdc, GetStockObject( BLACK_BRUSH ) );

    Ellipse( hdc,
        snkBdy[ *snkHead - 1 ].x    , snkBdy[ *snkHead - 1 ].y,
        snkBdy[ *snkHead - 1 ].x + 1, snkBdy[ *snkHead - 1 ].y + 1 );
}

// Draw snake's body without head nor tail
void drawBody( HDC hdc, POINT *snkBdy, int *snkHead )
{
    int i;

    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    SelectObject( hdc, GetStockObject( BLACK_BRUSH ) );
    
    for ( i = SNAKE_TAIL + 1; i < *snkHead; i++ )
        Ellipse( hdc,
            snkBdy[ i ].x    , snkBdy[ i ].y,
            snkBdy[ i ].x + 1, snkBdy[ i ].y + 1 );
}

void drawTail( HDC hdc, POINT *snkBdy, HPEN tailPen, HBRUSH tailBrush )
{
    SelectObject( hdc, tailPen );
    SelectObject( hdc, tailBrush );

    Ellipse( hdc,
        snkBdy[ SNAKE_TAIL ].x    , snkBdy[ SNAKE_TAIL ].y,
        snkBdy[ SNAKE_TAIL ].x + 1, snkBdy[ SNAKE_TAIL ].y + 1 );
}

void deleteTail( HDC hdc, POINT *snkBdy, POINT *oldTailPos,
    HPEN gridPen, HBRUSH gridBrush )
{
    SelectObject( hdc, GetStockObject( WHITE_PEN ) );
    SelectObject( hdc, GetStockObject( WHITE_BRUSH ) );

    Ellipse( hdc,
        oldTailPos->x    , oldTailPos->y,
        oldTailPos->x + 1, oldTailPos->y + 1 );

    // Re-draw old tail's square
    SelectObject( hdc, gridPen );
    SelectObject( hdc, gridBrush );
    
    MoveToEx( hdc, oldTailPos->x + 1, oldTailPos->y    , NULL );
    LineTo  ( hdc, oldTailPos->x    , oldTailPos->y     );
    LineTo  ( hdc, oldTailPos->x    , oldTailPos->y + 1 );
    LineTo  ( hdc, oldTailPos->x + 1, oldTailPos->y + 1 );
}

BOOL putAppleOnBrd( POINT *aplPos, char ( *brd )[ BRD_SIZE_SQ ] )
{
    int tries = 0;
    BOOL appleOn = FALSE;

    while ( ( !appleOn ) && tries < 100 )
    {
        // Get a random position on board
        aplPos->x = rand() % BRD_SIZE_SQ;
        aplPos->y = rand() % BRD_SIZE_SQ;

        // aplPos square free? ( no wall and no snake? )
        if ( brd[ aplPos->y ][ aplPos->x ] == 0 )
            appleOn = TRUE;     // square free --> set apple flag
        else
            tries++;            // try again
    }

    return appleOn;
}

void drawApple( HDC hdc, POINT *aplPos, HPEN aplPen, HBRUSH aplBrush )
{
    SelectObject( hdc, aplPen );
    SelectObject( hdc, aplBrush );

    Ellipse( hdc,
        aplPos->x    , aplPos->y,
        aplPos->x + 1, aplPos->y + 1 );
}