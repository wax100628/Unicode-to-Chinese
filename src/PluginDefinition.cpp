//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <string>
#include <algorithm>
#include <locale.h>

#define VALID_HEX_STR "0123456789abcdefABCDEF"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;


HWND getCurrentScintillaHandle()
{
	int currentEdit;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	return (currentEdit == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
};

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Convert U16 to Chinese"), Unicode2Chinese, NULL, false);
    setCommand(1, TEXT("Convert Chinese to U16"), Chinese2Unicode, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, CONST TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void Unicode2Utf8(char* pOut, const wchar_t* pIn)
{
	char* pwch = (char*)pIn;
	pOut[0] = 0xE0 | ((pwch[1] & 0xF0) >> 4);
	pOut[1] = (0x80 | ((pwch[1] & 0x0F) << 2)) + ((pwch[0] & 0xC0) >> 6);
	pOut[2] = 0x80 | (pwch[0] & 0x3F);
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void Unicode2Chinese()
{
	size_t pos = 0;
	char* pPreLocale = NULL;
	char *selectedText = NULL;
	char *pUtf8ChsText = NULL;
	size_t start = 0;
	size_t end = 0;
	std::string uniStr;
	size_t i, j;
	unsigned short uCh = 0;
	
	// 设置 C 运行时字符集
	pPreLocale = setlocale(LC_ALL, "Chinese");
	// 获取编辑器窗口句柄
	HWND hCurrScintilla = getCurrentScintillaHandle();
	// 获取光标选择的数据长度
	size_t selectedLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
	if (selectedLength == 0)
		goto _label_ret;

	selectedText = new char[selectedLength];
	if (!selectedText)
		goto _label_ret;

	pUtf8ChsText = new char[selectedLength];
	if (!pUtf8ChsText)
		goto _label_ret;

	// 获取选中字符串
	::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);
	selectedLength = strlen(selectedText) + 1;

	// 去除非 Hex 字符
	 uniStr = std::string(selectedText, selectedLength);
	if (uniStr.empty())
		goto _label_ret;
	while ((pos = uniStr.find_first_not_of(VALID_HEX_STR)) != std::string::npos)
		uniStr.erase(pos, 1);
	
	if (uniStr.empty())
	{
		::MessageBox(hCurrScintilla, _T("未选择任何有效Unicode 代码"), _T("提示"), MB_OK);
		goto _label_ret;
	}

	// 过滤后的字符串长度应是 4 的倍数
	if (uniStr.size() % 4)
	{
		::MessageBox(hCurrScintilla, _T("转换错误"), _T("错误"), MB_OK);
		goto _label_ret;
	}

	// 将 Unicode16 代码转换为 UTF-8 字符
	try
	{
		for (i = 0, j = 0; i < uniStr.size(); )
		{
			std::string str(uniStr, i, 4);
			uCh = std::stoi(str, 0, 16) & 0xFFFF;
			// 高位为 0 时，是 ASCII 码
			if ((uCh & 0xFF00) == 0x00)
			{
				*(pUtf8ChsText + j) = uCh & 0xFF;
				++j;
			}
			else
			{
				Unicode2Utf8(pUtf8ChsText + j, (wchar_t*)&uCh);
				j += 3;
			}

			i += 4;
		}
	}
	catch (std::exception&)
	{
		::MessageBox(hCurrScintilla, _T("转换异常"), _T("错误"), MB_OK);
		goto _label_ret;
	}

	start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
	end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
	if (end < start)
		std::swap(start, end);

	::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
	::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
	::SendMessage(hCurrScintilla, SCI_REPLACETARGET, j, (LPARAM)pUtf8ChsText);
	::SendMessage(hCurrScintilla, SCI_SETSEL, start, start + j);

_label_ret:
	if(selectedText)
		delete[] selectedText;
	if(pUtf8ChsText)
		delete[] pUtf8ChsText;
	if (pPreLocale)
		setlocale(LC_ALL, pPreLocale);
}

void Utf82Unicode(wchar_t* pOut, const char* pIn)
{
	char* pwch = (char*)pOut;
	pwch[1] = ((pIn[0] & 0x0F) << 4) + ((pIn[1] >> 2) & 0x0F);
	pwch[0] = ((pIn[1] & 0x03) << 6) + (pIn[2] & 0x3F);
}

void Chinese2Unicode()
{
	char* pPreLocale = NULL;
	char *selectedText = NULL;
	char *pU16CodeChsText = NULL;
	size_t start = 0;
	size_t end = 0;
	size_t i, j;
	char buf[16] = { 0 };
	unsigned short uCh = 0;

	// 设置 C 运行时字符集
	pPreLocale = setlocale(LC_ALL, "Chinese");
	// 获取编辑器窗口句柄
	HWND hCurrScintilla = getCurrentScintillaHandle();
	// 获取光标选择的数据长度
	size_t selectedLength = ::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
	if (selectedLength == 0)
		goto _label_ret;

	selectedText = new char[selectedLength];
	if (!selectedText)
		goto _label_ret;

	pU16CodeChsText = new char[selectedLength*6];
	if (!pU16CodeChsText)
		goto _label_ret;

	// 获取选中字符串
	::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)selectedText);
	selectedLength = strlen(selectedText);

	// 将 Unicode16 代码转换为 UTF-8 字符
	try
	{
		for (i = 0, j = 0; i < selectedLength; )
		{
			// 如果最高为 0 则表示这是单个字符
			if ((selectedText[i] & 0x80) == 0)
			{
				sprintf_s(buf, "\\u00%02x", selectedText[i] & 0xFF);
				++i;
			}
			else
			{
				Utf82Unicode((wchar_t*)&uCh, selectedText + i);
				sprintf_s(buf, "\\u%04x", uCh);
				i += 3;
			}

			memmove(pU16CodeChsText + j, buf, 6);
			j += 6;
		}
	}
	catch (std::exception&)
	{
		::MessageBox(hCurrScintilla, _T("转换失败"), _T("错误"), MB_OK);
		goto _label_ret;
	}


	start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
	end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
	if (end < start)
		std::swap(start, end);

	::SendMessage(hCurrScintilla, SCI_SETTARGETSTART, start, 0);
	::SendMessage(hCurrScintilla, SCI_SETTARGETEND, end, 0);
	::SendMessage(hCurrScintilla, SCI_REPLACETARGET, j, (LPARAM)pU16CodeChsText);
	::SendMessage(hCurrScintilla, SCI_SETSEL, start, start + j);

_label_ret:
	if (selectedText)
		delete[] selectedText;
	if (pU16CodeChsText)
		delete[] pU16CodeChsText;
	if (pPreLocale)
		setlocale(LC_ALL, pPreLocale);
}
