#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <random>
#include <string>
#include <conio.h>
#include <atomic>
#include <chrono>

using namespace std;
mutex mtx;
vector<int> mode(3, 0);
using std::thread;
int gameScore = 0;
string inputWord = "";
int play = 1;
#define mapY 25
#define mapX 7

// 좌표로 출력
void gotoxy(int x, int y) { 
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 컴포넌트 인터페이스
class StringComponent {
public:
    virtual std::string getString() const = 0;
    virtual ~StringComponent() = default;
};

// 구체적인 컴포넌트
class BaseString : public StringComponent {
public:
    std::string getString() const override {
        return "";
    }
};

// 알파벳 데코레이터
class Alphabet : public StringComponent {
    StringComponent* component;

public:
    explicit Alphabet(StringComponent* comp) : component(comp) {}

    std::string getString() const override {
        return component->getString() + "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }
};

// 숫자 데코레이터
class Number : public StringComponent {
    StringComponent* component;

public:
    explicit Number(StringComponent* comp) : component(comp) {}

    std::string getString() const override {
        return component->getString() + "0123456789";
    }
};

// 기호 데코레이터
class Symbol : public StringComponent {
    StringComponent* component;

public:
    explicit Symbol(StringComponent* comp) : component(comp) {}

    std::string getString() const override {
        return component->getString() + "!@#$%^&*()_+-=[]{}|;:,.<>?";
    }
};

// 단어 클래스
class Word {
    std::string word;
    int x, y;
public:
    Word(std::string word_, int x_, int y_) : word(word_), x(x_), y(y_) {}

    std::string getWord() const {
        return word;
    }

    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    void chageY(int y_) {
        y = y_;
    }

    // 단어가 같은지를 비교
    bool operator==(const Word& other) const {
        return this->word == other.word; 
    }
};

// Word클래스 타입의 백터
std::vector<Word> wordVec;

// 랜덤 단어 생성
std::string randomString(int len) {
    srand(static_cast<unsigned int>(time(0)));

    std::vector<StringComponent*> component;
    std::string result = "";
    std::string randomStr = "";

    component.push_back(new Alphabet(new BaseString()));
    component.push_back(new Number(new BaseString()));
    component.push_back(new Symbol(new BaseString()));    

    for (int i = 0; i < 3; i++) { // 입력한 모드에 따라 문자열 더하기
        if (mode[i]) result += component[i]->getString();
    }
    for (int i = 0; i < len; i++) { // 길이만큼 랜덤 단어 생성
        randomStr += result[rand() % result.size()];
    }

    return randomStr;
}

// 단어 생성 함수
void generateWord() {
    srand(static_cast<unsigned int>(time(0)));

    // 콘솔 창의 너비
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    // 최대 길이는 화면의 길이 1/4
    int maxLength = consoleWidth / mapX;
    // 문자열 시작점 지정
    int randomX = rand() % mapX * maxLength;    

    // 문자열 길이 랜덤
    int randomLength = rand() % maxLength + 1;
    std::string randomStr = randomString(randomLength);

    Word newWord(randomStr, randomX, 0); // 객체 생성
    std::lock_guard<std::mutex> lock(mtx); // 쓰레드 간에 데이터에 안전하게 접근할 수 있도록 보호
    wordVec.push_back(newWord); // 백터에 새로운 객체 저장
}

// 화면 출력
void printMap() {
    system("cls");
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& w : wordVec) { // 백터에 저장된 단어 출력
        gotoxy(w.getX(), w.getY());
        std::cout << w.getWord();
    }
    gotoxy(0, mapY);
    std::cout << "------------------------------------------------------------------------------------------------------------------------";
    gotoxy(108, mapY + 1);
    cout << "Score : " << gameScore; gotoxy(0, mapY + 1);
    cout << "Input : " << inputWord;
}

// 게임 시작 모드 설정
void gameStart() {
    std::vector<std::string> modeName = { // 출력할 문자열 저장
        "Alphabet",
        "Number",
        "SpecialSymbol"
    };

    std::cout << "Choose Mode" << std::endl;

    for (int i = 0; i < 3; ++i) { // 모드 입력받기
        std::cout << modeName[i] << " : [1/0] -> ";
        std::cin >> mode[i];
    }
}

// 종료 화면 출력
void gameOver() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    gotoxy(consoleWidth / 2 - 6, mapY / 2);
    cout << "  GAME OVER!  ";
    gotoxy(0, mapY + 1);
    return;
}

// 단어 1칸 내리기
void moveWordsDown() {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& word : wordVec) {
        if (word.getY() < mapY - 1) { // 단어의 y가 화면을 넘기지 않을때
            word.chageY(word.getY() + 1); // 단어의 y 1증가
        }
        else play = 0; // 종료조건
    }
}

// 입력한 단어 있는지 여부 확인
void checkWordCollision(const std::string& userInput) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& word : wordVec) {
        if (userInput == word.getWord()) { // 입력값이 벡터의 객체랑 같을때
            gameScore++; // 점수증가
            wordVec.erase(std::remove(wordVec.begin(), wordVec.end(), word), wordVec.end()); // 해당 원소 삭제
            break;
        }
    }
}
 // 키 입력 여부
bool keyPressed() {
    return _kbhit() != 0;
}

// 입력한 키 리턴 
char getPressedKey() {
    if (keyPressed()) {
        return _getch();
    }
    return '\0';
}

// 게임 시작
void gamePlay() {

    gameStart();
    inputWord = "";

    thread inputThread([&]() { // 입력 쓰레드
        while (play) {
            char word = getPressedKey();
            if (word == '\r') { // 엔터를 눌렀을때
                checkWordCollision(inputWord);
                inputWord = ""; //입력 값 초기화
            }
            else if (word == '\b' && !inputWord.empty()) { // 백스페이스 입력시
                inputWord.pop_back();
            }
            else if (word >= 32 && word <= 126) { // 문자인지 여부
                inputWord += word;
            }
        }
    });

    thread downThread([&]() { // 다운 쓰레드
        int gameCnt = 0;
        while (play) {
            srand(static_cast<unsigned int>(time(0)));
            int gameSpeed = rand() % 3 + 1; // 속도 랜덤            
            moveWordsDown();
            if (gameCnt % 5 == 0) generateWord(); // 5번 싸이클 마다 단어 생성
            gameCnt = (gameCnt + 1) % 10;
            Sleep(500 * gameSpeed);
        }
    });

    while (play) {
        printMap();
        Sleep(1000);
    }

    inputThread.join();
    downThread.join();
}

int main() {
    gamePlay();
    gameOver();
    exit(0);
    return 0;
}

