
// Async.Socket.Example.Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Async.Socket.Example.App.h"
#include "Async.Socket.Example.Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAsyncSocketExampleDlg dialog

const DWORD  DEFAULT_IP = INADDR_LOOPBACK;
const USHORT DEFAULT_PORT = 1609;

const int MODE_SERVER = 0;
const int MODE_CLIENT = 1;

const CString DEFAULT_MSG[] = { L"hello from server", L"hello from client" };

#define check(ret)\
  if ((ret) != vu::VU_OK) throw std::runtime_error(m_ptr_socket->get_last_error_message_A());

CAsyncSocketExampleDlg::CAsyncSocketExampleDlg(CWnd* pParent)
  : CDialogEx(CAsyncSocketExampleDlg::IDD, pParent)
  , m_ip(0), m_port(0)
  , m_msg(_T(""))
  , m_mode(FALSE)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAsyncSocketExampleDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_IPAddress(pDX, IDC_IP, m_ip);
  DDX_Text(pDX, IDC_PORT, m_port);
  DDX_Radio(pDX, IDC_MODE_SERVER, m_mode);
  DDX_Control(pDX, IDC_LOG, m_log);
  DDX_Text(pDX, IDC_MSG, m_msg);
  DDX_Control(pDX, IDC_CONNECTIONS, m_connections);
}

BEGIN_MESSAGE_MAP(CAsyncSocketExampleDlg, CDialogEx)
  ON_WM_CLOSE()
  ON_BN_CLICKED(IDC_START, OnBnClicked_Start)
  ON_BN_CLICKED(IDC_STOP,  OnBnClicked_Stop)
  ON_BN_CLICKED(IDC_CLEAR, OnBnClicked_Clear)
  ON_BN_CLICKED(IDC_SEND,  OnBnClicked_Send)
  ON_LBN_DBLCLK(IDC_CONNECTIONS, OnLbnDblclk_Connections)
  ON_BN_CLICKED(IDC_MODE_SERVER, OnBnClicked_ModeServer)
  ON_BN_CLICKED(IDC_MODE_CLIENT, OnBnClicked_ModeClient)
END_MESSAGE_MAP()

BOOL CAsyncSocketExampleDlg::OnInitDialog()
{
  __super::OnInitDialog();

  SetIcon(m_hIcon, TRUE);  // Set big icon
  SetIcon(m_hIcon, FALSE); // Set small icon

  m_ip   = DEFAULT_IP;
  m_port = DEFAULT_PORT;
  m_mode = MODE_SERVER;

  m_msg  = DEFAULT_MSG[m_mode];

  m_log.ResetContent();
  m_connections.ResetContent();

  UpdateData(FALSE);

  return TRUE; // return TRUE  unless you set the focus to a control
}

void CAsyncSocketExampleDlg::OnBnClicked_Start()
{
  VU_CPP_TRY()

  if (m_ptr_socket != nullptr)
  {
    return;
  }

  UpdateData(TRUE);

  m_connections.ResetContent();

  m_ptr_socket = new vu::AsyncSocket;
  this->add_log("Created");

  m_ptr_socket->on(vu::AsyncSocket::CONNECT, [&](vu::Socket& connection) -> void
  {
    this->fill_connections();
    auto s = vu::format_A("connected to %d\n", connection.get_remote_sai().sin_port);
    this->add_log(s);
  });

  m_ptr_socket->on(vu::AsyncSocket::OPEN, [&](vu::Socket& connection) -> void
  {
    this->fill_connections();
    auto s = vu::format_A("accepted %d\n", connection.get_remote_sai().sin_port);
    this->add_log(s);
  });

  m_ptr_socket->on(vu::AsyncSocket::CLOSE, [&](vu::Socket& connection) -> void
  {
    this->fill_connections();
    auto s = vu::format_A("closed %d\n", connection.get_remote_sai().sin_port);
    this->add_log(s);
  });

  m_ptr_socket->on(vu::AsyncSocket::SEND, [&](vu::Socket& connection) -> void
  {
    std::string data = vu::to_string_A(m_msg.GetBuffer(0));
    data += "\n";
    connection.send(data.data(), int(data.size()));

    auto s = vu::format_A("sent to %d message `%s`\n", connection.get_remote_sai().sin_port, data.c_str());
    this->add_log(s);
  });

  m_ptr_socket->on(vu::AsyncSocket::RECV, [&](vu::Socket& connection) -> void
  {
    std::vector<char> data(KiB);
    connection.recv(data.data(), int(data.size()));

    auto s = vu::format_A("recv from %d message `%s`\n", connection.get_remote_sai().sin_port, data.data());
    this->add_log(s);
  });

  struct sockaddr_in sai = { 0 };
  sai.sin_addr.s_addr = htonl(m_ip);
  const auto ip = inet_ntoa(sai.sin_addr);
  const vu::Socket::Endpoint endpoint(ip, m_port);

  if (m_mode == MODE_SERVER)
  {
    check(m_ptr_socket->bind(endpoint));
    this->add_log("Binded");

    check(m_ptr_socket->listen());
    this->add_log("Listening...");
  }
  else if (m_mode == MODE_CLIENT)
  {
    check(m_ptr_socket->connect(endpoint));
    this->add_log("Connected");
  }

  check(m_ptr_socket->run_in_thread());
  this->add_log("Running...");

  VU_CPP_CATCH(std::runtime_error& e)
  {
    delete m_ptr_socket;
    m_ptr_socket = nullptr;
    this->add_log("Destroy");

    const auto s = vu::to_string_W(e.what());
    AfxMessageBox(s.c_str());
  }
}

void CAsyncSocketExampleDlg::OnBnClicked_Stop()
{
  if (m_ptr_socket == nullptr)
  {
    return;
  }

  m_connections.ResetContent();

  m_ptr_socket->stop();
  this->add_log("Stopped");

  m_ptr_socket->close();

  delete m_ptr_socket;
  m_ptr_socket = nullptr;
  this->add_log("Destroy");
}

void CAsyncSocketExampleDlg::OnBnClicked_Clear()
{
  m_log.ResetContent();
}

void CAsyncSocketExampleDlg::OnBnClicked_Send()
{
  UpdateData(TRUE);

  for (int i = 0; i < m_connections.GetCount(); i++)
  {
    if (m_connections.GetSel(i) || m_connections.GetSel(0)) // all connections, or selected connections
    {
      if (SOCKET client = m_connections.GetItemData(i))
      {
        CString s = m_msg + L'\n';
        std::string data = vu::to_string_A(s.GetBuffer());
        m_ptr_socket->send(client, data.data(), int(data.size()));
        this->add_log(data);
      }
    }
  }
}

void CAsyncSocketExampleDlg::OnLbnDblclk_Connections()
{
  int idx = m_connections.GetCurSel();
  if (idx != -1)
  {
    CString s;
    m_connections.GetText(idx, s);

    if (SOCKET connection = m_connections.GetItemData(m_connections.GetCurSel()))
    {
      s.Format(_T("Would you like to disconnect %s ?"), s);
      if (AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION) == IDYES)
      {
        vu::Socket socket;
        socket.attach(connection);
        socket.disconnect();
      }
    }
    else // treat a null pointer as all connections
    {
      s.Format(_T("Would you like to disconnect all connections ?"));
      if (AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION) == IDYES)
      {
        m_ptr_socket->disconnect_connections();
      }
    }

    this->fill_connections();
  }
}

void CAsyncSocketExampleDlg::OnBnClicked_ModeServer()
{
  m_msg = DEFAULT_MSG[MODE_SERVER];
  GetDlgItem(IDC_MSG)->SetWindowText(m_msg);
}

void CAsyncSocketExampleDlg::OnBnClicked_ModeClient()
{
  m_msg = DEFAULT_MSG[MODE_CLIENT];
  GetDlgItem(IDC_MSG)->SetWindowText(m_msg);
}

BOOL CAsyncSocketExampleDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  auto result = __super::OnNotify(wParam, lParam, pResult);

  GetDlgItem(IDC_IP)->EnableWindow(m_ptr_socket == nullptr || !m_ptr_socket->running());
  GetDlgItem(IDC_PORT)->EnableWindow(m_ptr_socket == nullptr || !m_ptr_socket->running());
  GetDlgItem(IDC_START)->EnableWindow(m_ptr_socket == nullptr || !m_ptr_socket->running());
  GetDlgItem(IDC_STOP)->EnableWindow(m_ptr_socket != nullptr && m_ptr_socket->running());
  GetDlgItem(IDC_SEND)->EnableWindow(m_ptr_socket != nullptr && m_ptr_socket->running());
  GetDlgItem(IDC_MODE_SERVER)->EnableWindow(m_ptr_socket == nullptr || !m_ptr_socket->running());
  GetDlgItem(IDC_MODE_CLIENT)->EnableWindow(m_ptr_socket == nullptr || !m_ptr_socket->running());

  return result;
}

void CAsyncSocketExampleDlg::OnClose()
{
  this->OnBnClicked_Stop();
  __super::OnClose();
}

void CAsyncSocketExampleDlg::add_log(const std::string& text)
{
  CString s(text.c_str());
  m_log.InsertString(-1, s);
  m_log.SetTopIndex(m_log.GetCount() - 1);
}

void CAsyncSocketExampleDlg::fill_connections()
{
  m_connections.ResetContent();

  int idx = m_connections.AddString(L"All Connections");
  m_connections.SetItemData(idx, DWORD_PTR(NULL)); // treat a null pointer as all connections
  m_connections.SetCurSel(idx);

  auto connections = m_ptr_socket->get_connections();
  for (const auto& e : connections)
  {
    vu::Socket socket;
    socket.attach(e);

    const auto& sai = socket.get_remote_sai();
    const auto& address = sai.sin_addr.S_un.S_un_b;
    const int port = sai.sin_port;

    CString str;
    str.Format(L"%d.%d.%d.%d:%d", address.s_b1, address.s_b2, address.s_b3, address.s_b4, port);
    int idx = m_connections.InsertString(-1, str);
    m_connections.SetItemData(idx, DWORD_PTR(socket.handle()));
    m_connections.SetCurSel(idx);
  }
}
