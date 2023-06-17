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

map<SOCKET, int> clist; // ������ Ŭ���̾�Ʈ ���� ����
int count = 1;          // Ŭ���̾�Ʈ ��

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

    // ���� ����
    SOCKET s_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_sock == INVALID_SOCKET)
    {
        err_display("socket()");
        return -1;
    }

    // bind: local address(IP addr + Port num) + socket
    // bind�� �ʿ�� �ϴ� �Ķ����: ���� ��ũ����, local addr(SOCKADDR_IN), size

    SOCKADDR_IN saddr;             // local address
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s_sock, (SOCKADDR*)&saddr, sizeof(saddr))) // bind() != 0
    {
        err_display("bind()");
        return -1;
    }

    // listen: ������ listen ���·� ����� ���ÿ� listen buffer�� ����
    if (listen(s_sock, SOMAXCONN))
    {
        err_display("listen()");
        return -1;
    }

    SOCKADDR_IN caddr; // Ŭ���̾�Ʈ�� ������ ������ ����
    int namelen = sizeof(caddr); // accept �Լ��� ����° �Ķ����

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

        // thread ����
        thread th(echo_serv, c_sock);
        th.detach(); // detach�� ȣ���Ͽ� �����带 ��׶��忡�� �����ϵ��� ����
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
    wstring connect1 = L" �� �����Ͽ����ϴ�.\r\n";
    wstring msg1 = client1 + num1 + connect1;

    // Ŭ���̾�Ʈ ��ü���� �۽�
    for (const auto& pair : clist) {
        send(pair.first, reinterpret_cast<const char*>(msg1.c_str()), 80, 0);
    }

    while (1)
    {
        // ����
        recvlen = recv(c_sock, buf, 80, 0);
        if (recvlen == 0 || recvlen == SOCKET_ERROR)
        {
            cout << "client" << clist[c_sock] << " connection closed" << endl;

            wstring client2 = L"client ";
            wstring num2 = std::to_wstring(clist[c_sock]);
            wstring connect2 = L" �� ä�ù��� �������ϴ�.\r\n";
            wstring msg2 = client2 + num2 + connect2;

            // Ŭ���̾�Ʈ ��ü���� �۽�
            for (const auto& pair : clist) {
                send(pair.first, reinterpret_cast<const char*>(msg2.c_str()), 80, 0);
            }

            closesocket(c_sock);
            clist.erase(c_sock);
            break;
        }

        // ���� �����͸� ȭ�鿡 ���
        // ���� �����͸� ���ڿ��� ����� ���� ���������� �������� �� ���� �Է�
        buf[recvlen] = '\0';

        // wstring�� ������ ����
        std::wstring data(reinterpret_cast<wchar_t*>(buf), recvlen / 2);
        std::cout << "client" << clist[c_sock] << " : ";
        std::wcout << data << endl;

        wstring client = L"client ";
        wstring num = std::to_wstring(clist[c_sock]);
        wstring colon = L" : ";
        wstring msg = client + num + colon + data;

        // ���� �����͸� �״�� Ŭ���̾�Ʈ ��ü���� �۽�
        for (const auto& pair : clist) {
            send(pair.first, reinterpret_cast<const char*>(msg.c_str()), msg.size() * sizeof(wchar_t), 0);
        }
    }
}