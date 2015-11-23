//
//  Snake.c
//

#include <windows.h>
#include <string.h>

// Timer config
#define     ID_TIMER        1       // Move-timer
#define     DELAY_TIMER     300     // Timer interval [ms]

// Directions
#define     UP              0
#define     RIGHT           1
#define     DOWN            2
#define     LEFT            3

// Board config
#define     BRD_SIZE_PX     500     // Board size in pixels
#define     BRD_SIZE_SQ     20      // Board size in squares (logical units)

// Snake config
#define     SNAKE_SIZE      6       // Total amount of links
#define     SNAKE_HEAD      ( SNAKE_SIZE - 1 )
#define     SNAKE_TAIL      0

// 'Win Proc'
LRESULT CALLBACK WndProc( HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam );

// Auxiliary functions
void initLevel( HWND hwnd,
    int *currentDirPt,
    BOOL *collisionPt, BOOL *pauseOnPt,
    POINT *snkBdy,
    char ( *brd )[ BRD_SIZE_SQ ],
    char ( *walls )[ BRD_SIZE_SQ ] );

void moveSnake( HWND hwnd,
    int cX, int cY,
    int *currentDirPt, BOOL *collisionPt,
    POINT *snkBdy, char ( *brd )[ BRD_SIZE_SQ ],
    HPEN tailPen, HBRUSH tailBrush,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol );

void setUpMappingMode( HDC hdc, int cX, int cY );

void drawGrid( HDC hdc );

void drawLevel( HDC hdc, char ( *brd )[ BRD_SIZE_SQ ] );

void drawHead( HDC hdc, POINT *snkBdy, BOOL *collisionPt,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol );

void drawNeck( HDC hdc, POINT *snkBdy );

void drawBody( HDC hdc, POINT *snkBdy );

void drawTail( HDC hdc, POINT *snkBdy, HPEN tailPen, HBRUSH tailBrush );

void deleteTail( HDC hdc, POINT *snkBdy, POINT *oldTailPos );

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
    
    // Snake status
    static int currentDir = 0;
    static BOOL collision = FALSE;
    static BOOL pauseOn = FALSE;
    
    // Colors
    static HPEN redPen, greenPen, brownPen;
    static HBRUSH redBrush, greenBrush, brownBrush;

    // Snake's body
    static POINT snakeBdy[ SNAKE_SIZE ] = { 0 };
    
    // Board
    static char board[ BRD_SIZE_SQ ][ BRD_SIZE_SQ ] = { 0 };

    // Definition level 1
    static char level01[ BRD_SIZE_SQ ][ BRD_SIZE_SQ ] =
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
        // Init brushes
        brownBrush = CreateSolidBrush( RGB( 0xA0, 0x5A, 0x2C ) );
        redBrush   = CreateSolidBrush( RGB( 0xFF, 0x00, 0x00 ) );
        greenBrush = CreateSolidBrush( RGB( 0x55, 0xD4, 0x00 ) );

        // Init pens
        brownPen = CreatePen( PS_SOLID, 0, RGB( 0xA0, 0x5A, 0x2C ) );
        redPen   = CreatePen( PS_SOLID, 0, RGB( 0xFF, 0x00, 0x00 ) );
        greenPen = CreatePen( PS_SOLID, 0, RGB( 0x55, 0xD4, 0x00 ) );

        // Init level status
        initLevel( hwnd,
            &currentDir, &collision, &pauseOn,
            snakeBdy, board, level01 );
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
            &currentDir, &collision,
            snakeBdy, board,
            brownPen, brownBrush,
            redPen, redBrush,
            greenPen, greenBrush );

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
                &currentDir, &collision, &pauseOn,
                snakeBdy, board, level01 );
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
            &currentDir, &collision,
            snakeBdy, board,
            brownPen, brownBrush,
            redPen, redBrush,
            greenPen, greenBrush );

        // Re-start move-timer, if not in collision state
        if ( !collision )
            SetTimer( hwnd, ID_TIMER, DELAY_TIMER, NULL );

        return 0;

    case WM_PAINT :
        hdc = BeginPaint( hwnd, &ps );

        setUpMappingMode( hdc, cxClient, cyClient );

        drawGrid( hdc );

        drawLevel( hdc, board );

        drawBody( hdc, snakeBdy );

        drawTail( hdc, snakeBdy, brownPen, brownBrush );

        drawHead( hdc, snakeBdy, &collision,
            redPen, redBrush, greenPen, greenBrush );

        EndPaint( hwnd, &ps );

        return 0 ;

    case WM_DESTROY :
        // Stop move-timer
        KillTimer( hwnd, ID_TIMER );

        // Delete pens
        DeleteObject( redPen );
        DeleteObject( greenPen );
        DeleteObject( brownPen );

        // Delete brushes
        DeleteObject( redBrush );
        DeleteObject( greenBrush );
        DeleteObject( brownBrush );

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
    int *currentDirPt,
    BOOL *collisionPt, BOOL *pauseOnPt,
    POINT *snkBdy,
    char ( *brd )[ BRD_SIZE_SQ ],
    char ( *walls )[ BRD_SIZE_SQ ] )
{
    int i;

    // Stop move-timer
    KillTimer( hwnd, ID_TIMER );

    // Set dir to UP
    *currentDirPt = UP;

    // Reset flags
    *collisionPt = FALSE;
    *pauseOnPt = FALSE;

    // Load level obstacles
    memcpy( brd, walls, BRD_SIZE_SQ * BRD_SIZE_SQ * sizeof( char ) );

    // Reset snake's position
    for ( i = SNAKE_TAIL; i < SNAKE_SIZE; i++ )
    {
        snkBdy[ i ].x = BRD_SIZE_SQ / 2 - 1;
        snkBdy[ i ].y = BRD_SIZE_SQ - 2 - i;

        // Block current sanke position on board
        brd[ snkBdy[ i ].y ][ snkBdy[ i ].x ] = 2;
    }

    // Re-start move-timer
    SetTimer( hwnd, ID_TIMER, DELAY_TIMER, NULL );

    // Trigger a re-paint
    InvalidateRect( hwnd, NULL, TRUE );
}

void moveSnake( HWND hwnd,
    int cX, int cY,
    int *currentDirPt, BOOL *collisionPt,
    POINT *snkBdy, char ( *brd )[ BRD_SIZE_SQ ],
    HPEN tailPen, HBRUSH tailBrush,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol )
{
    static HDC hdc;
    static POINT oldTail;

    //==================================================
    // Move snake's body
    //==================================================
    // Save original tail's pos
    oldTail = snkBdy[ SNAKE_TAIL ];

    // Shift position of all links towards the tail,
    // tail's original pos is overwritten
    memmove( snkBdy, snkBdy + 1, ( SNAKE_SIZE - 1 ) * sizeof( POINT ) );

    // Free tail's original pos on board
    brd[ oldTail.y ][ oldTail.x ] = 0;

    //==================================================
    // Update head's position
    //==================================================
    switch( *currentDirPt )
    {
    case UP :
        snkBdy[ SNAKE_HEAD ].y--;
        break;

    case DOWN :
        snkBdy[ SNAKE_HEAD ].y++;
        break;

    case RIGHT :
        snkBdy[ SNAKE_HEAD ].x++;
        break;

    case LEFT :
        snkBdy[ SNAKE_HEAD ].x--;
        break;

    default :
        break;
    }

    // Validate new head's pos
    if ( snkBdy[ SNAKE_HEAD ].x > ( BRD_SIZE_SQ - 1 ) )
        snkBdy[ SNAKE_HEAD ].x %= BRD_SIZE_SQ;

    if ( snkBdy[ SNAKE_HEAD ].x < 0 )
        snkBdy[ SNAKE_HEAD ].x += BRD_SIZE_SQ;

    if ( snkBdy[ SNAKE_HEAD ].y > ( BRD_SIZE_SQ - 1 ) )
        snkBdy[ SNAKE_HEAD ].y %= BRD_SIZE_SQ;

    if ( snkBdy[ SNAKE_HEAD ].y < 0 )
        snkBdy[ SNAKE_HEAD ].y += BRD_SIZE_SQ;

    //==================================================
    // Detect collisions
    //==================================================
    if ( brd[ snkBdy[ SNAKE_HEAD ].y ][ snkBdy[ SNAKE_HEAD ].x ] )
        *collisionPt = TRUE;
    else
        *collisionPt = FALSE;

    // Block new head pos on board
    if ( !( *collisionPt ) )
        brd[ snkBdy[ SNAKE_HEAD ].y ][ snkBdy[ SNAKE_HEAD ].x ] = 2;

    //==================================================
    // Update snake's plot
    //==================================================
    hdc = GetDC( hwnd );

    setUpMappingMode( hdc, cX, cY );

    deleteTail( hdc, snkBdy, &oldTail );

    drawNeck( hdc, snkBdy );

    drawTail( hdc, snkBdy, tailPen, tailBrush );

    drawHead( hdc, snkBdy, collisionPt,
        headPenCol, headBrushCol, headPenNoCol, headBrushNoCol );

    ReleaseDC( hwnd, hdc );
}

void drawGrid( HDC hdc )
{
    int i;

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

void drawHead( HDC hdc, POINT *snkBdy, BOOL *collisionPt,
    HPEN headPenCol, HBRUSH headBrushCol,
    HPEN headPenNoCol, HBRUSH headBrushNoCol  )
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
        snkBdy[ SNAKE_HEAD ].x    , snkBdy[ SNAKE_HEAD ].y,
        snkBdy[ SNAKE_HEAD ].x + 1, snkBdy[ SNAKE_HEAD ].y + 1 );
}

// Draw only first link after the head
void drawNeck( HDC hdc, POINT *snkBdy )
{
    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    SelectObject( hdc, GetStockObject( BLACK_BRUSH ) );

    Ellipse( hdc,
        snkBdy[ SNAKE_HEAD - 1 ].x    , snkBdy[ SNAKE_HEAD - 1 ].y,
        snkBdy[ SNAKE_HEAD - 1 ].x + 1, snkBdy[ SNAKE_HEAD - 1 ].y + 1 );
}

// Draw snake's body without head nor tail
void drawBody( HDC hdc, POINT *snkBdy )
{
    int i;

    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    SelectObject( hdc, GetStockObject( BLACK_BRUSH ) );
    
    for ( i = SNAKE_TAIL + 1; i < SNAKE_HEAD; i++ )
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

void deleteTail( HDC hdc, POINT *snkBdy, POINT *oldTailPos )
{
    SelectObject( hdc, GetStockObject( WHITE_PEN ) );
    SelectObject( hdc, GetStockObject( WHITE_BRUSH ) );

    Ellipse( hdc,
        oldTailPos->x    , oldTailPos->y,
        oldTailPos->x + 1, oldTailPos->y + 1 );

    // Re-draw old tail's square
    SelectObject( hdc, GetStockObject( BLACK_PEN ) );
    
    MoveToEx( hdc, oldTailPos->x + 1, oldTailPos->y    , NULL );
    LineTo  ( hdc, oldTailPos->x    , oldTailPos->y     );
    LineTo  ( hdc, oldTailPos->x    , oldTailPos->y + 1 );
    LineTo  ( hdc, oldTailPos->x + 1, oldTailPos->y + 1 );
}
