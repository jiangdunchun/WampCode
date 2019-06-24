using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Fleck;

namespace PacketServer
{
    class Program
    {
        static int _count = 0;

        static void Main(string[] args)
        {
            WebSocketServer _server = new WebSocketServer("ws://0.0.0.0:1994");
            _server.Start(socket => {
                socket.OnOpen = () =>
                {
                    _count = 0;
                    Thread newThread = new Thread(SendThread);
                    newThread.Start(socket);
                };
                socket.OnClose = () =>
                {
                    
                };
                socket.OnMessage = message =>
                {
                    
                };
            });
            Console.ReadLine();
        }

        static void SendThread(object param)
        {
            Thread.Sleep(1000);
            IWebSocketConnection socket = param as IWebSocketConnection;
            if (socket == null) return;

            FileReader reader = new FileReader();
            while (true)
            {
                if (reader.ReadOneFrameFromFile())
                {
                    Console.WriteLine("count:" + _count++ + ", length:" + reader.m_H264Frame.Length);
                    socket.Send(reader.m_H264Frame);
                }
                else
                {
                    reader.Close();
                    break;
                }
                Thread.Sleep(20);
            }
        }
    }

    class FileReader
    {
        List<byte> m_BufH264 = new List<byte>();
        
        FileStream m_fs = new FileStream("640x360.h264", FileMode.Open);
        //FileStream m_fs = new FileStream("856x480.h264", FileMode.Open);
        //FileStream m_fs = new FileStream("1280x720.h264", FileMode.Open);
        //FileStream m_fs = new FileStream("1920x1080.h264", FileMode.Open);
        //FileStream m_fs = new FileStream("3840x2160.h264", FileMode.Open);
        int m_iHaveRead = 0;
        int m_iReadLenth = 1024;
        public byte[] m_H264Frame;
        public int frame_index = 0;
        public bool ReadOneFrameFromFile()
        {
            while (!GetOneFrame())
            {
                if ((m_iHaveRead + m_iReadLenth) < m_fs.Length)
                {
                    m_iHaveRead += m_iReadLenth;
                    byte[] bRead = new byte[m_iReadLenth];
                    m_fs.Read(bRead, 0, m_iReadLenth);
                    m_BufH264.AddRange(bRead);
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
        public bool GetOneFrame()
        {
            if (m_BufH264.Count <= 8)
            {
                return false;
            }
            int iCount = 0;
            for (int i = 4; i < m_BufH264.Count - 4; i++)
            {
                if (m_BufH264[i] == 0 && m_BufH264[i + 1] == 0 && m_BufH264[i + 2] == 0 && m_BufH264[i + 3] == 1)
                {
                    iCount = i;
                    break;
                }
            }
            if (iCount > 0)
            {
                m_H264Frame = new byte[iCount];
                m_BufH264.CopyTo(0, m_H264Frame, 0, iCount);
                m_BufH264.RemoveRange(0, iCount);
                return true;
            }
            else
            {
                return false;
            }
        }
        public void Close()
        {
            m_fs.Close();
        }
    }
}
