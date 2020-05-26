#include <windows.h>
#include <iostream>
#include<string>

using namespace std;

const int kTimeout = 1000;
void Server(char*); // Прототип сервера (COM1)

int main(int argc, char* argv[])
{
	MoveWindow(GetConsoleWindow(), 100, 100, 700, 600, true);

	system("cls");
	Server(argv[1]);
	return 0;
}

void Server(char* argv)
{
	STARTUPINFO cif;
	ZeroMemory(&cif, sizeof(STARTUPINFO));
	PROCESS_INFORMATION pi;

	CreateProcess(argv, NULL, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL, &cif, &pi); // Создание процесса

	string ComName = "COM1";
	HANDLE Semaphores[3];

	Semaphores[0] = CreateSemaphore(NULL, 0, 1, "SEMAPHORE");       // Семафор выполнения.
	Semaphores[1] = CreateSemaphore(NULL, 0, 1, "SEMAPHORE_END");   // Семафор завершения.
	Semaphores[2] = CreateSemaphore(NULL, 0, 1, "SEMAPHORE_EXIT");  // Семафор выхода.

	HANDLE Handler = CreateFile(
		ComName.c_str(),                                            // Имя открываемого файла.
		GENERIC_READ | GENERIC_WRITE,                               // Тип доступа к файлу.
		0,                                                          // Параметры совместного доступа.
		NULL,                                                       // Атрибуты защиты файла.
		OPEN_EXISTING,                                              // Режим автосоздания.
		FILE_ATTRIBUTE_NORMAL,                                      // Асинхронный режим работы.
		NULL                                                        // Описатель файла шаблона.
	);

	SetCommMask(Handler, EV_RXCHAR);                                // Устанавливаем маску на события порта.
	SetupComm(Handler, 1500, 1500);                                 // Инициализирует коммуникационные параметры для заданного устройства (Дескриптор, буфер ввода и вывода)

	COMMTIMEOUTS CommTimeOuts;                                      // Структура, характеризующая временные параметры последовательного порта.
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;                  // Mаксимальное время для интервала между поступлением двух символов по линии связи.
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;                    // Множитель, используемый, чтобы вычислить полный период времени простоя для операций чтения.
	CommTimeOuts.ReadTotalTimeoutConstant = kTimeout;               // Константа, используемая, чтобы вычислить полный (максимальный) период времени простоя для операций чтения.
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;                   // Множитель, используемый, чтобы вычислить полный период времени простоя для операций записи.
	CommTimeOuts.WriteTotalTimeoutConstant = kTimeout;              // Константа, используемая, чтобы вычислить полный период времени простоя для операций записи.

	if (!SetCommTimeouts(Handler, &CommTimeOuts))
	{
		CloseHandle(Handler);
		Handler = INVALID_HANDLE_VALUE;
		return;
	}

	DCB ComDCM;                               // Структура, характеризующая основные параметры последовательного порта.
	memset(&ComDCM, 0, sizeof(ComDCM));       // Выделение памяти под структуру.
	ComDCM.DCBlength = sizeof(DCB);           // Задает длину, в байтах, структуры.
	GetCommState(Handler, &ComDCM);           // Извлекает данные о текущих настройках управляющих сигналов для указанного устройства.
	ComDCM.BaudRate = DWORD(9600);            // Скорость передачи данных.
	ComDCM.ByteSize = 8;                      // Определяет число информационных бит в передаваемых и принимаемых байтах.
	ComDCM.Parity = NOPARITY;                 // Определяет выбор схемы контроля четности (Бит честности отсутствует).
	ComDCM.StopBits = ONESTOPBIT;             // Задает количество стоповых бит (Один бит).
	ComDCM.fAbortOnError = TRUE;              // Задает игнорирование всех операций чтения/записи при возникновении ошибки.
	ComDCM.fDtrControl = DTR_CONTROL_DISABLE; // Задает режим управления обменом для сигнала DTR.
	ComDCM.fRtsControl = RTS_CONTROL_DISABLE; // Задает режим управления потоком для сигнала RTS.
	ComDCM.fBinary = TRUE;                    // Включает двоичный режим обмена.
	ComDCM.fParity = FALSE;                   // Включает режим контроля четности.
	ComDCM.fInX = FALSE;                      // Задает использование XON/XOFF управления потоком при приеме.
	ComDCM.fOutX = FALSE;                     // Задает использование XON/XOFF управления потоком при передаче.
	ComDCM.XonChar = 0;                       // Задает символ XON используемый как для приема, так и для передачи.
	ComDCM.XoffChar = (unsigned char)0xFF;    // Задает символ XOFF используемый как для приема, так и для передачи.
	ComDCM.fErrorChar = FALSE;                // Задает символ, использующийся для замены символов с ошибочной четностью.
	ComDCM.fNull = FALSE;                     // Указывает на необходимость замены символов с ошибкой четности на символ задаваемый полем ErrorChar.
	ComDCM.fOutxCtsFlow = FALSE;              // Включает режим слежения за сигналом CTS.
	ComDCM.fOutxDsrFlow = FALSE;              // Включает режим слежения за сигналом DSR.
	ComDCM.XonLim = 128;                      // Задает минимальное число символов в приемном буфере перед посылкой символа XON.
	ComDCM.XoffLim = 128;                     // Определяет максимальное количество байт в приемном буфере перед посылкой символа XOFF.

	if (!SetCommState(Handler, &ComDCM))
	{
		CloseHandle(Handler);
		Handler = INVALID_HANDLE_VALUE;
		return;
	}

	cout << "Server\n\nEnter (Exit/exit) - to finish the work\n\n";

	while (true)
	{
		string message;
		DWORD NumberOfBytesWritten;
		char buffer[20];
		int bufferSize = sizeof(buffer);

		cout << "Enter message: ";
		cin.clear();
		cin >> message;
		if (message == "Exit" || message == "exit")
		{                                                             // Условие выхода из программы.
			ReleaseSemaphore(Semaphores[2], 1, NULL);                 // Реализация семафора выхода (Ставим в сигнальное состояние).
			break;
		}

		ReleaseSemaphore(Semaphores[0], 1, NULL);                     // Реализация семафора выполения (Ставим в сигнальное состояние).

		int NumberOfBlocks = message.size() / bufferSize + 1;         // Получение числа блоков 
		WriteFile(Handler, &NumberOfBlocks, sizeof(NumberOfBlocks), &NumberOfBytesWritten, NULL);

		int size = message.size();
		WriteFile(Handler, &size, sizeof(size), &NumberOfBytesWritten, NULL);

		for (int i = 0; i < NumberOfBlocks; i++)
		{
			message.copy(buffer, bufferSize, i * bufferSize);
			if (!WriteFile(Handler, buffer, bufferSize, &NumberOfBytesWritten, NULL))
				cout << "Error...";
		}
		WaitForSingleObject(Semaphores[1], INFINITE);                 // Ожидание перехода в несигнальное состояние.
	}

	CloseHandle(Handler);
	CloseHandle(Semaphores[0]);
	CloseHandle(Semaphores[1]);
	cout << "\n";
	system("pause");
}
