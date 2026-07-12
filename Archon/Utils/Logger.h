#pragma once
#include "Render/Render2DUtil.h"

namespace Log
{
	static RGBA COLOR_SUCCESS = RGBA(92, 150,  44); // Success
	static RGBA COLOR_ERROR   = RGBA( 247,  84, 100); // Error
	static RGBA COLOR_INFO    = RGBA(  41, 153, 153); // Info
	static RGBA COLOR_MESSAGE = RGBA( 139,   0,   0); // Message
	
    inline bool IniConsoleHook;
	inline HANDLE hStdout;
	inline bool IsInput = false;
	
	struct Message_ {
		enum MsgType {
			None,
			charptr,
			string,
			wcharptr,
			wstring
		};

		void* msg = nullptr;
		RGBA Color = COLOR_MESSAGE;
		MsgType msgtype = None;

		void PassMessage();

		Message_(const char* msg_, RGBA Color_ = COLOR_MESSAGE) {
			msgtype = MsgType::charptr;

			size_t strlencc = strnlen_s(msg_, 1000);

			char* str = new char[strlencc];

			for (size_t i = 0; i < strlencc; i++)
			{
				str[i] = msg_[i];
			}

			this->msg = (void*)str;
			this->Color = Color_;
		}

		Message_(std::string msg_, RGBA Color_ = COLOR_MESSAGE) {
			msgtype = MsgType::string;

			std::string* str = new std::string();

			for (size_t i = 0; i < msg_.length(); i++)
			{
				str[0][i] = msg_[i];
			}


			this->msg = (void*)str;
			this->Color = Color_;
		}

		Message_(std::wstring msg_, RGBA Color_ = COLOR_MESSAGE) {
			msgtype = MsgType::wstring;

			std::wstring* str = new std::wstring();

			for (size_t i = 0; i < msg_.length(); i++)
			{
				str[0][i] = msg_[i];
			}


			this->msg = (void*)str;
			this->Color = Color_;
		}

		Message_(const wchar_t* msg_, RGBA Color_ = COLOR_MESSAGE) {
			msgtype = MsgType::wcharptr;

			size_t strlencc = wcslen(msg_);

			wchar_t* str = new wchar_t[strlencc];

			for (size_t i = 0; i < strlencc; i++)
			{
				str[i] = msg_[i];
			}

			this->msg = (void*)str;
			this->Color = Color_;
		}
	};

	inline std::vector<Message_> MessageList;

	inline void EmptyQueue() {

		for (size_t i = 0; i < MessageList.size(); i++)
		{
			MessageList[i].PassMessage();
		}

		MessageList.clear();
	}

	inline std::string MakeAnsiColor(const RGBA& c)
	{
		char buf[64];
		
		std::snprintf(buf, sizeof(buf), "\x1b[38;2;%u;%u;%um", c.R, c.G, c.B);
		return std::string(buf);
	}

	inline const char* AnsiReset()
	{
		return "\x1b[0m";
	}

	inline void InitConsoleOnce()
	{
		if (IniConsoleHook)
			return;

		IniConsoleHook = true;
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		
		DWORD mode = 0;
		if (GetConsoleMode(hStdout, &mode))
		{
			mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hStdout, mode);
		}
	}

	
	__declspec(noinline) inline void LogA(const char* prefix, const char* msg, const RGBA& color)
	{
		InitConsoleOnce();

		if (IsInput)
		{
			MessageList.push_back(Message_(msg, color));
			return;
		}

		std::string ansi = MakeAnsiColor(color);
		
		std::cout << prefix;
		
		std::cout << ansi << msg << AnsiReset() << "\n";
	}
	
	__declspec(noinline) inline void LogA(const char* prefix, const std::string& msg, const RGBA& color)
	{
		LogA(prefix, msg.c_str(), color);
	}
	
	__declspec(noinline) inline void LogW(const wchar_t* prefix, const wchar_t* msg, const RGBA& color)
	{
		InitConsoleOnce();

		if (IsInput)
		{
			MessageList.push_back(Message_(msg, color));
			return;
		}

		std::string ansi = MakeAnsiColor(color);
		
		std::cout << ansi;
		std::wcout << prefix << msg << L"\n";
		std::cout << AnsiReset();
	}
	
	__declspec(noinline) inline void LogW(const wchar_t* prefix, const std::wstring& msg, const RGBA& color)
	{
		LogW(prefix, msg.c_str(), color);
	}

	inline void Success(const char* msg, const RGBA& color = COLOR_SUCCESS)
	{
		LogA(xorstr_("[+] "), msg, color);
	}

	inline void Success(const std::string& msg, const RGBA& color = COLOR_SUCCESS)
	{
		LogA(xorstr_("[+] "), msg, color);
	}

	inline void SuccessW(const wchar_t* msg, const RGBA& color = COLOR_SUCCESS)
	{
		LogW(xorstr_(L"[+] "), msg, color);
	}

	inline void SuccessW(const std::wstring& msg, const RGBA& color = COLOR_SUCCESS)
	{
		LogW(xorstr_(L"[+] "), msg, color);
	}

	inline void Error(const char* msg, const RGBA& color = COLOR_ERROR)
	{
		LogA(xorstr_("[-] "), msg, color);
	}

	inline void Error(const std::string& msg, const RGBA& color = COLOR_ERROR)
	{
		LogA(xorstr_("[-] "), msg, color);
	}

	inline void ErrorW(const wchar_t* msg, const RGBA& color = COLOR_ERROR)
	{
		LogW(xorstr_(L"[-] "), msg, color);
	}

	inline void ErrorW(const std::wstring& msg, const RGBA& color = COLOR_ERROR)
	{
		LogW(xorstr_(L"[-] "), msg, color);
	}


	inline void Info(const char* msg, const RGBA& color = COLOR_INFO)
	{
		LogA(xorstr_("[*] "), msg, color);
	}

	inline void Info(const std::string& msg, const RGBA& color = COLOR_INFO)
	{
		LogA(xorstr_("[*] "), msg, color);
	}

	inline void InfoW(const wchar_t* msg, const RGBA& color = COLOR_INFO)
	{
		LogW(xorstr_(L"[*] "), msg, color);
	}

	inline void InfoW(const std::wstring& msg, const RGBA& color = COLOR_INFO)
	{
		LogW(xorstr_(L"[*] "), msg, color);
	}


	inline void Message(const char* msg, const RGBA& color = COLOR_MESSAGE)
	{
		LogA(xorstr_("[*] "), msg, color);
	}

	inline void Message(const std::string& msg, const RGBA& color = COLOR_MESSAGE)
	{
		LogA(xorstr_("[*] "), msg, color);
	}

	inline void MessageW(const wchar_t* msg, const RGBA& color = COLOR_MESSAGE)
	{
		LogW(xorstr_(L"[*] "), msg, color);
	}

	inline void MessageW(const std::wstring& msg, const RGBA& color = COLOR_MESSAGE)
	{
		LogW(xorstr_(L"[*] "), msg, color);
	}



	inline void Message_::PassMessage()
	{
		switch (msgtype)
		{

		case charptr:
			Message((const char*)msg, Color);
			delete[](msg);
			break;

		case string:
			Message(*(std::string*)msg, Color);
			delete (std::string*)msg;
			break;

		case wstring:
			MessageW(*(std::wstring*)msg, Color);
			delete (std::wstring*)msg;
			break;

		case wcharptr:
			MessageW((const wchar_t*)msg, Color);
			delete[](msg);
			break;

		default:
			return;
			break;
		}
	}
	
	inline void MoveCursorConsole(int x, int y) {

		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		GetConsoleScreenBufferInfo(hStdout, &consoleInfo);


		COORD newPosition;

		newPosition.X = consoleInfo.dwCursorPosition.X + x;


		newPosition.Y = consoleInfo.dwCursorPosition.Y + y;

		SetConsoleCursorPosition(hStdout, newPosition);

	}
	
	inline void GetInputW(std::string Text, std::wstring& Output, bool& hasFinished) {
		if (IsInput) return;

		IsInput = true;

		std::cout.clear();
		std::cin.clear();

		std::fflush(stdin);

		Text = xorstr_("[Input] ") + Text;

		std::cout << Text;


		std::getline(std::wcin, Output);

		int lengthLine = Text.length() + Output.length();

		MoveCursorConsole(0, -1);
		MoveCursorConsole(-lengthLine, 0);


		for (size_t i = 0; i < lengthLine; i++)
		{
			std::cout << " ";
		}

		MoveCursorConsole(-lengthLine, 0);

		IsInput = false;

		EmptyQueue();

		hasFinished = true;
	}
}
