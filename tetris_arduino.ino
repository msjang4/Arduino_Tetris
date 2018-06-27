
#include <Esplora.h>
#include <SPI.h>
#include <TFT.h>
#include <EEPROM.h>
byte check[7];
bool hasseven = false;
bool onGround = false;
bool wasSpinned = false;
const byte movebobby = 18, rotatebobby=6;
byte canmovebobby = movebobby;
byte canrotatebobby = rotatebobby;
const byte gap_next_hold = 35;
const byte gap_hold_scoring = 30;
short SpeedLevel[21] = {
	0,490*5/3,450*5/3,410*5/3,370*5/3,330*5/3,280*5/3,220*5/3,170*5/3,110*5/3,100*5/3,90*5/3,80*5/3,70*5/3,60*5/3,60*5/3,50*5/3,50*5/3,40*5/3,40*5/3,30*5/3
};
enum{gap_level_score = 13};
char JLSTZ_Order[8][5][2] = {
  { { 0, 0 },{ -1, 0 },{ -1,-1 },{ 0,+2 },{ -1,+2 } },
  { { 0, 0 },{ +1, 0 },{ +1,+1 },{ 0,-2 },{ +1,-2 } },
  { { 0, 0 },{ +1, 0 },{ +1,+1 },{ 0,-2 },{ +1,-2 } },
  { { 0, 0 },{ -1, 0 },{ -1,-1 },{ 0,+2 },{ -1,+2 } },
  { { 0, 0 },{ +1, 0 },{ +1,-1 },{ 0,+2 },{ +1,+2 } },
  { { 0, 0 },{ -1, 0 },{ -1,+1 },{ 0,-2 },{ -1,-2 } },
  { { 0, 0 },{ -1, 0 },{ -1,+1 },{ 0,-2 },{ -1,-2 } },
  { { 0, 0 },{ +1, 0 },{ +1,-1 },{ 0,+2 },{ +1,+2 } }
};
char I_Order[8][5][2] = {
  { { 0, 0 },{ -2, 0 },{ +1, 0 },{ -2,+1 },{ +1,-2 } },
  { { 0, 0 },{ +2, 0 },{ -1, 0 },{ +2,-1 },{ -1,+2 } },
  { { 0, 0 },{ -1, 0 },{ +2, 0 },{ -1,-2 },{ +2,+1 } },
  { { 0, 0 },{ +1, 0 },{ -2, 0 },{ +1,+2 },{ -2,-1 } },
  { { 0, 0 },{ +2, 0 },{ -1, 0 },{ +2,-1 },{ -1,+2 } },
  { { 0, 0 },{ -2, 0 },{ +1, 0 },{ -2,+1 },{ +1,-2 } },
  { { 0, 0 },{ +1, 0 },{ -2, 0 },{ +1,+2 },{ -2,-1 } },
  { { 0, 0 },{ -1, 0 },{ +2, 0 },{ -1,-2 },{ +2,+1 } }
};
class Color //컬러 클래스 
{
public:
    byte r, g, b;

    Color() : r(0), g(0), b(0)
    {
    }

    Color(long rgb) :
        b((rgb >> 16) & 255),
        g((rgb >> 8) & 255),
        r((rgb >> 0) & 255)
    {
    }


    void EsploraTFT_background() const
    {
        EsploraTFT.background(this->b, this->g, this->r);
    }

    void EsploraTFT_stroke() const
    {
        EsploraTFT.stroke(this->b, this->g, this->r);
    }

    void EsploraTFT_fill() const
    {
        EsploraTFT.fill(this->b, this->g, this->r);
    }
};


namespace BasicColors
{
    Color WHITE  (0xFFFFFFL); 
    Color GRAY   (0x808080L);
    Color BLACK  (0x000000L);
    Color RED    (0xFF0000L); 
    Color YELLOW (0xFFFF00L);
    Color OLIVE  (0x808000L);
    Color BLUE   (0x0000FFL);
    Color PURPLE (0x800080L);

    Color LIGHT_GRAY(0xC0C0C0L);
}

class Retarder
{
    long time;

    long nextTimeout;                                                                                                                                                               

public:
    Retarder(long time) : time(time)
    {
        this->restart();
    }

    void setTime(long time)
    {
        this->time = time;
        this->restart();
    }

    bool expired()
    {
        if (millis() > this->nextTimeout)
        {
            this->restart();

            return true;
        }

        return false;
    }

    void restart(int addDelay = 0)
    {
        this->nextTimeout = millis() + this->time + addDelay;
    }
};

class _SwitchesManager
{
    bool clearedKey[20];
public:
    bool isPressed(int button)
    {
        bool switchToggled = Esplora.readButton(button) == LOW;

        if (this->clearedKey[button] && switchToggled)
        {
            this->clearedKey[button] = false;
            return true;
        }
        else if (!switchToggled)
        {
            this->clearedKey[button] = true;
        }

        return false;
    }
};

_SwitchesManager SwitchesManager;

const char *SOUND_ICON =
        "    #  # "
        "   ##   #"
        "##### # #"
        "##### # #"
        "##### # #"
        "   ##   #"
        "    #  # ";

const char *NO_SOUND_ICON =
        "    #    "
        "   ##    "
        "##### # #"
        "#####  # "
        "##### # #"
        "   ##    "
        "    #    ";

class _SoundManager
{
public:
    Color background;
    bool isFXEnabled;
    long showIconTime;

    _SoundManager() : isFXEnabled(true), showIconTime(0)
    {
    }
    \
    void playFXSound(int freq, int duration)
    {
        if (isFXEnabled)
        {
            Esplora.noTone();
            Esplora.tone(freq, duration);
        }
    }

};

_SoundManager SoundManager;

namespace ActivityManager
{
    enum
    {
        GAME_START,
        ONGOING_GAME,
        GAME_OVER

    } Activity;

    class ActivityLoop
    {
    public:
        virtual void initialize() = 0;

        virtual void loop() = 0;
    };

    void SetGameActivity(int activity);

    void StartTetris();
}

namespace BoardGame
{
    enum { BOARD_OFFSET_X = 18 };
    enum { BOARD_OFFSET_Y = 2 };
    enum { NEXT_PIECE_OFFSET_X = 120 };
    enum { NEXT_PIECE_OFFSET_Y = 5 };
    enum { HOLD_PIECE_OFFSET_X = 120 };
    enum { HOLD_PIECE_OFFSET_Y = 5+gap_next_hold };
    enum { DISPLAY_WIDTH  = 160 };
    enum { DISPLAY_HEIGHT = 128 };
    class Piece
    {
    public:
        virtual void rotate(bool dir) = 0;
        virtual bool isO(){};
        virtual bool isI(){};
        virtual int getState(){};
    };
    class TetrisPiece : public Piece // Piece상속
    {
        bool O=false;
        bool I= false;
        byte state=0;//바꿈
    public:
        bool p[5][5];
        TetrisPiece(const char *p0 = 0)
        {
            if (p0)
            {
                for (byte x = 0; x < 5; x ++)
                for (byte y = 0; y < 5; y ++)
                {
                    this->p[x][y] = *p0 == ' ' ? false : true;
                    p0 ++;
                }
            }
        }
        void setState(int val){
            this->state= val;
        }
        void changeState(bool dir){
            if(dir)
                state = (state+1)%4;
            else   
                state = (4+state-1)%4;
        }
        bool isO(){
            return O;
        }
        bool isI(){
            return I;
        }
        int getState(){
            return state;  
        }
        void setIsO(bool val){
            O = val;
        }
        void setIsI(bool val){
            I = val;
        }
        
        void rotate(bool dir)
        {
            if(this->isO()){
                return;
            }
            TetrisPiece buffer(*this);
            if(this->isI()){
                for (byte j = 0; j < 4; j ++)
                for (byte i = 1; i < 5; i ++)
                    this->p[i][j] = dir ? buffer.p[1+j][4- i] : buffer.p[4 -  j][i-1];
            }
            else
                for (byte j = 0; j < 5; j ++)
                for (byte i = 0; i < 5; i ++)
                    this->p[i][j] = dir ? buffer.p[j][4- i] : buffer.p[4 - j][i];
            changeState(dir);
    
        }

        bool get(int x, int y) const
        {
            return this->p[x][y];
        }
    };

   const Color ColorsList[] =
    {
        
        Color(0xFFFF00L), //O미노 노란!!
        Color(0x00FF00L), //S미노 녹색!!
        Color(0x00FFFFL), //I미노 시안!!
        Color(0xFF7F00L), //L미노 주황색

        Color(0xFF0000L), //Z미노 빨강
        Color(0xFF00FFL), // T미노 보라
        
        Color(0x0000FFL) //J미노 파랑

        // Color(0xFFFF00L), //O미노 노란
        //    Color(0x00FF00L), //S미노 녹색
        // Color(0x00FFFFL), //I미노 시안
        // Color(0xff7f00L), //L미노 귤색

        // Color(0xFF0000L), //Z미노 빨강
        // Color(0xAA00FFL), // T미노 보라
    // Color(0x0000FFL) //J미노 파랑

        // Color(0xFF4444L),
        // Color(0x00FF00L),
        // Color(0x6666FFL),
        // Color(0xAAAAAAL),
        // Color(0x00FFDDL),
        // Color(0xFF00FFL),
        // Color(0xFFFF00L)
    };

    class Board
    {
        enum { MAX_BOARD_WIDTH  =  10 };
        enum { MAX_BOARD_HEIGHT = 21 };

        enum { CLEARED_CELL = 255 };

        byte p[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH];

        byte numCols, numRows, pieceSize;

    public:
        int getNumRows() const
        {
            return this->numRows;
        }

        int getNumCols() const
        {
            return this->numCols;
        }

        int getPieceSize() const
        {
            return this->pieceSize;
        }

        void initialize(byte numCols, byte numRows, byte pieceSize)
        {
            this->numCols = numCols;
            this->numRows = numRows;
            this->pieceSize = pieceSize;

            for (byte j = 0; j < this->numRows; j ++)
            for (byte i = 0; i < this->numCols; i ++)
                this->p[j][i] = 0;
        }

        void moveRowDown(int row) //row 행을 삭제하고 다 한칸씩 밑으로 땡겨버림
        {
            for (byte r = row; r > 0; r --)
            {
                for (byte col = 0; col < this->numCols; col ++)
                    this->p[r][col] = this->p[r - 1][col];
            }

            for (byte col = 0; col < this->numCols; col ++)
                this->p[0][col] = 0;
        }

        bool isPerfectClear() const  // 모든 행이 꽉찼거나 비어 있는지 확인
        {
            for(byte row=0; row < this->numRows;row++){
                for (byte col = 0; col < this->numCols; col ++)
                    {
                        if (this->p[row][col]!=0)//빈칸이 아니라면 
                            return false;
                            //즉 꽉차지 않은행은 텅텅비어있어야만한다는 뜻
                }
            }
            return true;
        }

        bool isRowComplete(int row) const  // 해당 행이 꽉찼는지 확인
        {
            for (byte col = 0; col < this->numCols; col ++)
            {
                if (!this->p[row][col])//p[row][col]==0이면 즉, 빈칸이 있으면 
                    return false;
            }
            return true;
        }

        void consolidatePiece(int posX, int posY, const TetrisPiece &piece, int value) // Tetris
        {
            for (byte j = 0; j < 5; j ++)
            {
                int cy = j + posY;

                for (byte i = 0; i < 5; i ++)
                {
                    int cx = i + posX;

                    if (piece.get(i, j))
                        this->p[cy][cx] = value + 1;
                }
            }
        }
        bool isTSpin(int posX, int posY, const TetrisPiece &piece, int value)
        {
            if(value!=5)
                return false;
            byte cnt=0;
            for (byte j = 1; j <=3; j +=2)
            {
                int cy = j + posY;

                for (byte i = 1; i <=3 ;i +=2)
                {
                    int cx = i + posX;
                    
                    if(cx < 0 || cy < 0 || cx >= this->getNumCols() || cy >= this->getNumRows() ||this->p[cy][cx]!=0)//빈칸이 아니라면
                        cnt++;
                }
            }
            return cnt>=3;
        }

        void removeClear()
        {
            for (byte cx = 0; cx < this->numCols; cx ++)
            for (byte cy = 0; cy < this->numRows; cy ++)
            {
                if (this->p[cy][cx] == CLEARED_CELL)
                {
                    for (int k = cy; k > 0; k --)
                        this->p[k][cx] = this->p[k - 1][cx];

                    this->p[0][cx] = 0;
                }
            }
        }

        bool isClear(int cx, int cy) const
        {
            return this->p[cy][cx] == CLEARED_CELL;
        }

        byte get(int cx, int cy) const
        {
            return this->p[cy][cx];
        }
    };

    class GravityGame : public ActivityManager::ActivityLoop
    {
    


    protected:
        bool paused, BtB;
        unsigned short score; //바꿈
        char Ren,Level,sentline,goal;//바꿈
        Board &board;

        Piece &currentPiece;
        char currPosX, currPosY;//바꿈
        Piece &nextPiece;
        Piece &holdPiece;
        char Status1[15], Status2[15];//바꿈
        char clearRowState;

        Retarder moveDownRetarder;
        Retarder keyRetarder;
        Retarder keyDownRetarder;
        Retarder rotateRetarder;
        Retarder clearRowsRetarder;
        Retarder dropRetarder;
        bool canHold;
    public:
        const char *TITLE;

        GravityGame(Board &board, Piece &currentPiece, Piece &nextPiece,  Piece &holdPiece,const char *title) :
            board            (board),
            currentPiece     (currentPiece),
            nextPiece        (nextPiece),
            holdPiece        (holdPiece),
            TITLE            (title),
            keyDownRetarder  (50),
            dropRetarder     (150), //300
            rotateRetarder   (200), //300
            moveDownRetarder (1000),
            keyRetarder      (100),
            canHold         (true),
            clearRowsRetarder(100)
        {
        }

        virtual void initialize() = 0;

        virtual void initialize(int numCols, int numRows, int pieceSize)
        {
            this->paused = false;

            this->board.initialize(numCols, numRows, pieceSize);
            this->score = 0;
            this->Level = 1;
            this->Ren =0;
            this->BtB = false;
            this->sentline = 0;
            this->goal = 10;
            this->clearRowState = -1;
            this->moveDownRetarder.setTime(SpeedLevel[1]);

            this->paintBackground();

            this->paintScore();
        }
        virtual bool isPaused() const
        {
            return this->paused;
        }
    
        virtual int getLevel(){
            return this->Level;
        }
        virtual void addScore(int points)
        {
            this->score += points;
        }

        virtual int getScore() const
        {
            return this->score;
        }

        virtual void paintBackground()
        {
            BasicColors::BLACK.EsploraTFT_background();
            SoundManager.background = BasicColors::BLACK;

            EsploraTFT.noFill();
            BasicColors::WHITE.EsploraTFT_stroke();

            int leftLineX  = BOARD_OFFSET_X - 2;
            int rightLineX = BOARD_OFFSET_X + this->board.getNumCols() * this->board.getPieceSize();

            EsploraTFT.line(leftLineX,  0, leftLineX,  DISPLAY_HEIGHT);
            EsploraTFT.line(rightLineX, 0, rightLineX, DISPLAY_HEIGHT);
            EsploraTFT.line(rightLineX, HOLD_PIECE_OFFSET_Y+gap_hold_scoring,DISPLAY_WIDTH-1 ,  HOLD_PIECE_OFFSET_Y+gap_hold_scoring);
            EsploraTFT.line(rightLineX, 95, DISPLAY_WIDTH-1,  95);

            EsploraTFT.text("NEXT:",  90, 10);
            EsploraTFT.text("HOLD:", 90, 5+gap_next_hold);
            EsploraTFT.text("LEVEL:", 90, 102);
            EsploraTFT.text("SCORE:", 90, 102+gap_level_score);
        }

        virtual void paintFullBoard()
        {
            const int pieceSize = this->board.getPieceSize();

            EsploraTFT.noStroke();

            for (byte j = 0; j < this->board.getNumRows(); j ++)
            {
                int py = BOARD_OFFSET_Y + j * pieceSize;

                for (byte i = 0; i < this->board.getNumCols(); i ++)
                {
                    int px = BOARD_OFFSET_X + i * pieceSize;

                    BasicColors::GRAY.EsploraTFT_fill();

                    EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);
                }
            }
        }

        virtual void paintBoard() const
        {
            const int pieceSize = this->board.getPieceSize();

            EsploraTFT.noStroke();

            for (byte cy = 0; cy < this->board.getNumRows(); cy ++)
            {
                int py = BOARD_OFFSET_Y + cy * pieceSize;

                for (byte cx = 0; cx < this->board.getNumCols(); cx ++)
                {
                    int px = BOARD_OFFSET_X + cx * pieceSize;

                    if (this->board.get(cx, cy))
                        ColorsList[this->board.get(cx, cy) - 1].EsploraTFT_fill();
                    else
                        BasicColors::BLACK.EsploraTFT_fill();

                    EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);
                }
            }
        }
        
    
        virtual void paintScore(byte offset=0) const
        {
            char levelS[10];
            char scoreS[10];

            String(this->getLevel()).toCharArray(levelS, 10);
            String(this->score)     .toCharArray(scoreS, 10);

            EsploraTFT.noStroke();
            BasicColors::BLACK.EsploraTFT_fill();


            int rightLinex = BOARD_OFFSET_X + this->board.getNumCols() * this->board.getPieceSize();
            EsploraTFT.rect(rightLinex+5,  HOLD_PIECE_OFFSET_Y+gap_hold_scoring-1+5,70, 20);
            EsploraTFT.rect(rightLinex+45,  102-1,160-rightLinex-45, 10+gap_level_score);
            //EsploraTFT.rect(140 - 50,  109, 50, 10);



            EsploraTFT.noFill();
            BasicColors::WHITE.EsploraTFT_stroke();
            if(strcmp("          ",Status1)==0){
                EsploraTFT.text(Status2,     150 - 6 * strlen(Status2), HOLD_PIECE_OFFSET_Y+gap_hold_scoring+10);
            
            }
            else{
            
            EsploraTFT.text(Status1,     150 - 6 * strlen(Status1)+offset, HOLD_PIECE_OFFSET_Y+gap_hold_scoring+5);
            EsploraTFT.text(Status2,     150 - 6 * strlen(Status2)+offset, HOLD_PIECE_OFFSET_Y+gap_hold_scoring+10+5);
            }
            EsploraTFT.text(levelS,     155 - 6 * strlen(levelS), 102);
            EsploraTFT.text(scoreS,     155 - 6 * strlen(scoreS), 102+gap_level_score);
            
            //EsploraTFT.text(bestScoreS, 140 - 6 * strlen(bestScoreS), 110);
        }

        virtual void togglePause()
        {
            this->paused = !this->paused;

            if (this->paused)
            {
                this->paintFullBoard();

                this->paintNextPiece(false);
                this->paintHoldPiece(false);

                const char *pauseStr = "PAUSE";
                int pauseStrY = (EsploraTFT.width() - 12 * strlen(pauseStr)) / 2;

                EsploraTFT.textSize(2);
                BasicColors::OLIVE.EsploraTFT_stroke();
                EsploraTFT.text(pauseStr, pauseStrY, 30);
                BasicColors::YELLOW.EsploraTFT_stroke();
                EsploraTFT.text(pauseStr, pauseStrY + 2, 32);
                EsploraTFT.textSize(1);
            }
            else
            {
                this->paintBackground();

                this->paintScore();

                this->paintBoard();

                this->paintNextPiece(true);

                this->paintPiece(true);
                this->paintHoldPiece(true);
            }
        }

        virtual void paintNextPiece(bool set = true) const = 0;
        virtual void paintHoldPiece(bool set = true) const = 0;

        virtual void paintPiece(bool set = true) const = 0;
        virtual bool tryPiece() const = 0;
        virtual void consolidatePiece() = 0;
         bool isEmpty(TetrisPiece& piece){
            for (byte x = 0; x < 5; x ++)
            for (byte y = 0; y < 5; y ++)
                if(piece.get(x,y) == true)
                    return false;
            return true;

        }
        void clearPiece(TetrisPiece& piece){
            for (byte x = 0; x < 5; x ++)
            for (byte y = 0; y < 5; y ++)
                piece.p[x][y] = false;
        }

        void dropPiece(){
            this->paintPiece(false);
            
                SoundManager.playFXSound(1100, 10);
            this->currPosY = this->getDropY();
            this->movePieceDown();
        }
        void drawGhostPiece(const TetrisPiece& piece, Color color,bool set=true){
            if(this->clearRowState!=-1)
                return;
            int pieceSize = this->board.getPieceSize();
            int y = BOARD_OFFSET_Y+this->getDropY()*pieceSize, x=BOARD_OFFSET_X + this->currPosX * pieceSize;
            
            EsploraTFT.noFill();
            
            for (byte j = 0; j < 5; j ++)
            {
                int py = y + j * pieceSize;

                for (byte i = 0; i < 5; i ++)
                {
                     int px = x + i * pieceSize;
                    bool isCell = piece.get(i, j);

                    if (isCell)
                    {
                        if (isCell && set)
                           color.EsploraTFT_stroke();
                        else
                            BasicColors::BLACK.EsploraTFT_stroke();

                        EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);
                    }
                }
            }

        }

        int getDropY(){
            int tmp = this->currPosY;
            for(int i=this->currPosY; i<this->board.getNumRows() ;i++){
                this->currPosY=i;
                if(!this->tryPiece()){
                    this->currPosY= tmp;
                    return i-1;
                }                
            }
        }
        void tryMovePiece(int dir)
        {
            this->paintPiece(false);

            this->currPosX += dir;

            if (this->tryPiece())
                SoundManager.playFXSound(4400, 10);
            else
                this->currPosX -= dir;

            this->paintPiece(true);
        }
        int getKickIndex(Piece& piece,bool dir){
  
            if(dir)
                return 2*piece.getState();
            else
                return (4+piece.getState()-1)%4*2+1;
        }
        void tryRotatePiece(bool dir)
        {
            if(this->currentPiece.isO())
                return;
            this->paintPiece(false);
            int idx = getKickIndex(this->currentPiece,dir);
            this->currentPiece.rotate(dir);
            if(this->currentPiece.isI()){
                for(byte j=0;j<5;j++){
                    currPosX+= I_Order[idx][j][0];
                    currPosY+= I_Order[idx][j][1];
                    if (this->tryPiece()){
                        SoundManager.playFXSound(3300, 10);
                        this->paintPiece(true); 
                        return;
                    }
                    currPosX-= I_Order[idx][j][0];
                    currPosY-= I_Order[idx][j][1];
                }
                this->currentPiece.rotate(!dir);
            }
            else{
                for(byte j=0;j<5;j++){
                    currPosX+= JLSTZ_Order[idx][j][0];
                    currPosY+= JLSTZ_Order[idx][j][1];
                    if (this->tryPiece()){
                        SoundManager.playFXSound(3300, 10);
                          this->paintPiece(true);
                        return;
                    }
                    currPosX-= JLSTZ_Order[idx][j][0];
                    currPosY-= JLSTZ_Order[idx][j][1];
                }
                this->currentPiece.rotate(!dir);
            }
            this->paintPiece(true);
        }

        void movePieceDown()
        {
            this->paintPiece(false);

            this->currPosY ++;

            if (this->tryPiece())
            {
                SoundManager.playFXSound(2200, 10);
            }
            else
            {
                this->currPosY --;

                this->paintPiece();

                this->consolidatePiece();
            }

            this->paintPiece(true);
        }
        
        virtual void completeClear() = 0;
        virtual void holdingPiece() =0;
        virtual void paintClearRows(bool set = true) const = 0;

        virtual void loop() //중요
        {
            if (SwitchesManager.isPressed(SWITCH_LEFT))
                this->togglePause();

            if (this->clearRowState == -1)
            {
                if (!this->isPaused())
                {
                    if (this->moveDownRetarder.expired()){
                            this->movePieceDown();
                            currPosY++;
                            if(!this->tryPiece()){
                                moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); //SpeedLevel[1]-SpeedLevel[getLevel()]의 딜레이만 주기
                                onGround=true;
                            }   
                            currPosY--;
                            wasSpinned=false;
                    }
                    else
                    {
                        int joystickX = Esplora.readJoystickX();
                        int joystickY = Esplora.readJoystickY();
                        if (abs(joystickY) > abs(joystickX))
                        {
                                if (joystickY > 200 && this->keyDownRetarder.expired()){
                                    currPosY++;
                                    if(!this->tryPiece()){
                                      currPosY--;//땅 위에서 누르면 moveDown타이머를 기다리겠다는 뜻
                                    }   
                                    else{
                                        currPosY--;

                                        this->movePieceDown(); 
                                        //땅 위에위에서 down시 다음 moveDown타이머를 SpeedLevel[1]로 설정하겠다는 뜻
                                        currPosY++;
                                        if(!this->tryPiece()){
                                            moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); 
                                            onGround=true;
                                        }  
                                        currPosY--;
                                        wasSpinned=false;
                                    }
                                }
                                else if(joystickY < -200 && this->rotateRetarder.expired()){
                                    this->tryRotatePiece(true);
                                    currPosY++;
                                    if(canrotatebobby!=0&&!this->tryPiece()){
                                        moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); //SpeedLevel[1]-SpeedLevel[getLevel()]의 딜레이만 주기
                                        canrotatebobby--;
                                    }   
                                    currPosY--;
                                    wasSpinned=true;
                                }
                        }
                        else
                        {
                            if (joystickX < -200 && this->keyRetarder.expired()){
                                this->tryMovePiece(1); 
                                currPosY++;
                                if(canmovebobby!=0&&!this->tryPiece()){
                                    moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); //SpeedLevel[1]-SpeedLevel[getLevel()]의 딜레이만 주기
                              canmovebobby--;
                                }   
                                currPosY--;
                            }
                            if (joystickX > 200 && this->keyRetarder.expired()){
                                
                                this->tryMovePiece(-1);
                                currPosY++;
                                if(canmovebobby!=0&&!this->tryPiece()){
                                    moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); //SpeedLevel[1]-SpeedLevel[getLevel()]의 딜레이만 주기
                                   canmovebobby--;
                                }   
                                currPosY--;    
                            } //오른쪽 왼쪽 이동은 wasSpinned에 영향을 안주는게 맞다고 봄
                        }

                        if (SwitchesManager.isPressed(SWITCH_DOWN)&&this->dropRetarder.expired()){
                            if(!onGround){
                                wasSpinned=false; //얘는 무조건 wasSpinned가 아니여야함.
                            }
                            this->dropPiece();
                            
                        }
                        else if (SwitchesManager.isPressed(SWITCH_RIGHT)&&this->rotateRetarder.expired()){
                            
                            this->tryRotatePiece(false);
                            currPosY++;
                            if(canrotatebobby!=0&&!this->tryPiece()){
                                moveDownRetarder.restart(SpeedLevel[1]-SpeedLevel[getLevel()]); //SpeedLevel[1]-SpeedLevel[getLevel()]의 딜레이만 주기
                              canrotatebobby--;
                            }   
                            currPosY--;
                            wasSpinned=true;
                        }
                        else if(SwitchesManager.isPressed(SWITCH_UP)&&this->canHold){
                            this->canHold=false;
                            wasSpinned=false;
                            onGround = false;
                            this->holdingPiece();
                        }
                    }
                }
            }

            else if (this->clearRowsRetarder.expired())
            {
                this->clearRowState ++;

                if (this->clearRowState == 5)
                {
                    this->completeClear();
                }
                else
                {
                    if (!(this->clearRowState & 1))
                        SoundManager.playFXSound(6000, 50);

                    this->paintClearRows(this->clearRowState & 1);
                }
            }
        }

        virtual void gameOver()
        {
            ActivityManager::SetGameActivity(ActivityManager::GAME_OVER);
        }
    };

    const TetrisPiece TetrisPiecesList[] =
    {
        TetrisPiece(
                "     "
                "     "
                "  *X "
                "  XX "
                "     "),
        TetrisPiece(
                "     "
                " XX  "
                "  *X "
                "     "
                "     "),
        TetrisPiece(
                "     "
                "     "
                "XX*X "
                "     "
                "     "),
        TetrisPiece(
                "     "
                " X   "
                " X*X "
                "     "
                "     "),
        TetrisPiece(
                "     "
                "  XX "
                " X*  "
                "     "
                "     "),
        TetrisPiece(
                "     "
                "  X  "
                " X*X "
                "     "
                "     "),
        TetrisPiece(
                "     "
                "   X "
                " X*X "
                "     "
                "     ")
    };

    enum { NUM_TETRIS_PIECES = 7 };

    class TetrisGame : public GravityGame
    {
        byte current, next,hold;
        byte history[4];
        TetrisPiece currentPiece, nextPiece, holdPiece; 
    //    bool BtB;
        byte clearRows[4];
        byte numClearRows;

    public:
        TetrisGame(Board &board) :
            GravityGame(board, this->currentPiece, this->nextPiece, this->holdPiece,"TETRIS_RIA")
        {
        }

        void initialize()
        {
            
            GravityGame::initialize(10, 21, 6);//아아
            randomSeed(millis());
            srand(millis());
            srandom(millis());
            for(byte i=0;i<7;i++)
                check[i] = false;
            hasseven = false;
            this->next = random(NUM_TETRIS_PIECES);
            check[this->next] = true;
            this->nextPiece = TetrisPiecesList[this->next];
            clearPiece(this->holdPiece);
            this->nextPiece.setState(3);
            // isO설정
            this->nextPiece.setIsO(this->next==0);
            this->nextPiece.setIsI(this->next==2);
            this->numClearRows = 0;

            this->createPiece();
        }
        // virtual int getLevel() const
        // {
        //     int level = 1 + this->getScore() / 20;

        //     if (level > 10)
        //         level = 10;

        //     return level;
        // }

        bool tryPiece() const
        {
            for (byte j = 0; j < 5; j ++)
            {
                int cy = j + this->currPosY;

                for (byte i = 0; i < 5; i ++)
                {
                    int cx = i + this->currPosX;

                    if (this->currentPiece.get(i, j))
                    {
                        if (cx < 0 || cy < 0 || cx >= this->board.getNumCols() || cy >= this->board.getNumRows() || this->board.get(cx, cy))
                            return false;
                    }
                }
            }

            return true;
        }
        
        void holdingPiece(){
            
                SoundManager.playFXSound(5000, 10);
            if(this->isEmpty(this->holdPiece)){
                this->paintPiece(false);
                this->hold = this->current;
                this->holdPiece = this->currentPiece;
                if(!createPiece()){
                    this->paintNextPiece(false);
                    this->board.consolidatePiece(this->currPosX, this->currPosY, this->currentPiece, this->current);
                    this->gameOver();
                }
                this->paintPiece(true);
                this->paintHoldPiece();
            }
            else{
                this->paintPiece(false);
                this->currPosX = 2; this->currPosY = -2;
                
                TetrisPiece tmp = this->currentPiece;
                this->currentPiece = this->holdPiece;
                this->holdPiece = tmp;

                swap(this->current,this->hold);

                while (this->currPosY < 0 && !this->tryPiece())
                   this->currPosY ++;
                if(!this->tryPiece()){
                    this->paintNextPiece(false);
                        this->board.consolidatePiece(this->currPosX, this->currPosY, this->currentPiece, this->current);
                        this->gameOver();
                }
                this->paintPiece(true);
                this->paintHoldPiece();
            }
        }
        
        void paintPiece(const TetrisPiece &piece, const Color &color, int x, int y, bool set, bool clearBg) const
        {
            const byte pieceSize = this->board.getPieceSize();
            EsploraTFT.noStroke();
             for (byte j = 0; j < 5; j ++)
             {
                int py = y + j * pieceSize;

                for (byte i = 0; i < 5; i ++)
                {
                     int px = x + i * pieceSize;
                    bool isCell = piece.get(i, j);

                    if (clearBg || isCell)
                    {
                        if (isCell && set)
                            color.EsploraTFT_fill();
                        else
                            BasicColors::BLACK.EsploraTFT_fill();

                        EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);
                    }
                }
            }
        }

        void paintPiece(bool set = true) const //찐
        {
            const byte pieceSize = this->board.getPieceSize();
            drawGhostPiece(this->currentPiece,ColorsList[this->current], set);

            this->paintPiece(this->currentPiece, ColorsList[this->current],
                             BOARD_OFFSET_X + this->currPosX * pieceSize,
                             BOARD_OFFSET_Y + this->currPosY * pieceSize, set, false);
        }

        void paintNextPiece(bool set = true) const
        {
            this->paintPiece(this->nextPiece, ColorsList[this->next],
                             NEXT_PIECE_OFFSET_X,
                             NEXT_PIECE_OFFSET_Y, set, true);
        }

        void paintHoldPiece(bool set = true) const
        {
            this->paintPiece(this->holdPiece, ColorsList[this->hold],
                             HOLD_PIECE_OFFSET_X,
                             HOLD_PIECE_OFFSET_Y, set, true);
        }

        bool createPiece()//Tetris임
        {
            this->current      = this->next;
            this->currentPiece = this->nextPiece;

            this->currPosX = 2;
            this->currPosY = -2;

            while (this->currPosY < 0 && !this->tryPiece())
                this->currPosY ++;

            if (!this->tryPiece())
            {
                return false;
            }
            else
            {
                if(hasseven==false){
                    do{
                       this->next = random(NUM_TETRIS_PIECES); //LSJZOITLJOITT //책갈피2
                    }while(check[this->next]);
                    check[this->next] = true;
                    byte i;
                    for(i=0;i<7;i++)
                        if(check[i]==false)
                            break;
                    if(i==7)
                        hasseven=true;
                }
                else{
                    byte i=0;
                    do{
                      this->next = random(NUM_TETRIS_PIECES); //LSJZOITLJOITT //책갈피2 
                        i++;
                        if(i==6){
                            break; // 최신 TGM은 6번 시도한다 했음
                        }
                    }while(this->next==this->history[0]|| this->next== this->history[1] || this->next==this->history[2] ||this->next==this->history[3]);
                    //기록에 있는 거랑 다르게
                
                }
                for(byte i=0;i<3;i++){
                    history[i] = history[i+1];
                }
                history[3] = this->next;
               this->nextPiece = TetrisPiecesList[this->next];
               //isO, isI 설정
                this->nextPiece.setState(3);
                this->nextPiece.setIsO(this->next==0);
                this->nextPiece.setIsI(this->next==2);

                int randRot = random(4); 

                for (byte i = 0; i < randRot; i ++)
                    this->nextPiece.rotate(true);
                 this->nextPiece.rotate(true);
                 this->nextPiece.rotate(false);

                this->paintNextPiece(true);

                this->paintPiece(true);

                return true;
            }
        }
        void scoring(int Cnt,bool TSpin){
           
             if (Cnt != 0) {
                if (Ren != 0) {
                    if (TSpin == true) {
                        sprintf(Status1, "%dREN TSPIN", Ren);
                    }
                    else
                        sprintf(Status1, "  %dCombo  ", Ren);
                    score+= 5 * Ren * Level;
                    sentline+=Ren / 2;
                }
                else if (TSpin == true) {
                    sprintf(Status1, "  T-SPIN  ");                                       
                }
                Ren++;
                switch (Cnt) {
                case 1:
                    if (TSpin == true && BtB == true) {
                        strcpy(Status2, "SINGLE BTB");
                        score+=( 120 * Level);
                        sentline+=(3);
                    }
                    else {
                        strcpy(Status2, "  SINGLE  ");
                        BtB=(TSpin);
                        if (TSpin == true) {
                            score+=(80 * Level);
                            sentline+=(2);
                        }
                        else {
                            score+=(10 * Level);
                        }
                    }
                    break;
                case 2:
                    if (TSpin == true && BtB == true) {
                        strcpy(Status2, "DOUBLE BTB");
                        score+=( 180 * Level);
                        sentline+=(5);
                    }
                    else {
                        strcpy(Status2, "  DOUBLE  ");
                        BtB=(TSpin);
                        if (TSpin == true) {
                            score+=( 120 * Level);
                            sentline+=( 4);
                        }
                        else {
                            score+=(30 * Level);
                            sentline+=(1);
                        }
                    }
                    break;
                case 3:
                    if (TSpin == true && BtB == true) {
                        strcpy(Status2, "TRIPLE BTB");
                        score+=( 240 * Level);
                        sentline+=(7);
                    }
                    else {
                        strcpy(Status2, "  TRIPLE  ");
                        BtB=(TSpin);
                        if (TSpin == true) {
                            score+=(160 * Level);
                            sentline+=(6);
                        }
                        else {
                            score+=(50 * Level);
                            sentline+=(2);
                        }
                    }
                    break;
                case 4:
                    if (BtB == true) {
                        strcpy(Status2, "TETRIS BTB");
                        score+=(120 * Level);
                        sentline+=(5);
                    }
                    else {
                        strcpy(Status2, "  TETRIS  ");
                        BtB=(true);
                        score+=(80 * Level);
                        sentline+=(4);
                    }
                    break;
                }
                goal+=(-Cnt);
                while(goal <= 0) {
                    goal+=(10);
                    Level+=(1);
                }
            }
            else {
                Ren=(0);
                sprintf(Status1, "%s", "          ");
                sprintf(Status2, "%s", "          ");
            }
           
        }

        void consolidatePiece() // Tetris 
        {
            bool TSpin = wasSpinned&&this->board.isTSpin(this->currPosX, this->currPosY, this->currentPiece,this->current);
            this->board.consolidatePiece(this->currPosX, this->currPosY, this->currentPiece, this->current);
            this->numClearRows = 0;
            for (byte row = 0; row < this->board.getNumRows(); row ++)
            {
                if (this->board.isRowComplete(row))
                {
                    this->board.moveRowDown(row);
                    this->clearRows[this->numClearRows] = row;
                    this->numClearRows ++;
                }   
            }
            scoring(this->numClearRows, TSpin);
            if (this->numClearRows > 0)
            {
                SoundManager.playFXSound(6000, 50);

                this->paintClearRows(false);
                
                this->clearRowState = 0;
                this->clearRowsRetarder.restart();
            }
            else
            {
                if (!this->createPiece())
                {
                    this->paintNextPiece(false);

                    this->board.consolidatePiece(this->currPosX, this->currPosY, this->currentPiece, this->current);
                    this->gameOver();
                }

                this->moveDownRetarder.restart();
                this->keyDownRetarder.restart(150);
            }
            this->canHold= true;
            canmovebobby=movebobby;
            canrotatebobby=rotatebobby;
            
        }

        void paintClearRows(bool set) const
        {
            const int pieceSize = this->board.getPieceSize();

            EsploraTFT.noStroke();

            if (set)
                BasicColors::WHITE.EsploraTFT_fill();
            else
                BasicColors::BLACK.EsploraTFT_fill();

            for (byte k = 0; k < this->numClearRows; k ++)
            {
                byte py = BOARD_OFFSET_Y + this->clearRows[k] * pieceSize;

                byte px = BOARD_OFFSET_X;

                for (byte i = 0; i < this->board.getNumCols(); i ++)
                {
                    EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);

                    px += pieceSize;
                }
            }
        }
        
        void completeClear()
        {
            this->clearRowState = -1;

            if (!this->createPiece())
            {
                this->paintNextPiece(false);

                this->board.consolidatePiece(this->currPosX, this->currPosY, this->currentPiece, this->current);

                this->gameOver();
            }
            else
            {
                //책책
               
                this->paintBoard();
                this->paintPiece();
                 if(this->board.isPerfectClear()){
                    sentline+=7;
                    score+=250*Level;
                    sprintf(Status1, "%s", " PERFECT  ");
                    sprintf(Status2, "%s", "  CLEAR   ");
                    this->paintScore(3);
                }
                else
                    this->paintScore();

                this->moveDownRetarder.setTime(SpeedLevel[this->getLevel()]);
                this->keyDownRetarder.restart(150);
            }
        }
    };
    const char *tetrisStr = "Play TETRIS";
    const char *columnsStr = "Sound FX On/Off";

    const char *tetrisSelectedStr = ">> Play TETRIS <<";
    const char *columnsSelectedStr = ">> Sound FX On/Off <<";
    
    class GameStart
    {
        Retarder blinkRetarder;

        bool blink;

        int selected;

        char const *gameTitle;

    public:
        GameStart() : blinkRetarder(300), blink(false), selected(0), gameTitle("TETRIS_RIA")
        {
        }

        void setTitle(const char *gameTitle)
        {
            this->gameTitle = gameTitle;
        }

        void initialize()
        {
            BasicColors::BLUE.EsploraTFT_background();
            SoundManager.background = BasicColors::BLUE;

            if (gameTitle)
            {
                int gameStrX = (EsploraTFT.width() - 12 * strlen(gameTitle)) / 2;

                EsploraTFT.noFill();

                EsploraTFT.textSize(2);
                BasicColors::LIGHT_GRAY.EsploraTFT_stroke();
                EsploraTFT.text(gameTitle, gameStrX, 30);
                BasicColors::WHITE.EsploraTFT_stroke();
                EsploraTFT.text(gameTitle, gameStrX + 2, 32);
                EsploraTFT.textSize(1);
            }

            const char *authorStr = "by Jang MinSeok";
            EsploraTFT.text(authorStr,  (EsploraTFT.width() - 6 * strlen(authorStr)) / 2, 50);


            this->paintOptions();
        }

        void paintOptions()
        {
            EsploraTFT.noStroke();
            BasicColors::BLUE.EsploraTFT_fill();

            const int EsploraTFT_width = EsploraTFT.width();

            if (this->selected == 1)
                EsploraTFT.rect(0, 80, EsploraTFT_width, 8);

            if (this->selected == 0)
                EsploraTFT.rect(0, 95, EsploraTFT_width, 8);

            EsploraTFT.noFill();

            BasicColors::WHITE.EsploraTFT_stroke();

            if (this->selected == 1)
                EsploraTFT.text(tetrisStr,  (EsploraTFT_width - 6 * strlen(tetrisStr)) / 2, 80);
            
            if (this->selected == 0)
                EsploraTFT.text(columnsStr, (EsploraTFT_width - 6 * strlen(columnsStr)) / 2, 95);

            BasicColors::YELLOW.EsploraTFT_stroke();
            if (this->selected == 0)
                EsploraTFT.text(tetrisStr,  (EsploraTFT.width() - 6 * strlen(tetrisStr)) / 2, 80);

            if (this->selected == 1)
                EsploraTFT.text(columnsStr, (EsploraTFT.width() - 6 * strlen(columnsStr)) / 2, 95);
       //     this->toggleBlink();
        }
    void toggleSoundFX(){
            Esplora.noTone();
           SoundManager.isFXEnabled = !SoundManager.isFXEnabled;
            SoundManager.showIconTime = millis() + 1000;
        }
        void loop()
        {
       
            int offx = EsploraTFT.width() - 11;

                if (millis() <SoundManager.showIconTime)
                {
                    char const *icon = SoundManager.isFXEnabled ? SOUND_ICON : NO_SOUND_ICON;

                    for (byte i = 0; i < 7; i ++)
                    for (byte j = 0; j < 9; j ++)
                    {
                        if (*icon == ' ')
                            SoundManager.background.EsploraTFT_stroke();
                        else
                            BasicColors::YELLOW.EsploraTFT_stroke();

                        EsploraTFT.point(offx + j, 2 + i);

                        icon ++;
                    }
                }
                else
                {
                    SoundManager.background.EsploraTFT_fill();
                    SoundManager.background.EsploraTFT_stroke();
                        EsploraTFT.rect(offx-1, 2-1, 9+2, 7+2);//rect x, y, w, h

                }


            if (this->selected != 1 && Esplora.readJoystickY() > 250)
            {
                SoundManager.playFXSound(4400, 10);

                this->selected = 1;
             //   this->blink = true;
                this->paintOptions();
            }

            if (this->selected != 0 && Esplora.readJoystickY() < -250)
            {
                SoundManager.playFXSound(4400, 10);

                this->selected = 0;
             //   this->blink = true;
                this->paintOptions();
            }

            if (SwitchesManager.isPressed(SWITCH_DOWN))
            {
                if (this->selected == 0)
                    ActivityManager::StartTetris();

                if (this->selected == 1){
                    this->toggleSoundFX();      
                }
            }
        }
    };

    class GameOver
    {
        const Board &board;

        int blocksToPaint;

        Retarder fillBoardRetarder;

        void paintFullBoard()
        {
            const int pieceSize = this->board.getPieceSize();

            EsploraTFT.noStroke();

            int numBlocks = 0;

            BasicColors::GRAY.EsploraTFT_fill();

            for (byte cy = 0; cy < this->board.getNumRows(); cy ++)
            {
                for (int cx = this->board.getNumCols() - 1; cx >= 0; cx --)
                {
                    if (numBlocks == this->blocksToPaint && this->board.get(cx, cy))
                    {
                        int px = BOARD_OFFSET_X + cx * pieceSize;
                        int py = BOARD_OFFSET_Y + cy * pieceSize;

                        EsploraTFT.rect(px, py, pieceSize - 1, pieceSize - 1);
                    }

                    numBlocks ++;
                }
            }
        }

        void paintGameOver(bool alsoShadow)
        {
            const char *gameOverStr = "GAME OVER";

            EsploraTFT.textSize(2);

            int gameOverStrX = (EsploraTFT.width() - 12 * strlen(gameOverStr)) / 2;

            if (alsoShadow)
            {
                BasicColors::OLIVE.EsploraTFT_stroke();
                EsploraTFT.text(gameOverStr, gameOverStrX, 30);
            }

            BasicColors::YELLOW.EsploraTFT_stroke();
            EsploraTFT.text(gameOverStr, gameOverStrX + 2, 32);
            EsploraTFT.textSize(1);
        }

    public:
        GameOver(Board &board) : fillBoardRetarder(30), board(board)
        {
        }

        void initialize()
        {
            this->blocksToPaint = this->board.getNumCols() * this->board.getNumRows();

            this->paintGameOver(true);
        }

        void loop()
        {
            if (this->blocksToPaint > 0 && fillBoardRetarder.expired())
            {
                if ((this->blocksToPaint % this->board.getNumCols()) == 0)
                    SoundManager.playFXSound(3000, 20);

                this->blocksToPaint --;

                this->paintFullBoard();

                this->paintGameOver(false);
            }
            else
            {
                Esplora.noTone();
            }

            if (SwitchesManager.isPressed(SWITCH_DOWN))
                ActivityManager::SetGameActivity(ActivityManager::GAME_START);
        }
    };
}

namespace ActivityManager
{
    BoardGame::GameStart gameStart;
    BoardGame::Board board;
    BoardGame::TetrisGame tetrisGame(board);
    ActivityLoop *game = &tetrisGame;
    BoardGame::GameOver gameOver(board);
    int activity = 0;
    void StartTetris()
    {
        game = &tetrisGame;
        gameStart.setTitle(tetrisGame.TITLE);
        SetGameActivity(ONGOING_GAME);
    }
    void SetGameActivity(int _activity)
    {
        activity = _activity;
        Esplora.noTone();
        if (activity == GAME_START)
            gameStart.initialize();
        else if (activity == ONGOING_GAME)
            game->initialize();
        else if (activity == GAME_OVER)
            gameOver.initialize();
    }
    void Loop()
    {
        if (activity == GAME_START)
            gameStart.loop();
        else if (activity == ONGOING_GAME)
            game->loop();
        else if (activity == GAME_OVER)
            gameOver.loop();
    }
}

void setup()
{
    EsploraTFT.begin();
    long seed = Esplora.readLightSensor() * Esplora.readMicrophone() * Esplora.readTemperature(DEGREES_F);
    randomSeed(seed);
    srandom(seed);
    srand(seed);
    ActivityManager::SetGameActivity(ActivityManager::GAME_START);
}

void loop()
{
    ActivityManager::Loop();
}

   

