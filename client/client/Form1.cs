using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.Net.Sockets;
using System.IO;
using System.Threading;

namespace client
{
    public partial class Form1 : Form
    {
        TcpClient client;
        NetworkStream stream;
        public Form1()
        {
            InitializeComponent();
        }


        private void Form1_Load(object sender, EventArgs e)
        {
            //소켓 만들기
            client = new TcpClient();

            //서버에 연결 시도
            try
            {
                client.Connect("192.168.0.17", 8000);
                //stream 얻기
                stream = client.GetStream();
                Thread handler = new Thread(message_Receive);
                handler.IsBackground = true;
                handler.Start();
            }
            catch (SocketException se)
            {
                receiveTextBox.Text = se.Message;
            }


        }

        private void sendButton_Click(object sender, EventArgs e)
        {
            //stream에 메시지 보내기
            byte[] buffer = Encoding.Unicode.GetBytes(sendTextBox.Text);

            try
            {
                stream.Write(buffer, 0, buffer.Length);
            }
            catch (IOException ie)
            { 
                receiveTextBox.Text = ie.Message;
            }
        }

        private void message_Receive()
        {
            while(true) 
            {
                //메시지 받기
                byte[] recv = new byte[80];
                try
                {
                    int bytes = stream.Read(recv, 0, recv.Length);
                    string output = Encoding.Unicode.GetString(recv, 0, bytes);

                    if (receiveTextBox.InvokeRequired)
                        receiveTextBox.Invoke(new MethodInvoker(delegate { receiveTextBox.AppendText(output + Environment.NewLine); }));
                    else
                        receiveTextBox.AppendText(output + Environment.NewLine);
                }
                catch (IOException ie)
                {
                    if (receiveTextBox.InvokeRequired)
                        receiveTextBox.Invoke(new MethodInvoker(delegate { receiveTextBox.AppendText(ie.Message + Environment.NewLine); }));
                    else
                        receiveTextBox.AppendText(ie.Message + Environment.NewLine);
                    return;

                }
            }

        }
    }
}
