
// ServerDlg.cpp : implementation file                                	//1121102
//                                                                    	//1121103

#include "stdafx.h"                                                   	//1121104
#include "Server.h"                                                   	//1121105
#include "ServerDlg.h"                                                	//1121106
#include "afxdialogex.h"                                              	//1121107

#ifdef _DEBUG                                                         	//1121108
#define new DEBUG_NEW                                                 	//1121109
#endif                                                                	//1121110

#include <winsock2.h>                                                 	//1121111

#define PORT 5150			// Порт по умолчанию                              	//1121112
#define DATA_BUFSIZE 8192 	// Размер буфера по умолчанию              	//1121113

int  iPort = PORT; 	 // Порт для прослушивания подключений            	//1121114
bool bPrint = false; // Выводить ли сообщения клиентов                	//1121115

typedef struct _SOCKET_INFORMATION {                                  	//1121116
	CHAR Buffer[DATA_BUFSIZE];                                           	//1121117
	WSABUF DataBuf;                                                      	//1121118
	SOCKET Socket;                                                       	//1121119
	OVERLAPPED Overlapped;                                               	//1121120
	DWORD BytesSEND;                                                     	//1121121
	DWORD BytesRECV;                                                     	//1121122
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;                          	//1121123

DWORD TotalSockets = 0; // Всего сокетов                              	//1121124
LPSOCKET_INFORMATION SocketArray[FD_SETSIZE];                         	//1121125

HWND   hWnd_LB;  // Для передачи потокам                              	//1121126

BOOL CreateSocketInformation(SOCKET s, CListBox * pLB);               	//1121127
void FreeSocketInformation(DWORD Index, CListBox * pLB);              	//1121128
UINT ListenThread(PVOID lpParam);                                     	//1121129

// CServerDlg dialog                                                  	//1121130

CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)                    	//1121131
	: CDialogEx(IDD_SERVER_DIALOG, pParent)                              	//1121132
{                                                                     	//1121133
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);                      	//1121134
}                                                                     	//1121135

void CServerDlg::DoDataExchange(CDataExchange* pDX)                   	//1121136
{                                                                     	//1121137
	CDialogEx::DoDataExchange(pDX);                                      	//1121138
	DDX_Control(pDX, IDC_LISTBOX, m_ListBox);                            	//1121139
}                                                                     	//1121140

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)                              	//1121141
	ON_WM_PAINT()                                                        	//1121142
	ON_WM_QUERYDRAGICON()                                                	//1121143
	ON_BN_CLICKED(IDC_START, &CServerDlg::OnBnClickedStart)              	//1121144
	ON_BN_CLICKED(IDC_PRINT, &CServerDlg::OnBnClickedPrint)              	//1121145
END_MESSAGE_MAP()                                                     	//1121146


// CServerDlg message handlers                                        	//1121147

BOOL CServerDlg::OnInitDialog()                                       	//1121148
{                                                                     	//1121149
	CDialogEx::OnInitDialog();                                           	//1121150

	// Set the icon for this dialog.  The framework does this automatically	//1121151
	//  when the application's main window is not a dialog               	//1121152
	SetIcon(m_hIcon, TRUE);			// Set big icon                            	//1121153
	SetIcon(m_hIcon, FALSE);		// Set small icon                          	//1121154

	// TODO: Add extra initialization here                               	//1121155
	char Str[128];                                                       	//1121156

	sprintf_s(Str, sizeof(Str), "%d", iPort);                            	//1121157
	GetDlgItem(IDC_PORT)->SetWindowText(Str);                            	//1121158

	if (bPrint)                                                          	//1121159
		((CButton *)GetDlgItem(IDC_PRINT))->SetCheck(1);                    	//1121160
	else                                                                 	//1121161
		((CButton *)GetDlgItem(IDC_PRINT))->SetCheck(0);                    	//1121162

	return TRUE;  // return TRUE  unless you set the focus to a control  	//1121163
}                                                                     	//1121164

// If you add a minimize button to your dialog, you will need the code below	//1121165
//  to draw the icon.  For MFC applications using the document/view model,	//1121166
//  this is automatically done for you by the framework.              	//1121167

void CServerDlg::OnPaint()                                            	//1121168
{                                                                     	//1121169
	if (IsIconic())                                                      	//1121170
	{                                                                    	//1121171
		CPaintDC dc(this); // device context for painting                   	//1121172

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);	//1121173

		// Center icon in client rectangle                                  	//1121174
		int cxIcon = GetSystemMetrics(SM_CXICON);                           	//1121175
		int cyIcon = GetSystemMetrics(SM_CYICON);                           	//1121176
		CRect rect;                                                         	//1121177
		GetClientRect(&rect);                                               	//1121178
		int x = (rect.Width() - cxIcon + 1) / 2;                            	//1121179
		int y = (rect.Height() - cyIcon + 1) / 2;                           	//1121180

		// Draw the icon                                                    	//1121181
		dc.DrawIcon(x, y, m_hIcon);                                         	//1121182
	}                                                                    	//1121183
	else                                                                 	//1121184
	{                                                                    	//1121185
		CDialogEx::OnPaint();                                               	//1121186
	}                                                                    	//1121187
}                                                                     	//1121188

// The system calls this function to obtain the cursor to display while the user drags	//1121189
//  the minimized window.                                             	//1121190
HCURSOR CServerDlg::OnQueryDragIcon()                                 	//1121191
{                                                                     	//1121192
	return static_cast<HCURSOR>(m_hIcon);                                	//1121193
}                                                                     	//1121194



void CServerDlg::OnBnClickedStart()                                   	//1121195
{                                                                     	//1121196
	// TODO: Add your control notification handler code here             	//1121197
	char Str[81];                                                        	//1121198

	hWnd_LB = m_ListBox.m_hWnd; // Для ListenThread                      	//1121199
	GetDlgItem(IDC_PORT)->GetWindowText(Str, sizeof(Str));               	//1121101
	iPort = atoi(Str);                                                   	//1121102
	if (iPort <= 0 || iPort >= 0x10000)                                  	//1121103
	{                                                                    	//1121104
		AfxMessageBox("Incorrect Port number");                             	//1121105
		return;                                                             	//1121106
	}                                                                    	//1121107

	AfxBeginThread(ListenThread, NULL);                                  	//1121108

	GetDlgItem(IDC_START)->EnableWindow(false);                          	//1121109
}                                                                     	//1121110


UINT ListenThread(PVOID lpParam)                                      	//1121111
{                                                                     	//1121112
	SOCKET	ListenSocket;                                                 	//1121113
	SOCKET	AcceptSocket;                                                 	//1121114
	SOCKADDR_IN	InternetAddr;                                            	//1121115
	WSADATA	wsaData;                                                     	//1121116
	INT		Ret;                                                            	//1121117
	FD_SET	WriteSet;                                                     	//1121118
	FD_SET	ReadSet;                                                      	//1121119
	DWORD	i;                                                             	//1121120
	DWORD	Total;                                                         	//1121121
	ULONG	NonBlock;                                                      	//1121122
	DWORD	Flags;                                                         	//1121123
	DWORD	SendBytes;                                                     	//1121124
	DWORD	RecvBytes;                                                     	//1121125

	char  Str[200];                                                      	//1121126
	CListBox  * pLB =                                                    	//1121127
		(CListBox *)(CListBox::FromHandle(hWnd_LB));                        	//1121128

	if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)                       	//1121129
	{                                                                    	//1121130
		sprintf_s(Str, sizeof(Str),                                         	//1121131
			"WSAStartup() failed with error %d", Ret);                         	//1121132
		pLB->AddString(Str);                                                	//1121133
		WSACleanup();                                                       	//1121134
		return 1;                                                           	//1121135
	}                                                                    	//1121136

	if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0,               	//1121137
		NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)                   	//1121138
	{                                                                    	//1121139
		sprintf_s(Str, sizeof(Str),                                         	//1121140
			"WSASocket() failed with error %d",                                	//1121141
			WSAGetLastError());                                                	//1121142
		pLB->AddString(Str);                                                	//1121143
		return 1;                                                           	//1121144
	}                                                                    	//1121145

	InternetAddr.sin_family = AF_INET;                                   	//1121146
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);                    	//1121147
	InternetAddr.sin_port = htons(iPort);                                	//1121148

	if (bind(ListenSocket, (PSOCKADDR)&InternetAddr,                     	//1121149
		sizeof(InternetAddr)) == SOCKET_ERROR)                              	//1121150
	{                                                                    	//1121151
		sprintf_s(Str, sizeof(Str),                                         	//1121152
			"bind() failed with error %d",                                     	//1121153
			WSAGetLastError());                                                	//1121154
		pLB->AddString(Str);                                                	//1121155
		return 1;                                                           	//1121156
	}                                                                    	//1121157

	if (listen(ListenSocket, 5))                                         	//1121158
	{                                                                    	//1121159
		sprintf_s(Str, sizeof(Str),                                         	//1121160
			"listen() failed with error %d",                                   	//1121161
			WSAGetLastError());                                                	//1121162
		pLB->AddString(Str);                                                	//1121163
		return 1;                                                           	//1121164
	}                                                                    	//1121165

	NonBlock = 1;                                                        	//1121166
	if (ioctlsocket(ListenSocket, FIONBIO, &NonBlock)                    	//1121167
		== SOCKET_ERROR)                                                    	//1121168
	{                                                                    	//1121169
		sprintf_s(Str, sizeof(Str),                                         	//1121170
			"ioctlsocket() failed with error %d",                              	//1121171
			WSAGetLastError());                                                	//1121172
		pLB->AddString(Str);                                                	//1121173
		return 1;                                                           	//1121174
	}                                                                    	//1121175

	while (TRUE)                                                         	//1121176
	{                                                                    	//1121177
		FD_ZERO(&ReadSet);                                                  	//1121178
		FD_ZERO(&WriteSet);                                                 	//1121179

		FD_SET(ListenSocket, &ReadSet);                                     	//1121180

		for (i = 0; i < TotalSockets; i++)                                  	//1121181
			if (SocketArray[i]->BytesRECV >                                    	//1121182
				SocketArray[i]->BytesSEND)                                        	//1121183
				FD_SET(SocketArray[i]->Socket, &WriteSet);                        	//1121184
			else                                                               	//1121185
				FD_SET(SocketArray[i]->Socket, &ReadSet);                         	//1121186

		if ((Total = select(0, &ReadSet, &WriteSet, NULL,                   	//1121187
			NULL)) == SOCKET_ERROR)                                            	//1121188
		{                                                                   	//1121189
			sprintf_s(Str, sizeof(Str),                                        	//1121190
				"select() returned with error %d",                                	//1121191
				WSAGetLastError());                                               	//1121192
			pLB->AddString(Str);                                               	//1121193
			return 1;                                                          	//1121194
		}                                                                   	//1121195

		if (FD_ISSET(ListenSocket, &ReadSet))                               	//1121196
		{                                                                   	//1121197
			Total--;                                                           	//1121198
			if ((AcceptSocket = accept(ListenSocket, NULL,                     	//1121199
				NULL)) != INVALID_SOCKET)                                         	//1121101
			{                                                                  	//1121102
				// Перевод сокета в неблокирующий режим                           	//1121103
				NonBlock = 1;                                                     	//1121104
				if (ioctlsocket(AcceptSocket, FIONBIO,                            	//1121105
					&NonBlock) == SOCKET_ERROR)                                      	//1121106
				{                                                                 	//1121107
					sprintf_s(Str, sizeof(Str),                                      	//1121108
						"ioctlsocket() failed with error %d",                           	//1121109
						WSAGetLastError());                                             	//1121110
					pLB->AddString(Str);                                             	//1121111
					return 1;                                                        	//1121112
				}                                                                 	//1121113
				// Добавление сокет в массив SocketArray                          	//1121114
				if (CreateSocketInformation(AcceptSocket, pLB)                    	//1121115
					== FALSE)                                                        	//1121116
					return 1;                                                        	//1121117
			}                                                                  	//1121118
			else                                                               	//1121119
			{                                                                  	//1121120
				if (WSAGetLastError() != WSAEWOULDBLOCK)                          	//1121121
				{                                                                 	//1121122
					sprintf_s(Str, sizeof(Str),                                      	//1121123
						"accept() failed with error %d",                                	//1121124
						WSAGetLastError());                                             	//1121125
					pLB->AddString(Str);                                             	//1121126
					return 1;                                                        	//1121127
				}                                                                 	//1121128
			}                                                                  	//1121129
		} // while                                                          	//1121130

		for (i = 0; Total > 0 && i < TotalSockets; i++)                     	//1121131
		{                                                                   	//1121132
			LPSOCKET_INFORMATION SocketInfo = SocketArray[i];                  	//1121133

			// Если сокет попадает в множество ReadSet,                        	//1121134
			// выполняем чтение данных.                                        	//1121135

			if (FD_ISSET(SocketInfo->Socket, &ReadSet))                        	//1121136
			{                                                                  	//1121137
				Total--;                                                          	//1121138

				SocketInfo->DataBuf.buf = SocketInfo->Buffer;                     	//1121139
				SocketInfo->DataBuf.len = DATA_BUFSIZE;                           	//1121140

				Flags = 0;                                                        	//1121141
				if (WSARecv(SocketInfo->Socket,                                   	//1121142
					&(SocketInfo->DataBuf), 1, &RecvBytes,                           	//1121143
					&Flags, NULL, NULL) == SOCKET_ERROR)                             	//1121144
				{                                                                 	//1121145
					if (WSAGetLastError() != WSAEWOULDBLOCK)                         	//1121146
					{                                                                	//1121147
						sprintf_s(Str, sizeof(Str),                                     	//1121148
							"WSARecv() failed with error %d",                              	//1121149
							WSAGetLastError());                                            	//1121150
						pLB->AddString(Str);                                            	//1121151

						FreeSocketInformation(i, pLB);                                  	//1121152
					}                                                                	//1121153
					continue;                                                        	//1121154
				}                                                                 	//1121155

				else // чтение успешно                                            	//1121156
				{                                                                 	//1121157
					SocketInfo->BytesRECV = RecvBytes;                               	//1121158

					// Если получено 0 байтов, значит клиент                         	//1121159
					// закрыл соединение.                                            	//1121160

					if (RecvBytes == 0)                                              	//1121161
					{                                                                	//1121162
						FreeSocketInformation(i, pLB);                                  	//1121163
						continue;                                                       	//1121164
					}                                                                	//1121165

					// Распечатка сообщения, если нужно                              	//1121166

					if (bPrint)                                                      	//1121167
					{                                                                	//1121168
						unsigned l = sizeof(Str) - 1;                                   	//1121169
						if (l > RecvBytes)                                              	//1121170
							l = RecvBytes;                                                 	//1121171
						strncpy_s(Str, SocketInfo->Buffer, l);                          	//1121172
						Str[l] = 0;                                                     	//1121173
						pLB->AddString(Str);                                            	//1121174
					}                                                                	//1121175
				}                                                                 	//1121176
			}                                                                  	//1121177

			// Если сокет попадает в множество WriteSet,                       	//1121178
			// выполняем отправку данных.                                      	//1121179

			if (FD_ISSET(SocketInfo->Socket, &WriteSet))                       	//1121180
			{                                                                  	//1121181
				Total--;                                                          	//1121182

				SocketInfo->DataBuf.buf =                                         	//1121183
					SocketInfo->Buffer + SocketInfo->BytesSEND;                      	//1121184
				SocketInfo->DataBuf.len =                                         	//1121185
					SocketInfo->BytesRECV - SocketInfo->BytesSEND;                   	//1121186

				if (WSASend(SocketInfo->Socket,                                   	//1121187
					&(SocketInfo->DataBuf), 1, &SendBytes, 0,                        	//1121188
					NULL, NULL) == SOCKET_ERROR)                                     	//1121189
				{                                                                 	//1121190
					if (WSAGetLastError() != WSAEWOULDBLOCK)                         	//1121191
					{                                                                	//1121192
						sprintf_s(Str, sizeof(Str),                                     	//1121193
							"WSASend() failed with error %d",                              	//1121194
							WSAGetLastError());                                            	//1121195
						pLB->AddString(Str);                                            	//1121196

						FreeSocketInformation(i, pLB);                                  	//1121197
					}                                                                	//1121198
					continue;                                                        	//1121199
				}                                                                 	//1121101
				else                                                              	//1121102
				{                                                                 	//1121103
					SocketInfo->BytesSEND += SendBytes;                              	//1121104

					if (SocketInfo->BytesSEND ==                                     	//1121105
						SocketInfo->BytesRECV)                                          	//1121106
					{                                                                	//1121107
						SocketInfo->BytesSEND = 0;                                      	//1121108
						SocketInfo->BytesRECV = 0;                                      	//1121109
					}                                                                	//1121110
				}                                                                 	//1121111
			}                                                                  	//1121112
		}                                                                   	//1121113
	}                                                                    	//1121114

	return 0;                                                            	//1121115
}                                                                     	//1121116


BOOL CreateSocketInformation(SOCKET s, CListBox *pLB)                 	//1121117
{                                                                     	//1121118
	LPSOCKET_INFORMATION SI;                                             	//1121119
	char Str[81];                                                        	//1121120

	sprintf_s(Str, sizeof(Str), "Accepted socket number %d", s);         	//1121121
	pLB->AddString(Str);                                                 	//1121122

	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)	//1121123
	{                                                                    	//1121124
		sprintf_s(Str, sizeof(Str), "GlobalAlloc() failed with error %d", GetLastError());	//1121125
		pLB->AddString(Str);                                                	//1121126
		return FALSE;                                                       	//1121127
	}                                                                    	//1121128

	// Подготовка структуры SocketInfo для использования.                	//1121129
	SI->Socket = s;                                                      	//1121130
	SI->BytesSEND = 0;                                                   	//1121131
	SI->BytesRECV = 0;                                                   	//1121132

	SocketArray[TotalSockets] = SI;                                      	//1121133
	TotalSockets++;                                                      	//1121134
	return(TRUE);                                                        	//1121135
}                                                                     	//1121136

void FreeSocketInformation(DWORD Index, CListBox *pLB)                	//1121137
{                                                                     	//1121138
	LPSOCKET_INFORMATION SI = SocketArray[Index];                        	//1121139
	DWORD i;                                                             	//1121140
	char Str[81];                                                        	//1121141

	closesocket(SI->Socket);                                             	//1121142

	sprintf_s(Str, sizeof(Str), "Closing socket number %d", SI->Socket); 	//1121143
	pLB->AddString(Str);                                                 	//1121144

	GlobalFree(SI);                                                      	//1121145

	// Сдвиг массива сокетов                                             	//1121146
	for (i = Index; i < TotalSockets; i++)                               	//1121147
	{                                                                    	//1121148
		SocketArray[i] = SocketArray[i + 1];                                	//1121149
	}                                                                    	//1121150
	TotalSockets--;                                                      	//1121151
}                                                                     	//1121152


void CServerDlg::OnBnClickedPrint()                                   	//1121153
{                                                                     	//1121154
	// TODO: Add your control notification handler code here             	//1121155
	if (((CButton *)GetDlgItem(IDC_PRINT))->GetCheck() == 1)             	//1121156
		bPrint = true;                                                      	//1121157
	else                                                                 	//1121158
		bPrint = false;                                                     	//1121159
}                                                                     	//1121160
