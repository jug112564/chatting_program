#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <codecvt>  // for std::wstring_convert
#include <locale>   // for std::locale

#pragma comment(lib, "ws2_32.lib")

using std::cout;
using std::endl;
using std::map;
using std::thread;
using std::wstring;

void err_display(const char*);
void Print_IP(LPVOID);
void echo_serv(SOCKET c_sock);

map<SOCKET, int> clist; // 접속한 클라이언트 소켓 저장
int count = 1;          // 클라이언트 수

int main()
{
    std::wcout.imbue(std::locale(""));
    cout << "Server start" << endl;
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa))
    {
        err_display("WSAStartup");
        return -1;
    }

    // 소켓 생성
    SOCKET s_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_sock == INVALID_SOCKET)
    {
        err_display("socket()");
        return -1;
    }

    // bind: local address(IP addr + Port num) + socket
    // bind가 필요로 하는 파라미터: 소켓 디스크립터, local addr(SOCKADDR_IN), size

    SOCKADDR_IN saddr;             // local address
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s_sock, (SOCKADDR*)&saddr, sizeof(saddr))) // bind() != 0
    {
        err_display("bind()");
        return -1;
    }

    // listen: 소켓을 listen 상태로 만들고 동시에 listen buffer를 생성
    if (listen(s_sock, SOMAXCONN))
    {
        err_display("listen()");
        return -1;
    }

    SOCKADDR_IN caddr; // 클라이언트의 정보를 저장할 공간
    int namelen = sizeof(caddr); // accept 함수의 세번째 파라미터

    while (1)
    {
        SOCKET c_sock = accept(s_sock, (SOCKADDR*)&caddr, &namelen);
        if (c_sock == INVALID_SOCKET)
        {
            err_display("accept()");
            return -1;
        }

        cout << "connected client" << count << endl;

        clist.insert({ c_sock, count++ });

        // thread 생성
        thread th(echo_serv, c_sock);
        th.detach(); // detach를 호출하여 스레드를 백그라운드에서 실행하도록 설정
    }

    closesocket(s_sock);

    WSACleanup();

    return 0;
}

void err_display(const char* mes)
{
    LPVOID out;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&out, 0, NULL);
    cout << mes << " : " << (LPSTR)out << endl;
    LocalFree(out);
}

void Print_IP(LPVOID addr)
{
    char buf[20];

    if (!InetNtop(AF_INET, addr, buf, 20)) // if(InetNtop() == NULL)
    {
        err_display("InetNtop");
        return;
    }

    cout << "IP addr : " << buf << endl;
}

void echo_serv(SOCKET c_sock)
{
    char buf[80];
    int recvlen;

    SOCKADDR_IN caddr;
    int addrlen = sizeof(caddr);
    getpeername(c_sock, (SOCKADDR*)&caddr, &addrlen);
    cout << "Server connected by ";
    Print_IP(&caddr.sin_addr);

    wstring client1 = L"client ";
    wstring num1 = std::to_wstring(clist[c_sock]);
    wstring connect1 = L" 가 접속하였습니다.\r\n";
    wstring msg1 = client1 + num1 + connect1;

    // 클라이언트 전체에게 송신
    for (const auto& pair : clist) {
        send(pair.first, reinterpret_cast<const char*>(msg1.c_str()), 80, 0);
    }

    while (1)
    {
        // 수신
        recvlen = recv(c_sock, buf, 80, 0);
        if (recvlen == 0 || recvlen == SOCKET_ERROR)
        {
            cout << "client" << clist[c_sock] << " connection closed" << endl;

            wstring client2 = L"client ";
            wstring num2 = std::to_wstring(clist[c_sock]);
            wstring connect2 = L" 가 채팅방을 나갔습니다.\r\n";
            wstring msg2 = client2 + num2 + connect2;

            // 클라이언트 전체에게 송신
            for (const auto& pair : clist) {
                send(pair.first, reinterpret_cast<const char*>(msg2.c_str()), 80, 0);
            }

            closesocket(c_sock);
            clist.erase(c_sock);
            break;
        }

        // 수신 데이터를 화면에 출력
        // 수신 데이터를 문자열로 만들기 위해 강제적으로 마지막에 널 문자 입력
        buf[recvlen] = '\0';

        // wstring을 저장할 변수
        std::wstring data(reinterpret_cast<wchar_t*>(buf), recvlen / 2);
        std::cout << "client" << clist[c_sock] << " : ";
        std::wcout << data << endl;

        wstring client = L"client ";
        wstring num = std::to_wstring(clist[c_sock]);
        wstring colon = L" : ";
        wstring msg = client + num + colon + data;

        // 수신 데이터를 그대로 클라이언트 전체에게 송신
        for (const auto& pair : clist) {
            send(pair.first, reinterpret_cast<const char*>(msg.c_str()), msg.size() * sizeof(wchar_t), 0);
        }
    }
}