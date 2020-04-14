#include <windows.h>
#include <conio.h>
#include <cstdlib> 
#include <cstdio>  
#include <ctime>
#include <locale.h>
#include <fstream>
#include <iostream>


#define UP		72      // Код клавишь
#define LEFT	75
#define RIGHT	77
#define DOWN	80
#define ESC		27
#define PAUSE	32

using namespace std;

char name[16];


//функция переназначет стандартное положение курсора консоли в соответсвтие с переданыеми координатами x,y
void gotoxy(int x, int y) {
	HANDLE result;
	COORD NewPos;

	NewPos.X = x; // новый X
	NewPos.Y = y; // новый y
	result = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(result, NewPos);
	SetConsoleTextAttribute(result, FOREGROUND_GREEN | FOREGROUND_INTENSITY); // указыем какого цвета выводить символы в консоль (Насыщенно-зеленный)
}
//Функция срывающая курсор консоли (магиние палочки)
void InvisCursor() {
	CONSOLE_CURSOR_INFO a = { 100, FALSE };	// false - выключить показ курсора

	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &a);
}
//Функция рисует границы игрового поля
// Изначально использовалась кодировка ASCII для отрисовки горизонтальных и вертикальных линий ═,║, а также использовались уголки ╗,╝,╔,╚ для стыковки углов рамки
// но из-за подключения библиотеки locale.h и использовании русского языка (который использует другую кодировку (1251)) при повторной игре границы игрового поля рисовались
// другими символами (коды символов границы определялись по таблице кодировки 1251) что влияло на конечный вид границы
// поэтому в конечном варианте, решение представлено ниже с использованием обычного символа *
void Border() {
	// Рисуем горизонтальные грацицы
	for (int i = 2; i < 78; i++) 
{
		gotoxy(i, 3); printf("*");			// используя при этом (кодировку ASCII) символ * 
		gotoxy(i, 23); printf("*");
	}
	//Рисуем вертикальные границы
	for (int v = 4; v < 23; v++) 
	{
		gotoxy(2, v);  printf("*");
		gotoxy(77, v);  printf("*");
	}
	// Рисуем углы
	/*gotoxy(2, 3);    printf("*");
	gotoxy(2, 23);    printf("*");
	gotoxy(77, 3);    printf("*");
	gotoxy(77, 23);    printf("*");*/
}
//Класс описывающий еду
class Food
{
	int xf, yf;				// координаты еды
public:
	Food(int _x, int _y);
	void Draw_Food();		// функция отрисовки еды
	int X()
	{
		return xf;
	}
	int Y()
	{
		return yf;
	}
	void NewPosition(int _x, int _y) // Присваиваем новые координаты еде
	{
		xf = _x;
		yf = _y;
	}
};

//Конструктор имеет два целых числа в качестве параметров, которые будут назначены в качестве координат еды.
Food::Food(int _x, int _y) : xf(_x), yf(_y) {}

void Food::Draw_Food()
{
	gotoxy(xf, yf); printf("%c", 4); // Рисуем еду
}


//Класс описывающий змейку
class Snake {
	int body[200][2];					// Массив тела змейки
	int n;								// Переменная для управления индексом тела
	int x, y;							// Координаты змейки
	int dir;							// Переменная отвечающая за управление змейки				
	int h;								// Коэффицент умножения, для контроля скорости змейки

public:
	int tam;							// Стартовая длинна змейки
	int score;							// Переменная хранящая кол-во очков
	int speed;							// Скорость змейки
	int level;
	char key_controle;					// Ключ контроля
	Snake(int _x, int _y);  			// Конструктор змейки
	void position();					// Функция положения змейки
	void draw_body() const; 			// Функция рисовки тела змейки
	void del_body() const;  			// Функция затирания хвоста змейки
	bool game_over();					// Функция завершения игры
	void controls();					// Функция управления
	void update_position(); 			// Функция изменения позиции
	void foodANDsnake(Food&);			// Функция сравнения координат еды и змейки
	void edit_speed();					// Функция сравнения скорости
	void info() const;					// Функция вывода кол-ва очков на экран
	void show_record();
};
//конструктор класса змейки
Snake::Snake(int _x, int _y): x(_x), y(_y), tam(3), n(0), dir(3),score(0), h(1), speed(100), level(1) {}
//функция положения змейки



void Snake::position()
{
	body[n][0] = x;
	body[n][1] = y;
	n++;
	if (n == tam)      // перезаписываем индекс если он равен длине змейки
		n = 0;
}
//функция рисовки тела змейки
void Snake::draw_body() const
{
	gotoxy(x, y); printf("@"); // рисуем тело змейки символом @
}
//функция затирания "лишней" части змейки после хвоста (т.к. это массив, отрисованные символы тела змейки сами себя не затрут)
void Snake::del_body() const
{
	gotoxy(body[n][0], body[n][1]); printf(" "); // затираем хвост змейки
}
//функция управления змейкой
void Snake::controls()
{	// цикл на два прохода для того что бы считывание и изменение движения
	// змейки происходило синхронно, иначе управление словно с задержкой
	for(int i = 0; i < 2; i++) // пробигаем дважды для быстрого реагирования змейки на управление, иначе без цикла змейка управляется словно с задержкой, что плохо влияет на игру на больших скоростях
	{
		if (_kbhit()) 
		{
			key_controle = _getch();
			switch (key_controle)
			{
			case UP:    if (dir != 2) dir = 1; break;
			case DOWN:  if (dir != 1) dir = 2; break;
			case RIGHT: if (dir != 4) dir = 3; break; // при присвоении значения нажатой клавиши, перепроверяем что бы она не была ровна противоположному значению направления
			case LEFT:	if (dir != 3) dir = 4; break; //иначе бы змейка кушала сама себя, разворачиваясь в обратном направлении
			}

		}
	}
}
//функция обновления позиции змейки по нажатию на клавиши управления
void Snake::update_position()
{
	if		(dir == 3) x++;
	else if (dir == 1) y--;
	else if (dir == 4) x--;
	else if (dir == 2) y++;
}
//функция реализующая "съедания" еды
void Snake::foodANDsnake(Food& c) 
{
	if (c.X() == x && c.Y() == y)									// если координаты еды совпали с координатами змейки
	{
		score += 10;												// то прибавляем кол-во очков
		tam++;														// увелициваем размер змейки
		edit_speed();												// корректируем скорость змейки
		c.NewPosition((rand() % 73) + 4, (rand() % 19) + 4);	// вызываем фунцию генерации новых координат для еды
		c.Draw_Food();												// вызываем функцию отрисовки еды снова
		info();														//выводим надпись с результатом счета
	}
}

//функция соднржащая условия окончания игры, возвращает true/false 
bool Snake::game_over()
{
	if (y == 3 || y == 23 || x == 2 || x == 77) return true;	// если сталкиваемся с границами поля то true 
	for (int j = 0; j < tam; j++) {                      
		if (x == body[j][0] && y == body[j][1])					// если сталскиеваемся с телом змейки то true
			return true;
	}
	return false;												// в других случаеях все хорошо, играм дальше
}
//функция увеличивает скорость (уменьшает интервал обновления позиции)
void Snake::edit_speed() {
	if (score == h * 50) {	// если количество очков равно 50, 100, 150 и тд.
		speed -= 10;		// то уменьшаем время задержки, для увеличение скорости змейки 
		level++;		// инкриментируем данные уровня сложности
		h++;				// инкриментируем коэффицент умножения h
	}
}
//функция вывода информации о игровом процессе
void Snake::info() const
{
	int t = 0;
	t = clock() / CLOCKS_PER_SEC;	// преобразование миллисекунд в секунды
	setlocale(LC_ALL, "RUSSIAN");
	gotoxy(3, 1); printf("Счет:%d", score);		// количество очков
	gotoxy(17,1); printf("Длинна:%d", tam);		// длина змейки
	gotoxy(3, 2); printf("Уровень:%d", level);	// скорость змейки
	gotoxy(17, 2); printf("Время:%d", t);			// время в игре
}

void clearkeys()
{
	while (_kbhit())
		_getch();
}

bool once_more()
{
	setlocale(LC_ALL, "RUSSIAN");
	InvisCursor();
	gotoxy(40, 20); printf("Играть еше?");
	int ch = _getch();
	clearkeys();
	if (ch == 'N' || ch == 'n' || ch == 27)
		return false;
	return true;
}

const char* top10_file = "rating.txt";

void Snake::show_record()
{
	setlocale(LC_ALL, "RUSSIAN");
	system("cls");
	int t = clock() / CLOCKS_PER_SEC;	// преобразование миллисекунд в секунды
	gotoxy(40, 17); printf("Рекордное время в игре: %d сек.", t);
	gotoxy(40, 16); printf("Пройдено %d уровней", level);
	gotoxy(40, 15); printf("Рекордная длинна змейки: %d", tam);
	gotoxy(40, 14); printf("Рекордный счет: %d яблок", score);
	gotoxy(47, 12); printf("ВЫ ПРОИГРАЛИ"); 

	gotoxy(40, 18); printf("Введите ваше имя: ");
	std::cin >> name;
	
	ofstream fout(top10_file, ios_base::app); // открываем файл данных с режимом записи в конец файла
	fout << score << " " << tam << " " << level << " " << t << " " << name << "\n"; // записываем данные в файл
	fout.close(); // закрываем файл
}


void pak()
{
	setlocale(LC_ALL, "RUSSIAN");
	gotoxy(80/2,20); printf("Нажмите любую клавишу для продолжения...");
	_getch();
}

void Preview()
{
	setlocale(LC_ALL, "RUSSIAN");
	InvisCursor();
	gotoxy(90/2,30/4); printf("КЛАССИЧЕСКАЯ ИГРА ЗМЕЙКА");
	gotoxy(96 / 2, 30 / 3); printf("Версия 1.0");
	gotoxy(86 / 2, 30 / 2); printf("Разработал Корепанов Андрей");
	gotoxy(92 / 2, 16); printf("Студент СурГУ");
	pak();
	system("cls");
}

//структура хранящиая Топ10 игроков
struct SRecord {
	char name[16];     //имя игрока 
	int Score;         //количество набранных очков
	int Length;        // достигнутая длинна змейки
	int LvL;           // достигнутый уровень
	int Time;          // время в игре

	SRecord(); //  конструктор структуры
};

//стартовая инициализация структуры
SRecord::SRecord() {
	name[0] = '\0';
	Score = 0;
	Length = 0;
	LvL = 0;
	Time = 0;
}

SRecord ttop10[10]; // массив структур

istream& operator >> (istream& is, SRecord& rec) {
	is
		>> rec.Score
		>> rec.Length
		>> rec.LvL
		>> rec.Time;
	is.ignore(1);
	is.getline(&rec.name[0], 16);
	return is;
}


// функция считывающая, записывающая в структуру и выводящая на экран таблицу ТОП10 игроков
void ReadShow_top10()
{
	setlocale(LC_ALL, "RUSSIAN");
	ifstream fin(top10_file); // открываем файл на чтение
	if (fin) {
		for (int i = 0; i < 10; i++) // цикл чтения структур в массив
			fin >> ttop10[i];
	}
	fin.close(); // закрыли файл
	
	SRecord tmp; // временный объект структуры
	for (int i = 0; i < 10; i++) // цикл сортировки структур по длинне змейки
	{
		if (ttop10[i].Length < ttop10[i + 1].Length) 
		{
			tmp = ttop10[i + 1];
			ttop10[i + 1] = ttop10[i];   // перестановка структур местами 
			ttop10[i] = tmp;
		}
	}
	cout << "Имя\t\t\tОчки\tДлинна\tУровень\tВремя\n";  // выводим заголовок таблицы лидеров
	for(int j = 0; j < 10; j++)
	{
		cout << ttop10[j].name << "\t\t\t";
		cout << ttop10[j].Score << "\t";
		cout << ttop10[j].Length << "\t";				// выводим массив структур на экран
		cout << ttop10[j].LvL << "\t";
		cout << ttop10[j].Time << "\t";
		cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
	}
}


void game_loop()
{
	system("cls");
	srand(time(NULL));
	InvisCursor();										// скрыли курсор
	Border();											// отрисовали границу игрового поля
	Snake A(5, 10);								// проинициализировали конструктор класса змейки стартовой позицией змейки
	Food C((rand() % 73) + 4, (rand() % 19) + 4);	// проиницаиализировали конструктор класса еды, случайными значениями
	C.Draw_Food();										// нарисовали еду на игровой области
	while (A.key_controle != ESC && !A.game_over())		// играем пока не нажат ESC или не столкнулись с границей)
	{
		A.foodANDsnake(C);							// кушаем еду
		A.del_body();									// затираем ранне отрисованную змейку, в той позиции в которой ее уже нет
		A.position();									// переназначем положение змейки
		A.draw_body();									// рисуем тело змейки
		Sleep(A.speed);									//устанавливаем задержку обновления позиции
		A.controls();									// считываем управление змейкой
		A.update_position();							// меняем позицию по результату управления
		A.info();
	}
		A.show_record();
}

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	Preview();
	do
	{
		game_loop();
	} while (once_more());
	system("cls");
	ReadShow_top10(); // вызываем таблицу лидеров
	system("pause>>null");
	return 0;
}
